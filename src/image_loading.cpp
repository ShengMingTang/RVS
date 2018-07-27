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

namespace 
{
	auto const NaN = std::numeric_limits<float>::quiet_NaN();

	void read_raw(std::ifstream& stream, cv::Mat image)
	{
		CV_Assert(stream.good() && !image.empty() && image.isContinuous());
		stream.read(reinterpret_cast<char*>(image.data), image.size().area() * image.elemSize());
	}

	/**
	 * reads a color image in yuv 420 format
	 * @return the corresponding color image, normalized to [0, 1]
	 * */
	cv::Mat3f read_color_YUV(std::string filename, cv::Size size, int bit_depth, int frame) {
		auto type = CV_MAKETYPE(cvdepth_from_bit_depth(bit_depth), 1);
		cv::Mat y_channel(size, type);
		cv::Mat cb_channel(size / 2, type);
		cv::Mat cr_channel(size / 2, type);

		std::ifstream stream(filename, std::ios::binary);
		if (!stream.good())
			throw std::runtime_error("Failed to read raw YUV color file");
		stream.seekg(size.area() * y_channel.elemSize() * 3 / 2 * frame);
		read_raw(stream, y_channel);
		read_raw(stream, cb_channel);
		read_raw(stream, cr_channel);

		cv::resize(cb_channel, cb_channel, size, 0, 0, cv::INTER_CUBIC);
		cv::resize(cr_channel, cr_channel, size, 0, 0, cv::INTER_CUBIC);

		cv::Mat image(size, CV_MAKETYPE(cvdepth_from_bit_depth(bit_depth), 3));
		cv::Mat src[] = { y_channel, cr_channel, cb_channel };
		cv::merge(src, 3, image);
		
		image.convertTo(image, CV_32F, 1. / max_level(bit_depth));
		
		if (color_space == COLORSPACE_RGB)
			cv::cvtColor(image, image, CV_YCrCb2BGR);

		return image;
	}

	/**
	* reads a disparity map (VSRS format)
	* @return the corresponding depth map
	* */
	cv::Mat1f read_depth_YUV(std::string filename, cv::Size size, int bit_depth, float z_near, float z_far, int frame) {
		cv::Mat image(size, CV_MAKETYPE(cvdepth_from_bit_depth(bit_depth), 1));
		std::ifstream stream(filename, std::ios_base::binary);
		if (!stream.good())
			throw std::runtime_error("Failed to read raw YUV depth file");
		stream.seekg(size.area() * image.elemSize() * 3 / 2 * frame); // YUV 4:2:0 also for raw depth streams
		read_raw(stream, image);

		auto mask_depth = cv::Mat1b(image == 0);

		image.convertTo(image, CV_32F, 1. / max_level(bit_depth));

		cv::Mat1f depth = z_near / (z_near / z_far + image * (1.f - z_near / z_far));
		depth.setTo(NaN, mask_depth);
		return depth;
	}

	/**
	 * reads a color image in any format supported by opencv
	 * @return the corresponding color image, normalized to [0, 1]
	 * */
	cv::Mat3f read_color_RGB(std::string filename, cv::Size size, int bit_depth) {
		cv::Mat image = cv::imread(filename, cv::IMREAD_UNCHANGED);

		if (image.empty())
			throw std::runtime_error("Failed to read color file");		
		if (image.size() != size)
			throw std::runtime_error("Color file does not have the expected size");
		if (image.depth() != cvdepth_from_bit_depth(bit_depth))
			throw std::runtime_error("Color file has wrong bit depth");
		if (image.channels() != 3)
			throw std::runtime_error("Color file has wrong number of channels");

		image.convertTo(image, CV_32F, 1. / max_level(bit_depth));

		if (color_space == COLORSPACE_YUV)
			cv::cvtColor(image, image, CV_BGR2YCrCb);

		return image;
	}

	/**
	* read a depth map (in any format supported by opencv)
	* @return the corresponding depth map, normalized to [0, 1]
	* */
	cv::Mat read_depth_RGB(std::string filename, cv::Size size, int bit_depth) {
		cv::Mat image = cv::imread(filename, cv::IMREAD_UNCHANGED);

		if (image.empty())
			throw std::runtime_error("Failed to read depth file");
		if (image.size() != size)
			throw std::runtime_error("Depth file does not have the expected size");
		if (image.depth() != cvdepth_from_bit_depth(bit_depth))
			throw std::runtime_error("Depth file has the wrong bit depth");
		if (image.channels() != 1)
			throw std::runtime_error("Depth file has the wrong number of channels");
		
		image.convertTo(image, CV_32F);

		return image;
	}
}

int cvdepth_from_bit_depth(int bit_depth)
{
	if (bit_depth >= 1 && bit_depth <= 8)
		return CV_8U;
	else if (bit_depth >= 9 && bit_depth <= 16)
		return CV_16U;
	else throw std::invalid_argument("invalid raw image bit depth");
}

unsigned max_level(int bit_depth)
{
	return (1u << bit_depth) - 1u;
}

cv::Mat3f read_color(std::string filename, cv::Size size, int bit_depth, int frame) {
	if (filename.find(".yuv") != std::string::npos)
		return read_color_YUV(filename, size, bit_depth, frame);

	if (frame != 0)
		throw std::runtime_error("Readig multiple frames not (yet) supported for image files");

	return read_color_RGB(filename, size, bit_depth);
}

cv::Mat1f read_depth(std::string filename, cv::Size size, int bit_depth, float z_near, float z_far, int frame) {
	if (filename.find(".yuv") != std::string::npos)
		return read_depth_YUV(filename, size, bit_depth, z_near, z_far, frame);

	if (frame != 0)
		throw std::runtime_error("Readig multiple frames not (yet) supported for image files");

	return read_depth_RGB(filename, size, bit_depth);
}
