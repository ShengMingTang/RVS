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

#include "image_loading.hpp"
#include "Config.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <limits>

namespace rvs
{
	namespace
	{
		using detail::ColorSpace;
		using detail::g_color_space;

		void read_raw(std::ifstream& stream, cv::Mat image)
		{
			CV_Assert(stream.good() && !image.empty() && image.isContinuous());
			stream.read(reinterpret_cast<char*>(image.data), image.size().area() * image.elemSize());
		}

		cv::Mat read_color_YUV(std::string filepath, int frame, Parameters const& parameters) {
			auto size = parameters.getPaddedSize();
			auto bit_depth = parameters.getColorBitDepth();
			auto type = CV_MAKETYPE(cvdepth_from_bit_depth(bit_depth), 1);
			cv::Mat y_channel(size, type);
			cv::Mat u_channel(size / 2, type);
			cv::Mat v_channel(size / 2, type);

			std::ifstream stream(filepath, std::ios::binary);
			if (!stream.good()) {
				std::ostringstream what;
				what << "Failed to read raw YUV color file \"" << filepath << "\"";
				throw std::runtime_error(what.str());
			}

			stream.seekg(size.area() * y_channel.elemSize() * 3 / 2 * frame);
			read_raw(stream, y_channel);
			read_raw(stream, u_channel);
			read_raw(stream, v_channel);

			cv::resize(u_channel, u_channel, size, 0, 0, cv::INTER_CUBIC);
			cv::resize(v_channel, v_channel, size, 0, 0, cv::INTER_CUBIC);

			cv::Mat image(size, CV_MAKETYPE(cvdepth_from_bit_depth(bit_depth), 3));
			cv::Mat src[] = { y_channel, u_channel, v_channel };
			cv::merge(src, 3, image);
			return image;
		}

		cv::Mat read_depth_YUV(std::string filepath, int frame, Parameters const& parameters) {
			auto size = parameters.getPaddedSize();
			auto bit_depth = parameters.getDepthBitDepth();
			cv::Mat image(size, CV_MAKETYPE(cvdepth_from_bit_depth(bit_depth), 1));
			std::ifstream stream(filepath, std::ios_base::binary);
			if (!stream.good()) {
				std::ostringstream what;
				what << "Failed to read raw YUV depth file \"" << filepath << "\"";
				throw std::runtime_error(what.str());
			}
			
			switch (parameters.getDepthColorFormat()) {
			case ColorFormat::YUV420:
				stream.seekg(size.area() * image.elemSize() * 3 / 2 * frame);
				break;
			case ColorFormat::YUV400:
				stream.seekg(size.area() * image.elemSize() * frame);
				break;
			default:
				throw std::logic_error("Unknown depth map color format");
			}

			read_raw(stream, image);
			return image;
		}

		cv::Mat read_color_RGB(std::string filepath, Parameters const& parameters) {
			cv::Mat image = cv::imread(filepath, cv::IMREAD_UNCHANGED);

			if (image.empty())
				throw std::runtime_error("Failed to read color file");
			if (image.size() != parameters.getPaddedSize())
			{
				//throw std::runtime_error("Color file does not have the expected size");
				std::cout << "Color file does not have the expected size" <<std::endl;
				cv::resize(image, image, parameters.getPaddedSize());
			}
			if (image.depth() != cvdepth_from_bit_depth(parameters.getColorBitDepth()))
			{
				std::cout << image.depth() << std::endl;
				throw std::runtime_error("Color file has wrong bit depth");
			}
			if (image.channels() != 3)
			{
				if (image.channels() == 4)
				{
					cv::Mat dst[4];
					cv::split(image, dst);
					cv::merge(dst, 3, image);
				}
				else
					throw std::runtime_error("Color file has wrong number of channels");
			}

			return image;
		}

		cv::Mat read_depth_RGB(std::string filepath, Parameters const& parameters) {
			cv::Mat image = cv::imread(filepath, cv::IMREAD_UNCHANGED);

			if (image.empty())
				throw std::runtime_error("Failed to read depth file");
			if (image.size() != parameters.getPaddedSize())
				//throw std::runtime_error("Depth file does not have the expected size");
				cv::resize(image, image, parameters.getPaddedSize(),0.0,0.0,cv::INTER_NEAREST);
			//if (image.depth() != cvdepth_from_bit_depth(parameters.getDepthBitDepth()))
			//	throw std::runtime_error("Depth file has the wrong bit depth");
			if (image.channels() != 1)
			{
				std::cout << "Depth file has the wrong number of channels" <<std::endl;
				cv::Mat dst[3];
				cv::split(image, dst);
				image = dst[2];
			}
			return image;
		}
	}

	int cvdepth_from_bit_depth(int bit_depth)
	{
		if (bit_depth >= 1 && bit_depth <= 8)
			return CV_8U;
		else if (bit_depth >= 9 && bit_depth <= 16)
			return CV_16U;
		else if (bit_depth == 32)
			return CV_32F;
		else throw std::invalid_argument("invalid raw image bit depth");
	}

	unsigned max_level(int bit_depth)
	{
		assert(bit_depth > 0 && bit_depth <= 16);
		return (1u << bit_depth) - 1u;
	}

