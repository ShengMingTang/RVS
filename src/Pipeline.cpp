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
#include "BlendedView.hpp"
#include "SynthesizedView.hpp"
#include "inpainting.hpp"

#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>

#include <opencv2/imgproc.hpp>

#if WITH_OPENGL
#include "helpersGL.hpp"
#include "RFBO.hpp"
#endif

// [SM] begin //
#if WITH_OPENGL
bool is_opengl = true;
#else
bool is_opengl = false;
#endif
// [SM] end //

namespace rvs
{
	Pipeline::Pipeline()
	{
#ifndef NDEBUG
		cv::setBreakOnError(true);
#endif
	}

	void Pipeline::execute()
	{
		// [SM] begin //
		if(!is_opengl) {
			std::cout << "not using OpenGL" << std::endl;
			exit(-1);
		}
		// [SM] end //
		for (auto virtualFrame = 0; virtualFrame < getConfig().number_of_output_frames; ++virtualFrame) {
			auto inputFrame = getConfig().start_frame + virtualFrame;
			// [SM] from //
			// if (getConfig().number_of_output_frames > 1) {
			// 	std::cout << std::string(5, '=') << " FRAME " << inputFrame << ' ' << std::string(80, '=') << std::endl;
			// }
			// [SM] to //
			// [SM] end //
			for (auto virtualView = 0u; virtualView != getConfig().VirtualCameraNames.size(); ++virtualView) {
				computeView(inputFrame, virtualFrame, virtualView);
			}
		}
	}

	bool Pipeline::wantColor()
	{
		return false;
	}

	bool Pipeline::wantMaskedColor()
	{
		return false;
	}

	bool Pipeline::wantMask()
	{
		return false;
	}

	bool Pipeline::wantDepth()
	{
		return false;
	}

	bool Pipeline::wantMaskedDepth()
	{
		return false;
	}

	void Pipeline::saveColor(cv::Mat3f, int, int, Parameters const&)
	{
		throw std::logic_error(std::string(__func__) + "not implemented");
	}

	void Pipeline::saveMaskedColor(cv::Mat3f, int, int, Parameters const&)
	{
		throw std::logic_error(std::string(__func__) + "not implemented");
	}

	void Pipeline::saveMask(cv::Mat1b, int, int, Parameters const&)
	{
		throw std::logic_error(std::string(__func__) + " not implemented");
	}

	void Pipeline::saveDepth(cv::Mat1f, int, int, Parameters const&)
	{
		throw std::logic_error(std::string(__func__) + " not implemented");
	}

	void Pipeline::saveMaskedDepth(cv::Mat1f, cv::Mat1b, int, int, Parameters const&)
	{
		throw std::logic_error(std::string(__func__) + " not implemented");
	}

	void Pipeline::onIntermediateSynthesisResult(int, int, int, int, SynthesizedView const&) {}

	void Pipeline::onIntermediateBlendingResult(int, int, int, int, BlendedView const&) {}

	void Pipeline::onFinalBlendingResult(int, int, int, BlendedView const&) {}

	auto getExtendedIndex(int outputFrameIndex, int numberOfInputFrames) {

		if (numberOfInputFrames <= 0) {
			throw std::runtime_error("Cannot extend frame index with zero input frames");
		}
		const auto frameGroupIndex = outputFrameIndex / numberOfInputFrames;
		const auto frameRelativeIndex = outputFrameIndex % numberOfInputFrames;
		return frameGroupIndex % 2 != 0 ? numberOfInputFrames - frameRelativeIndex - 1
			: frameRelativeIndex;

	}

