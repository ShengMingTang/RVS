/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2010-2018, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
Original authors:

Universite Libre de Bruxelles, Brussels, Belgium:
  Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be
  Daniele Bonatto, Daniele.Bonatto@ulb.ac.be
  Arnaud Schenkel, arnaud.schenkel@ulb.ac.be

Koninklijke Philips N.V., Eindhoven, The Netherlands:
  Bart Kroon, bart.kroon@philips.com
  Bart Sonneveldt, bart.sonneveldt@philips.com
*/

#include "Pipeline.hpp"
#include "Parser.hpp"
#include "Timer.hpp"
#include "BlendedView.hpp"
#include "SynthesizedView.hpp"
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

extern bool g_with_opengl;
#if WITH_OPENGL
#include "helpersGL.hpp"
#include "RFBO.hpp"
#endif

#define DUMP_MAPS false

Pipeline::Pipeline(std::string filename)
{
	this->m_filename = filename;

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

	for (int frame = m_config.start_frame; frame < m_config.start_frame + m_config.number_of_frames; ++frame) {
		if (m_config.number_of_frames > 1) {
			std::clog << std::string(5, '=') << " FRAME " << frame << ' ' << std::string(80, '=') << std::endl;
		}

		load_images(frame);
		compute_views(frame - m_config.start_frame);
	}
}

void Pipeline::load_images(int frame) {
	m_input_images.resize(m_config.params_real.size());

	if (m_config.znear.empty()) {
		throw std::runtime_error("We lost compatibility with \"RGB\" depth maps. z_far and z_near are required.");
	}

	for (std::size_t idx = 0; idx != m_input_images.size(); ++idx)
	{
		m_input_images[idx] = InputView(
			m_config.texture_names[idx],
			m_config.depth_names[idx],
			m_config.size,
			m_config.bit_depth_color,
			m_config.bit_depth_depth,
			m_config.znear[idx],
			m_config.zfar[idx],
			frame);
	}
}

