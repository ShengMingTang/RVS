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

		cv::Mat1f depth = (z_far * z_near) / (z_near + image * (z_far - z_near));
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
