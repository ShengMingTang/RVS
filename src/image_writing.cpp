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

#include "image_writing.hpp"
#include "image_loading.hpp"
#include "Config.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

namespace rvs
{
	namespace
	{
		using detail::ColorSpace;
		using detail::g_color_space;

		void write_raw(std::ofstream& stream, cv::Mat image)
		{
			CV_Assert(stream.good() && !image.empty() && image.isContinuous());
			stream.write(reinterpret_cast<char const*>(image.data), image.size().area() * image.elemSize());
		}

		void write_color_YUV(std::string filepath, cv::Mat image, int frame)
		{
			std::ofstream stream(filepath, frame
				? std::ios::binary | std::ios::app
				: std::ios::binary);
			if (!stream.is_open())
				throw std::runtime_error("Failed to open YUV output image");

			cv::Mat dst[3];
			cv::split(image, dst);

			cv::resize(dst[1], dst[1], cv::Size(), 0.5, 0.5, cv::INTER_CUBIC);
			cv::resize(dst[2], dst[2], cv::Size(), 0.5, 0.5, cv::INTER_CUBIC);

			write_raw(stream, dst[0]);
			write_raw(stream, dst[2]);
			write_raw(stream, dst[1]);
		}

		void write_depth_YUV(std::string filepath, cv::Mat image, int frame, Parameters const& parameters)
		{
			std::ofstream stream(filepath, frame
				? std::ios::binary | std::ios::app
				: std::ios::binary);
			if (!stream.is_open())
				throw std::runtime_error("Failed to open YUV output image");

			auto bit_depth = parameters.getDepthBitDepth();
			auto neutral = bit_depth == 32
				? 0.5
				: 0.5 * (1 + max_level(bit_depth));
			auto chroma = cv::Mat(image.size() / 2, image.type(), cv::Scalar::all(neutral));

			write_raw(stream, image);
			write_raw(stream, chroma);
			write_raw(stream, chroma);
		}

		void write_mask_YUV(std::string filepath, cv::Mat1b image, int frame)
		{
			std::ofstream stream(filepath, frame
				? std::ios::binary | std::ios::app
				: std::ios::binary);
			if (!stream.is_open())
				throw std::runtime_error("Failed to open YUV output image");

			auto chroma = cv::Mat1b(image.size() / 2, 128);

			write_raw(stream, image);
			write_raw(stream, chroma);
			write_raw(stream, chroma);
		}
	}

	void write_color(std::string filepath, cv::Mat3f color, int frame, Parameters const& parameters)
	{
		// Color space conversion
		auto color_space = filepath.substr(filepath.size() - 4, 4) == ".yuv"
			? ColorSpace::YUV
			: ColorSpace::RGB;
		if (g_color_space == ColorSpace::YUV && color_space == ColorSpace::RGB) {
			cv::cvtColor(color, color, cv::COLOR_YCrCb2BGR);
		}
		else if (g_color_space == ColorSpace::RGB && color_space == ColorSpace::YUV) {
			cv::cvtColor(color, color, cv::COLOR_BGR2YCrCb);
		}

		// Quantization
		auto bit_depth = parameters.getColorBitDepth();
		cv::Mat image;
		if (bit_depth == 32) {
			image = color;
		}
		else {
			color.convertTo(image, cvdepth_from_bit_depth(bit_depth), max_level(bit_depth));
		}

		// Pad image
		if (parameters.getPaddedSize() != parameters.getSize()) {
			auto padded = cv::Mat(parameters.getPaddedSize(), image.type(), cv::Scalar::all(0.));
			padded(parameters.getCropRegion()) = image;
			image = padded;
		}

		// Write the image
		if (color_space == ColorSpace::YUV) {
			write_color_YUV(filepath, image, frame);
		}
		else if (frame == 0) {
			cv::imwrite(filepath, image);
		}
		else {
			throw std::runtime_error("Writing multiple frames not (yet) supported for image files");
		}
	}

