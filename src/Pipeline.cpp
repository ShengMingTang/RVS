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
#include "Timer.hpp"
#include "BlendedView.hpp"
#include "SynthesizedView.hpp"
#include "inpainting.hpp"
#include "image_writing.hpp"

#include <iostream>
#include <vector>
#include <memory>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#if WITH_OPENGL
#include "helpersGL.hpp"
#include "RFBO.hpp"
#endif

#define DUMP_MAPS false

Pipeline::Pipeline(std::string const& filepath)
	: m_config(Config::loadFromFile(filepath))
{
#ifndef NDEBUG
	cv::setBreakOnError(true);
#endif
}

void Pipeline::execute()
{
	for (auto virtualFrame = 0; virtualFrame < m_config.number_of_frames; ++virtualFrame) {
		auto inputFrame = m_config.start_frame + virtualFrame;
		if (m_config.number_of_frames > 1) {
			std::cout << std::string(5, '=') << " FRAME " << inputFrame << ' ' << std::string(80, '=') << std::endl;
		}
		for (auto virtualView = 0; virtualView != m_config.VirtualCameraNames.size(); ++virtualView) {
			computeView(inputFrame, virtualFrame, virtualView);
		}
	}
}

void Pipeline::computeView(int inputFrame, int virtualFrame, int virtualView)
{
	PROF_START("computeView");

	// Virtual view parameters for this frame and view
	auto params_virtual = m_config.params_virtual[virtualView];
	if (!m_config.pose_trace.empty()) {
		auto pose = m_config.pose_trace[inputFrame];
		params_virtual.adaptPose(pose.translation, pose.rotation);
	}

	// Initialize OpenGL frame buffer objects
#if WITH_OPENGL
	auto intermediateSize = cv::Size(
		int(g_rescale*params_virtual.getSize().width),
		int(g_rescale*params_virtual.getSize().height));
	if (g_with_opengl) {
		auto FBO = RFBO::getInstance();
		FBO->init(intermediateSize);
	}
#endif

	// Setup a view blender
	auto blender = createBlender();

	// Partial setup of a space transformer
	auto spaceTransformer = createSpaceTransformer();
	spaceTransformer->set_targetPosition(&params_virtual);

	// For each input view
	for (auto inputView = 0; inputView != m_config.InputCameraNames.size(); ++inputView) {
		std::cout << inputFrame << '.' << inputView << " => " << virtualFrame << '.' << virtualView << std::endl;
		auto const& params_real = m_config.params_real[inputView];

		// Complete setup of space transformer
		spaceTransformer->set_inputPosition(&params_real);

		// Setup a view synthesizer
		auto synthesizer = createSynthesizer();
		synthesizer->setSpaceTransformer(spaceTransformer.get());

		// Load the input image
		PROF_START("loading");
		InputView inputImage(
			m_config.texture_names[inputView], 
			m_config.depth_names[inputView], 
			inputFrame, params_real);
		PROF_END("loading");

		// Start OpenGL instrumentation (if any)
#if WITH_OPENGL
		if (g_with_opengl) {
			rd_start_capture_frame();
		}
#endif
		// Synthesize view
		synthesizer->compute(inputImage);

		// Blend with previous results
		PROF_START("blending");
		blender->blend(*synthesizer);
		PROF_END("blending");

		// End OpenGL instrumentation (if any)
#if WITH_OPENGL
		if (g_with_opengl) {
			rd_end_capture_frame();
		}
#endif

#if DUMP_MAPS
		// Dump texture, depth, quality and validity maps for analysis (with DUMP_MAPS enabled)
		dumpMaps(inputImage, inputView, virtualView, *synthesizer, *blender);
#endif
	}
		
	// Download maps from GPU
#if WITH_OPENGL
	if (g_with_opengl) {
		blender->assignFromGL2CV(intermediateSize);
	}
#endif

	// Perform inpainting
	PROF_START("inpainting");
	cv::Mat3f color = inpaint(blender->get_color(), blender->get_inpaint_mask(), true);
	PROF_END("inpainting");

	// Downscale (when g_Precision != 1)
	PROF_START("downscale");
	resize(color, color, params_virtual.getSize());
	PROF_END("downscale");

	// Write regular output (activated by OutputFiles)
	if (!m_config.outfilenames.empty()) {
		PROF_START("write");
		write_color(m_config.outfilenames[virtualView], color, virtualFrame, params_virtual);
		PROF_END("write");
	}

	// Compute mask (activated by OutputMasks or MaskedOutputFiles)
	cv::Mat1b mask;
	if (!m_config.outmaskedfilenames.empty()) {
		mask = blender->get_validity_mask(m_config.validity_threshold);
		resize(mask, mask, params_virtual.getSize(), cv::INTER_NEAREST);
	}

	// Write masked output (MaskedOutputFiles)
	if (!m_config.outmaskedfilenames.empty()) {
		color.setTo(cv::Vec3f::all(0.5f), mask);
		write_color(m_config.outmaskedfilenames[virtualView], color, virtualFrame, params_virtual);
	}

#if WITH_OPENGL
	if (g_with_opengl) {
		auto FBO = RFBO::getInstance();
		FBO->free();
	}
#endif

	PROF_END("computeView");
}

std::unique_ptr<BlendedView> Pipeline::createBlender()
{
	switch (m_config.blending_method) {
	case BlendingMethod::simple:
		return std::make_unique<BlendedViewSimple>(m_config.blending_factor);
	case BlendingMethod::multispectral:
		return std::make_unique<BlendedViewMultiSpec>(m_config.blending_low_freq_factor, m_config.blending_high_freq_factor);
	default:
		throw std::logic_error("Unknown blending method");
	}
}

std::unique_ptr<SynthesizedView> Pipeline::createSynthesizer()
{
	switch (m_config.vs_method) {
	case ViewSynthesisMethod::triangles:
		return std::make_unique<SynthetisedViewTriangle>();
	default:
		throw std::logic_error("Unknown view synthesis method");
	}
}

std::unique_ptr<SpaceTransformer> Pipeline::createSpaceTransformer()
{
#if WITH_OPENGL
	if (g_with_opengl) {
		return std::make_unique<OpenGLTransformer>();
	}
#endif
	return std::make_unique<PUTransformer>();
}

void Pipeline::dumpMaps(View const& input_image, std::size_t input_idx, std::size_t virtual_idx, View const& synthesizer, View const& blender)
{
	cv::Mat3f rgb;
	cv::Mat3b color;
	cv::Mat1w depth;
	cv::Mat1w quality;
	cv::Mat1w validity; // triangle shape
	std::ostringstream filepath;

	cv::cvtColor(input_image.get_color(), rgb, cv::COLOR_YCrCb2BGR);
	rgb.convertTo(color, CV_8U, 255.);
	input_image.get_depth().convertTo(depth, CV_16U, 2000.);

	filepath.str(""); filepath << "dump-input-color-" << input_idx << "to" << virtual_idx << ".png"; cv::imwrite(filepath.str(), color);
	filepath.str(""); filepath << "dump-input-depth-" << input_idx << "to" << virtual_idx << "_x2000.png"; cv::imwrite(filepath.str(), depth);
	filepath.str(""); filepath << "dump-input-depth_mask-" << input_idx << "to" << virtual_idx << ".png"; cv::imwrite(filepath.str(), input_image.get_depth_mask());

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
