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

namespace
{
	void write_raw(std::ofstream& stream, cv::Mat image)
	{
		CV_Assert(stream.good() && !image.empty() && image.isContinuous());
		stream.write(reinterpret_cast<char const*>(image.data), image.size().area() * image.elemSize());
	}

	void write_color_YUV(std::string filename, cv::Mat3f image, int bit_depth, int frame)
	{
		if (g_color_space == ColorSpace::RGB)
			cv::cvtColor(image, image, CV_BGR2YCrCb);

		cv::Mat ycbcr;
		image.convertTo(ycbcr, cvdepth_from_bit_depth(bit_depth), max_level(bit_depth));

		std::ofstream stream(filename, frame 
			? std::ios::binary | std::ios::app
			: std::ios::binary);
		if (!stream.is_open())
			throw std::runtime_error("Failed to open YUV output image");

		cv::Mat dst[3];
		cv::split(ycbcr, dst);

		cv::resize(dst[1], dst[1], cv::Size(), 0.5, 0.5, cv::INTER_CUBIC);
		cv::resize(dst[2], dst[2], cv::Size(), 0.5, 0.5, cv::INTER_CUBIC);

		write_raw(stream, dst[0]);
		write_raw(stream, dst[2]);
		write_raw(stream, dst[1]);
	}

	void write_color_RGB(std::string filename, cv::Mat3f image, int bit_depth)
	{
		if (g_color_space == ColorSpace::YUV)
			cv::cvtColor(image, image, CV_YCrCb2BGR);

		cv::Mat out;
		image.convertTo(out, cvdepth_from_bit_depth(bit_depth), max_level(bit_depth));

		cv::imwrite(filename, out);
	}
}

void write_color(std::string filepath, cv::Mat3f image, int frame, Parameters const& parameters)
{
	// Add padding
	if (parameters.getPaddedSize() != parameters.getSize()) {
		auto padded = cv::Mat3f(parameters.getPaddedSize(), cv::Vec3f::all(0.f));
		padded(parameters.getCropRegion()) = image;
		image = padded;
	}

	// Write padded image
	if (filepath.find(".yuv") != std::string::npos) {
		write_color_YUV(filepath, image, parameters.getColorBitDepth(), frame);
	}
	else if (frame == 0) {
		write_color_RGB(filepath, image, parameters.getColorBitDepth() <= 8 ? 8 : 16);
	}
	else {
		throw std::runtime_error("Writing multiple frames as images not (yet) supported");
	}

}