	cv::Mat3f read_color(std::string filepath, int frame, Parameters const& parameters)
	{
		// Load the image
		cv::Mat image;
		ColorSpace color_space;
		if (filepath.substr(filepath.size() - 4, 4) == ".yuv") {
			image = read_color_YUV(filepath, frame, parameters);
			color_space = ColorSpace::YUV;
		}
		else if (frame == 0) {
			image = read_color_RGB(filepath, parameters);
			color_space = ColorSpace::RGB;
		}
		else {
			throw std::runtime_error("Readig multiple frames not (yet) supported for image files");
		}

		// Crop out padded regions
		if (parameters.getPaddedSize() != parameters.getSize()) {
			image = image(parameters.getCropRegion()).clone();
		}

		// Normalize to [0, 1]
		cv::Mat3f color;
		if (image.depth() == CV_32F) {
			color = image;
		}
		else {
			image.convertTo(color, CV_32F, 1. / max_level(parameters.getColorBitDepth()));
		}

		// Color space conversion
		if (color_space == ColorSpace::YUV && g_color_space == ColorSpace::RGB) {
			cv::cvtColor(color, color, cv::COLOR_YUV2BGR);
		}
		else if (color_space == ColorSpace::RGB && g_color_space == ColorSpace::YUV) {
			cv::cvtColor(color, color, cv::COLOR_BGR2YUV);
		}

		return color;
	}

	cv::Mat1f read_depth(std::string filepath, int frame, Parameters const& parameters)
	{
		// Load the image
		cv::Mat image;
		if (filepath.substr(filepath.size() - 4, 4) == ".yuv") {
			image = read_depth_YUV(filepath, frame, parameters);
		}
		else if (frame == 0) {
			image = read_depth_RGB(filepath, parameters);
		}
		else {
			//throw std::runtime_error("Readig multiple frames not (yet) supported for image files");
			std::cout << "Reading multiple frames not (yet) supported for image files. Reading first frame" << std::endl;
			image = read_depth_RGB(filepath, parameters);
		}

		// Crop out padded regions
		if (parameters.getPaddedSize() != parameters.getSize()) {
			image = image(parameters.getCropRegion()).clone();
		}

		// Do not manipulate floating-point depth maps (e.g. OpenEXR)
		if (image.depth() == CV_32F) {
			return image;
		}

		// Normalize to [0, 1]
		cv::Mat1f depth;
		image.convertTo(depth, CV_32F, 1. / max_level(parameters.getDepthBitDepth()));

		// 1000 is for 'infinitly far'
		auto near = parameters.getDepthRange()[0];
		auto far = parameters.getDepthRange()[1];
		if (far >= 1000.f) {
			depth = near / depth;
		}
		else {
			depth = far * near / (near + depth * (far - near));
		}

		if (parameters.hasInvalidDepth()) {
			// Level 0 is for 'invalid'
			// Mark invalid pixels as NaN
			auto const NaN = std::numeric_limits<float>::quiet_NaN();
			depth.setTo(NaN, image == 0);
		}

		return depth;
	}
	PolynomialDepth read_polynomial_depth(std::string filepath, int frame, Parameters const& parameters)
	{
		// Load the image
		cv::Mat image;
		PolynomialDepth pd;
		std::string ext = filepath.substr(filepath.size() - 4, 4);
		if (frame == 0 && ext == ".exr") {
			std::array<cv::Mat1f,20> polynomial;
			for (int i = 0; i < 20; ++i) {
				std::stringstream ss;
				ss << i;
				size_t pos = filepath.find('*');
				std::string f = filepath.substr(0, pos) + ss.str() + filepath.substr(pos+1, filepath.size());
				cv::Mat1f p = cv::imread(f, cv::IMREAD_UNCHANGED);
			//	cv::GaussianBlur(p1,p1,cv::Size(11,11),3);
                polynomial[i] = p;
			}
			pd = PolynomialDepth(polynomial);
		}
		else if (ext == ".yuv") {
			std::array<cv::Mat1f, 20> polynomial;
			for (int i = 0; i < 20; ++i) {
				std::stringstream ss;
				ss << i;
				size_t pos = filepath.find('*');
				std::string f = filepath.substr(0, pos) + ss.str() + filepath.substr(pos + 1, filepath.size());
				cv::Mat depth = read_depth_YUV(f, frame, parameters);
				cv::Mat1f p;
				depth.convertTo(p, CV_32F, 1. / max_level(parameters.getDepthBitDepth()));
				if (i != 9 && i != 19)
					p = (parameters.getMultiDepthRange()[1] - parameters.getMultiDepthRange()[0]) * p + parameters.getMultiDepthRange()[0];
				else if (i == 9)
					p = (parameters.getDepthRange()[0] + (parameters.getDepthRange()[1] - parameters.getDepthRange()[0]) * p) * parameters.getFocal()[0] / (parameters.getDepthRange()[1] * parameters.getDepthRange()[0]);
				polynomial[i] = p;
			}
			pd = PolynomialDepth(polynomial);
		}
		else {
			throw std::runtime_error("Readig multiple frames not (yet) supported for image files");
		}

		return pd;
	}
}
