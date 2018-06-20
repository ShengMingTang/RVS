/*------------------------------------------------------------------------------ -

Copyright © 2018 - 2025 Université Libre de Bruxelles(ULB)

Authors : Sarah Fachada, Daniele Bonatto, Arnaud Schenkel
Contact : Gauthier.Lafruit@ulb.ac.be

SVS – Several inputs View Synthesis
This software synthesizes virtual views at any position and orientation in space,
from any number of camera input views, using depth image - based rendering
techniques.

Permission is hereby granted, free of charge, to the members of the Moving Picture
Experts Group(MPEG) obtaining a copy of this software and associated documentation
files(the "Software"), to use the Software exclusively within the framework of the
MPEG - I(immersive) and MPEG - I Visual activities, for the sole purpose of
developing the MPEG - I standard.This permission includes without limitation the
rights to use, copy, modify and merge copies of the Software, and explicitly
excludes the rights to publish, distribute, sublicense, sell, embed into a product
or a service and/or otherwise commercially exploit copies of the Software without
the written consent of the owner(ULB).

This permission is provided subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies, substantial portions or derivative works of the Software.

------------------------------------------------------------------------------ -*/

/*------------------------------------------------------------------------------ -

This source file has been modified by Koninklijke Philips N.V. for the purpose of
of the 3DoF+ Investigation.
Modifications copyright © 2018 Koninklijke Philips N.V.

Support for n-bit raw texture and depth streams.
Generalization of camera projections

Author  : Bart Kroon
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

#include "Pipeline.hpp"
#include "Parser.hpp"
#include "Timer.hpp"
#include "BlendedView.hpp"
#include "SynthetizedView.hpp"
#include "inpainting.hpp"
#include "image_writing.hpp"
#include "PerspectiveUnprojector.hpp"
#include "PerspectiveProjector.hpp"
#include "EquirectangularProjection.hpp"

#include <iostream>
#include <vector>
#include <memory>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#define DUMP_MAPS false

Pipeline::Pipeline(std::string filename)
{
	this->filename = filename;

#ifndef NDEBUG
	cv::setBreakOnError(true);
#endif
}

Pipeline::~Pipeline()
{
}

void Pipeline::execute()
{
	parse();

	for (int frame = config.start_frame; frame < config.start_frame + config.number_of_frames; ++frame) {
		if (config.number_of_frames > 1) {
			std::clog << std::string(5, '=') << " FRAME " << frame << ' ' << std::string(80, '=') << std::endl;
		}

		load_images(frame);
		compute_views(frame - config.start_frame);
	}
}

void Pipeline::load_images(int frame) {
	input_images.resize(config.params_real.size());

	if (config.znear.empty()) {
		throw std::runtime_error("We lost compatibility with \"RGB\" depth maps. z_far and z_near are required.");
	}

	for (std::size_t idx = 0; idx != input_images.size(); ++idx)
	{
		input_images[idx] = InputView(
			config.texture_names[idx],
			config.depth_names[idx],
			config.size,
			config.bit_depth_color,
			config.bit_depth_depth,
			config.znear[idx],
			config.zfar[idx],
			frame);
	}
}

void Pipeline::compute_views(int frame) {
	for (std::size_t virtual_idx = 0; virtual_idx != config.params_virtual.size(); ++virtual_idx) {
		std::unique_ptr<BlendedView> blender;

		if (config.blending_method == BLENDING_SIMPLE)
			blender.reset(new BlendedViewSimple(config.blending_factor));
		else if (config.blending_method == BLENDING_MULTISPEC)
			blender.reset(new BlendedViewMultiSpec(
				config.blending_low_freq_factor,
				config.blending_high_freq_factor));
		else throw std::logic_error("Unknown blending method");

		// Project according to parameters of the virtual view
		std::unique_ptr<Projector> projector;

		Parameters& params_virtual = config.params_virtual[virtual_idx];
		if( config.use_pose_trace)
		{
			params_virtual.adapt_initial_rotation( config.pose_trace[frame].rotation );
			params_virtual.adapt_initial_translation( config.pose_trace[frame].translation );
		}

		if( config.virtual_projection_type == PROJECTION_PERSPECTIVE )
			projector.reset(new PerspectiveProjector(params_virtual, config.virtual_size));
		else if ( config.virtual_projection_type == PROJECTION_EQUIRECTANGULAR )
			projector.reset(new erp::Projector(params_virtual, config.virtual_size));

		for (std::size_t input_idx = 0; input_idx != input_images.size(); ++input_idx) {
			std::clog << __FUNCTION__ << ": frame=" << frame << ", input_idx=" << input_idx << ", virtual_idx=" << virtual_idx << std::endl;

			// Select type of un-projection 
			std::unique_ptr<Unprojector> unprojector;
			if( config.input_projection_type == PROJECTION_PERSPECTIVE )
				unprojector.reset(new PerspectiveUnprojector(config.params_real[input_idx]));
			else if ( config.input_projection_type == PROJECTION_EQUIRECTANGULAR )
				unprojector.reset(new erp::Unprojector(config.params_real[input_idx], config.size ));

			// Select view synthesis method
			std::unique_ptr<SynthetizedView> synthesizer;
			if (vs_method == SYNTHESIS_TRIANGLE)
				synthesizer.reset(new SynthetizedViewTriangle);
			else
				throw std::logic_error("Unknown synthesis method");

			// Bind to projectors
			synthesizer->setUnprojector(unprojector.get());
			synthesizer->setProjector(projector.get());

			// Memory optimization: Load the input image
			PROF_START("loading");
			input_images[input_idx].load();
			PROF_END("loading");

			// Synthesize view
			synthesizer->compute(input_images[input_idx]);

			// Blend with previous results
			PROF_START("blending");
			blender->blend(*synthesizer);
			PROF_END("blending");

#if DUMP_MAPS
			// Dump texture, depth, quality and validity maps for analysis (with DUMP_MAPS enabled)
			dump_maps(input_idx, virtual_idx, *synthesizer, *blender);
#endif

			// Memory optimization: Unload the input image 
			input_images[input_idx].unload();
		}

		PROF_START("inpainting");
		cv::Mat3f color = inpaint(blender->get_color(), blender->get_inpaint_mask(), true);
		PROF_END("inpainting");

		PROF_START("downscale");
		resize(color, color, config.virtual_size);
		PROF_END("downscale");

		if (!config.outfilenames.empty()) {
			PROF_START("write");
			write_color(config.outfilenames[virtual_idx], color, config.bit_depth_color, frame);
			PROF_END("write");
		}

		if (!config.outmaskedfilenames.empty()) {
			PROF_START("masking");
			auto mask = blender->get_validity_mask(config.validity_threshold);
			resize(mask, mask, config.virtual_size, cv::INTER_NEAREST);
			color.setTo(cv::Vec3f::all(0.5f), mask);
			write_color(config.outmaskedfilenames[virtual_idx], color, config.bit_depth_color, frame);
			PROF_END("masking");
		}
	}
}

void Pipeline::parse()
{
	Parser p(filename);
	this->config = p.get_config();
}

void Pipeline::dump_maps(std::size_t input_idx, std::size_t virtual_idx, View const& synthesizer, View const& blender)
{
	cv::Mat3f rgb;
	cv::Mat3b color;
	cv::Mat1w depth;
	cv::Mat1w quality;
	cv::Mat1w validity; // triangle shape
	std::ostringstream filepath;

	cv::cvtColor(input_images[input_idx].get_color(), rgb, cv::COLOR_YCrCb2BGR);
	rgb.convertTo(color, CV_8U, 255.);
	input_images[input_idx].get_depth().convertTo(depth, CV_16U, 2000.);

	filepath.str(""); filepath << "dump-input-color-" << input_idx << "to" << virtual_idx << ".png"; cv::imwrite(filepath.str(), color);
	filepath.str(""); filepath << "dump-input-depth-" << input_idx << "to" << virtual_idx << "_x2000.png"; cv::imwrite(filepath.str(), depth);
	filepath.str(""); filepath << "dump-input-depth_mask-" << input_idx << "to" << virtual_idx << ".png"; cv::imwrite(filepath.str(), input_images[input_idx].get_depth_mask());

	cv::cvtColor(synthesizer.get_color(), rgb, cv::COLOR_YCrCb2BGR);
	rgb.convertTo(color, CV_8U, 255.);
	synthesizer.get_quality().convertTo(quality, CV_16U, 4.);
	synthesizer.get_depth().convertTo(depth, CV_16U, 2000.);
	synthesizer.get_validity().convertTo(validity, CV_16U, 6.);

	filepath.str(""); filepath << "dump-color-" << input_idx << "to" << virtual_idx << ".png"; cv::imwrite(filepath.str(), color);
	filepath.str(""); filepath << "dump-quality-" << input_idx << "to" << virtual_idx << "_x4.png"; cv::imwrite(filepath.str(), quality);
	filepath.str(""); filepath << "dump-validity-" << input_idx << "to" << virtual_idx << "_x6.png"; cv::imwrite(filepath.str(), validity);
	filepath.str(""); filepath << "dump-depth-" << input_idx << "to" << virtual_idx << "_x2000.png"; cv::imwrite(filepath.str(), depth);
	filepath.str(""); filepath << "dump-depth_mask-" << input_idx << "to" << virtual_idx << ".png"; cv::imwrite(filepath.str(), synthesizer.get_depth_mask());

	cv::cvtColor(blender.get_color(), rgb, cv::COLOR_YCrCb2BGR);
	rgb.convertTo(color, CV_8U, 255.);
	blender.get_quality().convertTo(quality, CV_16U, 4.);
	blender.get_validity().convertTo(validity, CV_16U, 6.);

	filepath.str(""); filepath << "dump-blended-color-" << input_idx << "to" << virtual_idx << ".png"; cv::imwrite(filepath.str(), color);
	filepath.str(""); filepath << "dump-blended-quality-" << input_idx << "to" << virtual_idx << "_x4.png"; cv::imwrite(filepath.str(), quality);
	filepath.str(""); filepath << "dump-blended-validity-" << input_idx << "to" << virtual_idx << "_x6.png"; cv::imwrite(filepath.str(), validity);
}