	void write_depth(std::string filepath, cv::Mat1f depth, int frame, Parameters const& parameters)
	{
		auto bit_depth = parameters.getDepthBitDepth();

		cv::Mat1f depth_tmp = depth.clone(); //clone to avoid modifying the input depth

		cv::Mat image;
		if (bit_depth == 32) {
			// Do not manipulate floating-point depth maps (e.g. OpenEXR)
			image = depth_tmp;
			std::cout << "### open exr   ###" << std::endl;
		}
		else {
			auto near = parameters.getDepthRange()[0];
			auto far = parameters.getDepthRange()[1];

			// 1000 is for 'infinitly far'
			if (far >= 1000.f) {
				depth_tmp = near / depth_tmp;
			}
			else {
				depth_tmp = (far * near / depth_tmp - near) / (far - near);
			}
			depth_tmp.convertTo(image, cvdepth_from_bit_depth(bit_depth), max_level(bit_depth));
		}

		// Pad image
		if (parameters.getPaddedSize() != parameters.getSize()) {
			auto padded = cv::Mat(parameters.getPaddedSize(), image.type(), cv::Scalar::all(0.));
			padded(parameters.getCropRegion()) = image;
			image = padded;
		}

		// Save the image
		if (filepath.substr(filepath.size() - 4, 4) == ".yuv") {
			write_depth_YUV(filepath, image, frame, parameters);
		}
		else if (frame == 0) {
			cv::imwrite(filepath, image);
		}
		else {
			throw std::runtime_error("Writing  multiple frames not (yet) supported for image files");
		}
	}

	void write_maskedDepth(std::string filepath, cv::Mat1f depth, cv::Mat1b mask, int frame, Parameters const& parameters)
	{
		auto bit_depth = parameters.getDepthBitDepth();

		cv::Mat1f depth_tmp = depth.clone(); //clone to avoid modifying the input depth

		cv::Mat image;
		if (bit_depth == 32) {
			// Do not manipulate floating-point depth maps (e.g. OpenEXR)
			image = depth_tmp;
		}
		else {
			auto near = parameters.getDepthRange()[0];
			auto far = parameters.getDepthRange()[1];

			// 1000 is for 'infinitly far'
			if (far >= 1000.f) {
				depth_tmp = near / depth_tmp;
			}
			else {
				depth_tmp = (far * near / depth_tmp - near) / (far - near);
			}
			depth_tmp.setTo(0.f, mask);
			depth_tmp.convertTo(image, cvdepth_from_bit_depth(bit_depth), max_level(bit_depth));
		}


		// Pad image
		if (parameters.getPaddedSize() != parameters.getSize()) {
			auto padded = cv::Mat(parameters.getPaddedSize(), image.type(), cv::Scalar::all(0.));
			padded(parameters.getCropRegion()) = image;
			image = padded;
		}

		// Save the image
		if (filepath.substr(filepath.size() - 4, 4) == ".yuv") {
			write_depth_YUV(filepath, image, frame, parameters);
		}
		else if (frame == 0) {
			cv::imwrite(filepath, image);
		}
		else {
			throw std::runtime_error("Writing  multiple frames not (yet) supported for image files");
		}
	}

	void write_mask(std::string filepath, cv::Mat1b mask, int frame, Parameters const& parameters)
	{
		mask.setTo(255, mask);

		// Pad image
		if (parameters.getPaddedSize() != parameters.getSize()) {
			auto padded = cv::Mat(parameters.getPaddedSize(), mask.type(), cv::Scalar::all(0.));
			padded(parameters.getCropRegion()) = mask;
			mask = padded;
		}

		// Save the image
		if (filepath.substr(filepath.size() - 4, 4) == ".yuv") {
			write_mask_YUV(filepath, mask, frame);
		}
		else if (frame == 0) {
			cv::imwrite(filepath, mask);
		}
		else {
			throw std::runtime_error("Writing multiple frames not (yet) supported for image files");
		}
	}
}