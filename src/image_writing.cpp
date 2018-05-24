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

Author  : Bart Kroon
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

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

	void write_color_YUV(std::string filename, cv::Mat3f image, int bit_depth)
	{
		if (color_space == COLORSPACE_RGB)
			cv::cvtColor(image, image, CV_BGR2YCrCb);

		cv::Mat ycbcr;
		image.convertTo(ycbcr, cvdepth_from_bit_depth(bit_depth), max_level(bit_depth));

		std::ofstream stream(filename, std::ios::binary);
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
		if (color_space == COLORSPACE_YUV)
			cv::cvtColor(image, image, CV_YCrCb2BGR);

		cv::Mat out;
		image.convertTo(out, cvdepth_from_bit_depth(bit_depth), max_level(bit_depth));

		cv::imwrite(filename, out);
	}
}

void write_color(std::string filename, cv::Mat3f image, int bit_depth)
{
	if (filename.find("yuv") != std::string::npos)
		write_color_YUV(filename, image, bit_depth);
	else
		write_color_RGB(filename, image, bit_depth);
}
