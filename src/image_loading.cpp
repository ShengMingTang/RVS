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

#include "image_loading.hpp"
#include "Config.hpp"

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"

#include <fstream>
#include <iostream>


template<typename T>
void read_raw(std::ifstream& stream, T* out, std::size_t length) {
	stream.read(reinterpret_cast<std::ifstream::char_type*>(out), length);
}

/**
 * import grey yuv image
 */
cv::Mat import_raw_mono(const std::string& yuv_filename, int width, int height, int bit_depth) {
	int type;
	switch (bit_depth) {
	case 8: type = CV_8U; break;
	case 16: type = CV_16U; break;
	default: throw std::invalid_argument("invalid raw image bit depth");
	}
	cv::Mat mat(height, width, type);
	int length = width * height * bit_depth / 8;
	std::ifstream yuv_stream(yuv_filename, std::ios_base::binary);
	read_raw(yuv_stream, mat.data, length);
	//imshow("depth",mat);
	//std::cout << mat.at<float>(0, 0) << std::endl;
	//cv::waitKey(0);
	return mat;
}
/**
 * import Yuv rgb interleaved
 * */
cv::Mat_<cv::Vec3b> import_rgb_interleaved(std::ifstream& yuv_stream, int width, int height) {
	cv::Size sz(width, height);
	cv::Mat_<uchar> rgb(sz);
	read_raw(yuv_stream, rgb.data, 3 * width*height);
	cv::Mat_<uchar> bgr;
	cv::cvtColor(rgb, bgr, CV_RGB2BGR);
	return bgr;
}
/**
 * import Yuv with scaling factor 420
 * */
cv::Mat_<cv::Vec3b> import_ycbcr420(std::ifstream& yuv_stream, int width, int height) {
	cv::Size sz(width, height);
	cv::Size sub_sz(width / 2, height / 2);

	cv::Mat_<uchar> y_channel(sz);
	read_raw(yuv_stream, y_channel.data, width*height);

	cv::Mat_<uchar> cb_channel(sub_sz);
	read_raw(yuv_stream, cb_channel.data, width*height / 4);
	cv::resize(cb_channel, cb_channel, sz, 0, 0, cv::INTER_CUBIC);

	cv::Mat_<uchar> cr_channel(sub_sz);
	read_raw(yuv_stream, cr_channel.data, width*height / 4);
	cv::resize(cr_channel, cr_channel, sz, 0, 0, cv::INTER_CUBIC);

	cv::Mat_<cv::Vec3b> ycrcb;
	std::vector<cv::Mat> src{ y_channel, cr_channel, cb_channel };
	cv::merge(src, ycrcb);

	cv::Mat_<cv::Vec3b> bgr;
	cv::cvtColor(ycrcb, bgr, CV_YCrCb2BGR);

	return ycrcb;
}
/**
 * import Yuv rgb planar
 * */
cv::Mat_<cv::Vec3b> import_rgb_planar(std::ifstream& yuv_stream, int width, int height) {
	cv::Size sz(width, height);

	cv::Mat_<uchar> r_channel(sz);
	read_raw(yuv_stream, r_channel.data, width*height);

	cv::Mat_<uchar> g_channel(sz);
	read_raw(yuv_stream, g_channel.data, width*height);

	cv::Mat_<uchar> b_channel(sz);
	read_raw(yuv_stream, b_channel.data, width*height);

	cv::Mat_<cv::Vec3b> bgr(sz);
	int from_to[] = { 0, 0, 0, 1, 0, 2 };
	std::vector<cv::Mat> dst{ bgr };
	std::vector<cv::Mat> src{ b_channel, g_channel, r_channel };
	cv::mixChannels(src, dst, from_to, 3);

	return bgr;
}


/**
 * reads a disparity map (VSRS format) 
 * @return the corresponding depth map
 * */
cv::Mat read_depth_yuv(std::string filename, cv::Size s, float z_near, float z_far) {
	const int width = s.width;
	const int height = s.height;
	cv::Mat img;

	const float alpha = -255.0f*z_near / (z_far - z_near);
	const float beta = 255.0f*z_near*z_far / (z_far - z_near);

	cv::Mat img2 = import_raw_mono(filename, width, height, 16);

	img2.convertTo(img2, CV_32F);
	img = beta / (img2 / 255.0 - alpha);

	for (int x = 0; x < img.cols; ++x)
		for (int y = 0; y < img.rows; ++y)
			if (img2.at<float>(y, x) == 0.0)
				img.at<float>(y, x) = 0.0;
	img.convertTo(img, CV_32F);
	return img;
}

/**
 * read a depth map (in any format supported by opencv)
 * @return the corresponding CV_8U depth map
 * */
cv::Mat read_depth_RGB(std::string filename, cv::Size s) {
	cv::Mat img = cv::imread(filename, cv::IMREAD_ANYDEPTH);
	if (img.empty()) {
		std::cout << "Depth file " << filename << " could not be read." << std::endl;
		return img;
	}
	if (img.size() != s)
		std::cout << "Depth file " << filename << " has not the expected size." << std::endl;
	img.convertTo(img, CV_32F);
	return img;
}
cv::Mat read_depth(std::string filename, cv::Size s, float z_near, float z_far) {
	if (filename.find(".yuv") != std::string::npos)
		return read_depth_yuv(filename, s, z_near, z_far);
	return read_depth_RGB(filename, s);
}


/**
 * reads a color image in yuv 420  format
 * @return the corresponding color image in CV_8UC3
 * */
cv::Mat read_color_yuv(std::string filename, cv::Size s) {
	const int width = s.width;
	const int height = s.height;
	cv::Mat img;

	std::ifstream yuv_stream(filename, std::ios_base::binary);
	img = import_ycbcr420(yuv_stream, width, height);

	if (img.empty())
		std::cout << filename << " not found" << std::endl;

	if (color_space == COLORSPACE_RGB)
		cv::cvtColor(img, img, CV_YCrCb2BGR);
	
	return img;
}
/**
 * reads a color image in any format supported by opencv 
 * @return the corresponding color image in CV_8UC3
 * */
cv::Mat read_color_RGB(std::string filename, cv::Size s) {
	cv::Mat img = cv::imread(filename, CV_LOAD_IMAGE_COLOR);

	if (img.empty()) {
		std::cout << "Color file " << filename << " could not be read." << std::endl;
		return img;
	}
	if (img.size() != s)
		std::cout << "Color file " << filename << " has not the expected size." << std::endl;

	if (color_space == COLORSPACE_YUV)
		cv::cvtColor(img, img, CV_BGR2YCrCb);

	return img;
}
cv::Mat read_color(std::string filename, cv::Size s) {
	cv::Mat img;
	if (filename.find(".yuv") != std::string::npos) {
		img = read_color_yuv(filename, s);
	}
	else {
		img = read_color_RGB(filename, s);
	}
	return img;
}
