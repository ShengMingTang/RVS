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

#include "image_writing.hpp"
#include "Config.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"



template<typename T>
void write_raw(std::ofstream& stream, const T* in, std::size_t length) {
	stream.write(reinterpret_cast<const std::ofstream::char_type*>(in), length);
}
/**
 * Export image in YUV 420
 * */
void export_ycbcr420(std::ofstream& yuv_stream, const cv::Mat& ycrcb) {
	cv::Mat dst[3];
	cv::split(ycrcb, dst); // BK: Rewrite after issue with RelWithDebInfo build on VC14 with OpenCV 3.1.0

	cv::resize(dst[1], dst[1], cv::Size(), 0.5, 0.5, cv::INTER_CUBIC);
	cv::resize(dst[2], dst[2], cv::Size(), 0.5, 0.5, cv::INTER_CUBIC);

	write_raw(yuv_stream, dst[0].data, dst[0].size().area());
	write_raw(yuv_stream, dst[2].data, dst[2].size().area());
	write_raw(yuv_stream, dst[1].data, dst[1].size().area());
}

/**
 * Export image in YUV RGB planar
 * */
void export_rgb_planar(std::ofstream& yuv_stream, const cv::Mat_<cv::Vec3b>& bgr) {
	cv::Size sz = bgr.size();

	cv::Mat_<uchar> r_channel(sz), g_channel(sz), b_channel(sz);
	std::vector<cv::Mat> dst{ b_channel, g_channel, r_channel };
	cv::split(bgr, dst);

	write_raw(yuv_stream, r_channel.data, sz.width*sz.height);
	write_raw(yuv_stream, g_channel.data, sz.width*sz.height);
	write_raw(yuv_stream, b_channel.data, sz.width*sz.height);
}


/**
 * Export image in YUV RGB interleaved
 * */
void export_rgb_interleaved(std::ofstream& yuv_stream, const cv::Mat_<cv::Vec3b>& bgr) {
	cv::Mat_<cv::Vec3b> rgb;
	cv::cvtColor(bgr, rgb, CV_BGR2RGB);
	write_raw(yuv_stream, rgb.data, 3 * rgb.cols*rgb.rows);
}


/**
 * Export YUV depth map
 * */
void export_raw_mono(const cv::Mat& img, const std::string& yuv_filename, int out_bit_depth) {
	int type;
	int in_bit_depth;
	switch (img.depth()) {
	case CV_8U: in_bit_depth = 8; break;
	case CV_16U: in_bit_depth = 16; break;
	default: throw std::invalid_argument("invalid input image bit depth");
	}
	switch (out_bit_depth) {
	case 8: type = CV_8U; break;
	case 16: type = CV_16U; break;
	default: throw std::invalid_argument("invalid raw image bit depth");
	}
	float alpha = std::exp2(float(out_bit_depth - in_bit_depth));
	cv::Mat out_mat;
	img.convertTo(out_mat, type, alpha);
	int length = img.cols * img.rows * out_bit_depth / 8;
	std::ofstream yuv_stream(yuv_filename, std::ios_base::binary);
	write_raw(yuv_stream, out_mat.data, length);
}



void write_color(std::string filename, const cv::Mat& img)
{
	cv::Mat result;
	//format supported by openCV (rgb)
	if (filename.find("yuv") == std::string::npos) {
		if (color_space == COLORSPACE_YUV) {
			cv::cvtColor(img, result, CV_YCrCb2BGR);
		}
		else if(color_space == COLORSPACE_RGB) {
			img.copyTo(result);
		}
		cv::imwrite(filename, result);
	}
	//yuv image (ycrcb)
	else {
		if (color_space == COLORSPACE_YUV) {
			img.copyTo(result);
		}
		else if (color_space == COLORSPACE_RGB) {
			cv::cvtColor(img, result, CV_BGR2YCrCb);
		}
		std::ofstream file(filename, std::ios::binary);
		export_ycbcr420(file, result);
	}
	
}

