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

#include "Pipeline.hpp"
#include "Parser.hpp"
#include "Timer.hpp"
#include "BlendedView.hpp"
#include "SynthetizedView.hpp"
#include "inpainting.hpp"
#include "image_writing.hpp"

#include <iostream>
#include <vector>
#include <memory>



Pipeline::Pipeline(std::string filename)
{
	this->filename = filename;
}

Pipeline::~Pipeline()
{
}

void Pipeline::execute()
{
	parse();
	create_images();
	load_images();
	compute_views();
}


void Pipeline::load_image(int idx) {
	input_images[idx].load();
	input_images[idx].preprocess_depth();
}

void Pipeline::load_images() {

	PROF_START("loading");
	for (size_t idx = 0; idx < input_images.size(); ++idx) {
		load_image((int)idx);
	}
	PROF_END("loading");
}

void Pipeline::unload_image(int i)
{
	input_images[i].unload();
}

void Pipeline::unload_images()
{
	for (size_t idx = 0; idx < input_images.size(); ++idx) {
		unload_image((int)idx);
	}
}


void Pipeline::compute_view(int idx) {
	//std::cout << filename << std::endl;
	std::unique_ptr<BlendedView> BV;
	if (config.blending_method == BLENDING_SIMPLE) {
		auto simple = new BlendedViewSimple;
		BV.reset(simple);
		simple->set_blending_exp(config.blending_factor);
	}
	else if (config.blending_method == BLENDING_MULTISPEC) {
		auto multispec = new BlendedViewMultiSpec;
		BV.reset(multispec);
		multispec->set_blending_exp(config.blending_low_freq_factor, config.blending_high_freq_factor);
	}
	for (int input_idx = 0; input_idx < static_cast<int>(input_images.size()); input_idx++) {
		//compute result from view reference idx
		std::unique_ptr<SynthetizedView> SV;
		if (vs_method == SYNTHESIS_TRIANGLE)
			SV.reset(new SynthetizedViewTriangle(config.params_virtual[idx], input_images[input_idx].get_size()));
		else if (vs_method == SYNTHESIS_SQUARE)
			SV.reset(new SynthetizedViewSquare(config.params_virtual[idx], input_images[input_idx].get_size()));
		else
			throw std::logic_error("Unknown synthesis method");
		SV->compute(input_images[input_idx]);
		//blend with previous results
		PROF_START("blending");
		BV->blend(*SV);
		PROF_END("blending");
	}
	cv::Mat to_inpaint = BV->get_inpaint_mask();

	cv::Mat result = BV->get_color();

	PROF_START("inpainting");

	result = inpaint(result, to_inpaint, true);

	PROF_END("inpainting");

	cv::Size s = BV->get_size();
	resize(result, result, s);

	if (!save) {
		cv::imshow("img1", result);
		cv::waitKey(0);
	}
	else {
		result = result * 255.0f;
		result.convertTo(result, CV_8UC3);
		write_color(config.outfilenames[idx], result);
	}

}


void Pipeline::compute_views() {

	for (int idx = 0; idx < static_cast<int>(config.params_virtual.size()); idx++) {
		std::cout << config.outfilenames[idx] << std::endl;
		compute_view(idx);
	}
}

void Pipeline::parse()
{
	Parser p(filename);
	this->config = p.get_config();
}

void Pipeline::create_images()
{
	for (size_t idx = 0; idx < config.params_real.size(); ++idx) {
		create_image((int)idx);
	}
}

void Pipeline::create_image(int idx)
{
	View image = View(config.texture_names[idx], config.depth_names[idx], config.params_real[idx], config.size);
	if (config.znear.size() > 0)
		image.set_z(config.znear[idx], config.zfar[idx]);

	input_images.push_back(image);
}