	void Pipeline::computeView(int inputFrame, int virtualFrame, int virtualView)
	{
		// Virtual view parameters for this frame and view
		auto params_virtual = getConfig().params_virtual[virtualView];
		if (!getConfig().pose_trace.empty()) {
			auto pose = getConfig().pose_trace[inputFrame];
			params_virtual.setPosition(params_virtual.getPosition() + pose.position);
			params_virtual.setRotation(pose.rotation);
			// [SM] from //
			// std::cout << "Pose: " << params_virtual.getPosition() << ' ' << params_virtual.getRotation() << std::endl;
			// [SM] to //
			// [SM] end //
		}

		// Initialize OpenGL frame buffer objects
#if WITH_OPENGL
		auto intermediateSize = cv::Size(
			int(detail::g_rescale*params_virtual.getSize().width),
			int(detail::g_rescale*params_virtual.getSize().height));
		if (g_with_opengl) {
			auto FBO = opengl::RFBO::getInstance();
			FBO->init(intermediateSize);
		}
#endif

		// Setup a view blender
		auto blender = createBlender(virtualView);

		// Partial setup of a space transformer
		auto spaceTransformer = createSpaceTransformer(virtualView);
		spaceTransformer->set_targetPosition(&params_virtual);

		// For each input view
		for (auto inputView = 0u; inputView != getConfig().InputCameraNames.size(); ++inputView) {
			// [SM] from //
			// std::cout << getConfig().InputCameraNames[inputView] << " => " << getConfig().VirtualCameraNames[virtualView] << std::endl;
			// [SM] to //
			// [SM] end //
			auto const& params_real = getConfig().params_real[inputView];

			// Complete setup of space transformer
			spaceTransformer->set_inputPosition(&params_real);

			// Setup a view synthesizer
			auto synthesizer = createSynthesizer(inputView, virtualView);
			synthesizer->setSpaceTransformer(spaceTransformer.get());

			//posetrace longer that input view: back and forwards in the input video
			int frame_to_load = getExtendedIndex(inputFrame, getConfig().number_of_frames);
			// [SM] from //
			// std::cout << "loading... " << frame_to_load << std::endl;
			// Load the input image
			// auto inputImage = loadInputView(frame_to_load, inputView, params_real);
			// [SM] to //
			// first time, load
			std::shared_ptr<View> inputImage = nullptr;
			if(getConfig().number_of_frames != 1) {
				std::cout << "exit because this version only supports number of input frame == 1" << std::endl;
				exit(-1);
			}
			if(m_lastInputImages.size() != getConfig().InputCameraNames.size()) {
				std::cout << "loading... " << frame_to_load << std::endl;
				inputImage = loadInputView(frame_to_load, inputView, params_real);
				m_lastInputImages.push_back(inputImage);
			}
			else {
				inputImage = m_lastInputImages[inputView];
			}
			// [SM] end //

			// Start OpenGL instrumentation (if any)
#if WITH_OPENGL
			if (g_with_opengl) {
				opengl::rd_start_capture_frame();
			}
#endif
			// Synthesize view
			synthesizer->compute(*inputImage);
			onIntermediateSynthesisResult(inputFrame, inputView, virtualFrame, virtualView, *synthesizer);

			// Blend with previous results
			blender->blend(*synthesizer);
			onIntermediateBlendingResult(inputFrame, inputView, virtualFrame, virtualView, *blender);

			// End OpenGL instrumentation (if any)
#if WITH_OPENGL
			if (g_with_opengl) {
				opengl::rd_end_capture_frame();
			}
#endif
		}

		onFinalBlendingResult(inputFrame, virtualFrame, virtualView, *blender);

		// Download maps from GPU
#if WITH_OPENGL
		if (g_with_opengl) {
			blender->assignFromGL2CV(intermediateSize);
		}
#endif

		// Perform inpainting
		cv::Mat3f color = detail::inpaint(blender->get_color(), blender->get_inpaint_mask(), true);

		// Downscale (when g_Precision != 1)
		resize(color, color, params_virtual.getSize());

		// Write regular output (activated by OutputFiles)
		if (wantColor()) {
			saveColor(color, virtualFrame, virtualView, params_virtual);
		}

		// Compute mask (activated by OutputMasks or MaskedOutputFiles or MaskedDepthOutputFiles)
		cv::Mat1b mask;
		if (wantMask() || wantMaskedColor() || wantMaskedDepth()) {
			mask = blender->get_validity_mask(getConfig().validity_threshold);
			resize(mask, mask, params_virtual.getSize(), cv::INTER_NEAREST);
		}

		// Write mask (activated by OutputMasks)
		if (wantMask()) {
			saveMask(mask, virtualFrame, virtualView, params_virtual);
		}

		// Write masked output (activated by MaskedOutputFiles)
		if (wantMaskedColor()) {
			color.setTo(cv::Vec3f::all(0.5f), mask);
			saveMaskedColor(color, virtualFrame, virtualView, params_virtual);
		}

		// Write depth maps (activated by DepthOutputFiles)
		if (wantDepth()) {
			auto depth = blender->get_depth();
			resize(depth, depth, params_virtual.getSize());
			saveDepth(depth, virtualFrame, virtualView, params_virtual);
		}

		// Write masked depth maps (activated by MaskedDepthOutputFiles)
		if (wantMaskedDepth()) {
			auto depth = blender->get_depth();
			resize(depth, depth, params_virtual.getSize());
			saveMaskedDepth(depth, mask, virtualFrame, virtualView, params_virtual);
		}

#if WITH_OPENGL
		if (g_with_opengl) {
			auto FBO = opengl::RFBO::getInstance();
			FBO->free();
		}
#endif
	}

	std::unique_ptr<BlendedView> Pipeline::createBlender(int)
	{
		if (getConfig().blending_method == BlendingMethod::simple) {
			return std::unique_ptr<BlendedView>(new BlendedViewSimple(getConfig().blending_factor));
		}

		if (getConfig().blending_method == BlendingMethod::multispectral) {
			return std::unique_ptr<BlendedView>(new BlendedViewMultiSpec(getConfig().blending_low_freq_factor, getConfig().blending_high_freq_factor));
		}

		std::ostringstream what;
		what << "Unknown view blending method \"" << getConfig().blending_method << "\"";
		throw std::runtime_error(what.str());
	}

	std::unique_ptr<SynthesizedView> Pipeline::createSynthesizer(int, int)
	{
		if (getConfig().vs_method == ViewSynthesisMethod::triangles) {
			return std::unique_ptr<SynthesizedView>(new SynthetisedViewTriangle);
		}

		std::ostringstream what;
		what << "Unknown view synthesis method \"" << getConfig().vs_method << "\"";
		throw std::runtime_error(what.str());
	}

	std::unique_ptr<SpaceTransformer> Pipeline::createSpaceTransformer(int)
	{
#if WITH_OPENGL
		if (g_with_opengl) {
			return std::unique_ptr<SpaceTransformer>(new OpenGLTransformer);
		}
#endif
		return std::unique_ptr<SpaceTransformer>(new GenericTransformer);
	}
}