void Pipeline::compute_views(int frame) {
#if WITH_OPENGL
	if (g_with_opengl) {
		auto FBO = RFBO::getInstance();
		FBO->init(cv::Size(int(g_rescale*m_config.virtual_size.width), int(g_rescale*m_config.virtual_size.height)));
	}
#endif
	for (std::size_t virtual_idx = 0; virtual_idx != m_config.params_virtual.size(); ++virtual_idx) {
		PROF_START("One view computed");
		std::unique_ptr<BlendedView> blender;

		if (m_config.blending_method == BLENDING_SIMPLE)
			blender.reset(new BlendedViewSimple(m_config.blending_factor));
		else if (m_config.blending_method == BLENDING_MULTISPEC)
			blender.reset(new BlendedViewMultiSpec(
				m_config.blending_low_freq_factor,
				m_config.blending_high_freq_factor));
		else throw std::logic_error("Unknown blending method");

		// Project according to parameters of the virtual view
		std::unique_ptr<SpaceTransformer> spaceTransformer;
#if WITH_OPENGL
		if (g_with_opengl) {
			spaceTransformer.reset(new OpenGLTransformer());
		}
#endif
		if (!g_with_opengl) {
			spaceTransformer.reset(new PUTransformer());
		}

		Parameters& params_virtual = m_config.params_virtual[virtual_idx];
		if(m_config.use_pose_trace)
		{
			params_virtual.adapt_initial_rotation( m_config.pose_trace[m_config.start_frame + frame].rotation );
			params_virtual.adapt_initial_translation( m_config.pose_trace[m_config.start_frame + frame].translation );
		}
		
		spaceTransformer->set_targetPosition(params_virtual, m_config.virtual_size, m_config.virtual_projection_type);


		for (std::size_t input_idx = 0; input_idx != m_input_images.size(); ++input_idx) {
			std::clog << __FUNCTION__ << ": frame=" << frame << ", input_idx=" << input_idx << ", virtual_idx=" << virtual_idx << std::endl;

			// Select type of un-projection 
			spaceTransformer->set_inputPosition(m_config.params_real[input_idx], m_config.size, m_config.input_projection_type);

			// Select view synthesis method
			std::unique_ptr<SynthesizedView> synthesizer;
			if (m_config.vs_method == SYNTHESIS_TRIANGLE)
				synthesizer.reset(new SynthetisedViewTriangle);
			else
				throw std::logic_error("Unknown synthesis method");

			// Bind to projectors
			synthesizer->setSpaceTransformer(spaceTransformer.get());

			// Memory optimization: Load the input image
			PROF_START("loading");
			m_input_images[input_idx].load();
			PROF_END("loading");

#if WITH_OPENGL
			if (g_with_opengl) {
				rd_start_capture_frame();
			}
#endif

			// Synthesize view
			synthesizer->compute(m_input_images[input_idx]);

			// Blend with previous results
			PROF_START("blending");
			blender->blend(*synthesizer);
			PROF_END("blending");

#if WITH_OPENGL
			if (g_with_opengl) {
				rd_end_capture_frame();
			}
#endif

#if DUMP_MAPS
			// Dump texture, depth, quality and validity maps for analysis (with DUMP_MAPS enabled)
			dump_maps(input_idx, virtual_idx, *synthesizer, *blender);
#endif

			// Memory optimization: Unload the input image 
			m_input_images[input_idx].unload();
		}
		
#if WITH_OPENGL
		if (g_with_opengl) {
			blender->assignFromGL2CV(cv::Size(int(g_rescale*m_config.virtual_size.width), int(g_rescale*m_config.virtual_size.height)));
		}
#endif

		PROF_START("inpainting");
		cv::Mat3f color = inpaint(blender->get_color(), blender->get_inpaint_mask(), true);
		PROF_END("inpainting");

		PROF_START("downscale");
		resize(color, color, m_config.virtual_size);
		PROF_END("downscale");

		if (!m_config.outfilenames.empty()) {
			PROF_START("write");
			write_color(m_config.outfilenames[virtual_idx], color, m_config.bit_depth_color, frame);
			PROF_END("write");
		}

		if (!m_config.outmaskedfilenames.empty()) {
			PROF_START("masking");
			auto mask = blender->get_validity_mask(m_config.validity_threshold);
			resize(mask, mask, m_config.virtual_size, cv::INTER_NEAREST);
			color.setTo(cv::Vec3f::all(0.5f), mask);
			write_color(m_config.outmaskedfilenames[virtual_idx], color, m_config.bit_depth_color, frame);
			PROF_END("masking");
		}
		PROF_END("One view computed");

#if WITH_OPENGL
		if (g_with_opengl) {
		auto FBO = RFBO::getInstance();
		FBO->free();
		}
#endif
	}
}

void Pipeline::parse()
{
	Parser p(m_filename);
	this->m_config = p.get_config();
}

void Pipeline::dump_maps(std::size_t input_idx, std::size_t virtual_idx, View const& synthesizer, View const& blender)
{
	cv::Mat3f rgb;
	cv::Mat3b color;
	cv::Mat1w depth;
	cv::Mat1w quality;
	cv::Mat1w validity; // triangle shape
	std::ostringstream filepath;

	cv::cvtColor(m_input_images[input_idx].get_color(), rgb, cv::COLOR_YCrCb2BGR);
	rgb.convertTo(color, CV_8U, 255.);
	m_input_images[input_idx].get_depth().convertTo(depth, CV_16U, 2000.);

	filepath.str(""); filepath << "dump-input-color-" << input_idx << "to" << virtual_idx << ".png"; cv::imwrite(filepath.str(), color);
	filepath.str(""); filepath << "dump-input-depth-" << input_idx << "to" << virtual_idx << "_x2000.png"; cv::imwrite(filepath.str(), depth);
	filepath.str(""); filepath << "dump-input-depth_mask-" << input_idx << "to" << virtual_idx << ".png"; cv::imwrite(filepath.str(), m_input_images[input_idx].get_depth_mask());

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
