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

#include "blending.hpp"
#include "IntegralImage2D.h"
#include "Timer.hpp"
#include "Config.hpp"

#include <iostream>

#include "opencv2/imgproc.hpp"

/**
 * Simple blur
 * */
void calcBlurring(cv::Mat img, cv::Mat &blr, cv::Mat msk, int rad)
{
	int chn = img.channels();
	if (chn == 1)
	{
		blr = cv::Mat(img.size().height, img.size().width, CV_32FC1);

		int srd_ele = 1;
		int srd_row = srd_ele * img.size().width;

		IntegralImage2D<float, 1> iim_dx;
		iim_dx.setInput((float*)img.data, msk.data, img.size().width, img.size().height, srd_ele, srd_row);

		int rad_2 = rad;
		int rad_1 = int(2 * rad_2 + 1);
#pragma omp parallel default(shared)
		{
#pragma omp for schedule(dynamic, 10) nowait
			for (int i = 0; i < img.size().height; i++)
			{
				for (int j = 0; j < img.size().width; j++)
				{
					if (msk.at<uchar>(i, j) == 0)
					{
						blr.at<float>(i, j) = 0;
						continue;
					}


					int start_x = j - rad_2;
					int start_y = i - rad_2;
					int end_y = start_y + rad_1;
					int end_x = start_x + rad_1;

					start_x = std::max(0, start_x);
					start_y = std::max(0, start_y);
					end_x = std::min(end_x, img.size().width);
					end_y = std::min(end_y, img.size().height);

					unsigned cnt = iim_dx.getFiniteElementsCountSE(start_x, start_y, end_x, end_y);
					unsigned sum = (int)iim_dx.getFirstOrderSumSE(start_x, start_y, end_x, end_y);

					if (cnt > 0)
						blr.at<float>(i, j) = float(float(sum) / float(cnt));
				}
			}
		}
	}
	else if (chn == 3)
	{
		blr = cv::Mat(img.size().height, img.size().width, CV_32FC3);

		int srd_ele = sizeof(cv::Vec3f) / sizeof(float);
		int srd_row = srd_ele * img.size().width;

		IntegralImage2D<float, 3> iim_dx;
		iim_dx.setInput((float*)img.data, msk.data, img.size().width, img.size().height, srd_ele, srd_row);

		int rad_2 = rad;
		int rad_1 = int(2 * rad_2 + 1);
#pragma omp parallel default(shared)
		{
#pragma omp for schedule(dynamic, 10) nowait
			for (int i = 0; i < img.size().height; i++)
			{
				for (int j = 0; j < img.size().width; j++)
				{
					if (msk.at<uchar>(i, j) == 0)
					{
						blr.at<cv::Vec3f>(i, j)[0] = 0;
						blr.at<cv::Vec3f>(i, j)[1] = 0;
						blr.at<cv::Vec3f>(i, j)[2] = 0;
						continue;
					}

					int start_x = j - rad_2;
					int start_y = i - rad_2;
					int end_y = start_y + rad_1;
					int end_x = start_x + rad_1;

					start_x = std::max(0, start_x);
					start_y = std::max(0, start_y);
					end_x = std::min(end_x, img.size().width);
					end_y = std::min(end_y, img.size().height);

					unsigned count = iim_dx.getFiniteElementsCountSE(start_x, start_y, end_x, end_y);
					cv::Vec3f sum = iim_dx.getFirstOrderSumSE(start_x, start_y, end_x, end_y);

					if (count > 0) {
						blr.at<cv::Vec3f>(i, j)[0] = float(float(sum[0]) / float(count));
						blr.at<cv::Vec3f>(i, j)[1] = float(float(sum[1]) / float(count));
						blr.at<cv::Vec3f>(i, j)[2] = float(float(sum[2]) / float(count));
					}
				}
			}
		}
	}
}
/**
 * gaussian blur
 * */
void calcBlurringGaussian(cv::Mat img, cv::Mat &blr, cv::Mat msk, int rad, float ect = 0.0f)
{
	float sig = (ect == 0.0f) ? 0.3f * float(rad) + 0.5f : ect;
	std::vector<float> fct_vec(rad + 1);
	for (int rad_idx = rad; rad_idx >= 0; rad_idx--)
	{
		fct_vec[rad_idx] = std::exp(-float(rad_idx * rad_idx) / (2.0f * sig * sig));
		if (rad_idx < rad)
			fct_vec[rad_idx] -= fct_vec[rad_idx + 1];
	}

	int chn = img.channels();
	if (chn == 1)
	{
		blr = cv::Mat(img.size().height, img.size().width, CV_8UC1);

		int srd_ele = 1;
		int srd_row = srd_ele * img.size().width;

		IntegralImage2D<unsigned char, 1> iim_dx;
		iim_dx.setInput(img.data, msk.data, img.size().width, img.size().height, srd_ele, srd_row);

#pragma omp parallel default(shared)
		{
#pragma omp for schedule(dynamic, 10) nowait
			for (int i = 0; i < img.size().height; i++)
			{
				for (int j = 0; j < img.size().width; j++)
				{
					if (msk.at<uchar>(i, j) == 0)
					{
						blr.at<uchar>(i, j) = 0;
						continue;
					}

					float cnt_tot = 0.0f;
					float sum_tot = 0.0f;
					for (int rad_idx = rad; rad_idx >= 0; rad_idx--)
					{
						int start_x = j - rad_idx;
						int start_y = i - rad_idx;
						int end_y = start_y + 2 * rad_idx + 1;
						int end_x = start_x + 2 * rad_idx + 1;

						start_x = std::max(0, start_x);
						start_y = std::max(0, start_y);
						end_x = std::min(end_x, img.size().width);
						end_y = std::min(end_y, img.size().height);

						unsigned cnt = iim_dx.getFiniteElementsCountSE(start_x, start_y, end_x, end_y);
						unsigned sum = iim_dx.getFirstOrderSumSE(start_x, start_y, end_x, end_y);

						cnt_tot += fct_vec[rad_idx] * float(cnt);
						sum_tot += fct_vec[rad_idx] * float(sum);
					}
					if (cnt_tot > 0)
						blr.at<uchar>(i, j) = uchar(float(sum_tot) / float(cnt_tot));
				}
			}
		}
	}
	else if (chn == 3)
	{
		blr = cv::Mat(img.size().height, img.size().width, CV_8UC3);

		int srd_ele = sizeof(cv::Vec3b) / sizeof(uchar);
		int srd_row = srd_ele * img.size().width;

		IntegralImage2D<unsigned char, 3> iim_dx;
		iim_dx.setInput(img.data, msk.data, img.size().width, img.size().height, srd_ele, srd_row);

#pragma omp parallel default(shared)
		{
#pragma omp for schedule(dynamic, 10) nowait
			for (int i = 0; i < img.size().height; i++)
			{
				for (int j = 0; j < img.size().width; j++)
				{
					if (msk.at<uchar>(i, j) == 0)
					{
						blr.at<cv::Vec3b>(i, j)[0] = 0;
						blr.at<cv::Vec3b>(i, j)[1] = 0;
						blr.at<cv::Vec3b>(i, j)[2] = 0;
						continue;
					}

					float cnt_tot = 0.0f;
					cv::Vec3f sum_tot = cv::Vec3f(0.0f, 0.0f, 0.0f);
					for (int rad_idx = rad; rad_idx >= 0; rad_idx--)
					{
						int start_x = j - rad_idx;
						int start_y = i - rad_idx;
						int end_y = start_y + 2 * rad_idx + 1;
						int end_x = start_x + 2 * rad_idx + 1;

						start_x = std::max(0, start_x);
						start_y = std::max(0, start_y);
						end_x = std::min(end_x, img.size().width);
						end_y = std::min(end_y, img.size().height);

						unsigned cnt = iim_dx.getFiniteElementsCountSE(start_x, start_y, end_x, end_y);
						cv::Vec3i sum = iim_dx.getFirstOrderSumSE(start_x, start_y, end_x, end_y);

						cnt_tot += fct_vec[rad_idx] * float(cnt);
						sum_tot[0] += fct_vec[rad_idx] * float(sum[0]);
						sum_tot[1] += fct_vec[rad_idx] * float(sum[1]);
						sum_tot[2] += fct_vec[rad_idx] * float(sum[2]);
					}
					if (cnt_tot > 0)
					{
						blr.at<cv::Vec3b>(i, j)[0] = uchar(sum_tot[0] / cnt_tot);
						blr.at<cv::Vec3b>(i, j)[1] = uchar(sum_tot[1] / cnt_tot);
						blr.at<cv::Vec3b>(i, j)[2] = uchar(sum_tot[2] / cnt_tot);
					}
				}
			}
		}
	}
}
void calcBlurringBilateral(cv::Mat img, cv::Mat &blr, cv::Mat /*msk*/, int bil_rad = 10, int bil_itr = 10, int bil_sig_col = 10, int bil_sig_spc = 150)
{
	blr = img.clone();
	for (int itr = 0; itr < bil_itr; itr++)
	{
		cv::Mat out;
		cv::bilateralFilter(blr, out, bil_rad, bil_sig_col, bil_sig_spc);
		blr = out;
	}
}
cv::Mat blend_img_by_max(const std::vector<cv::Mat>& imgs, const std::vector<cv::Mat>& qualities, const std::vector<cv::Mat>& depth_prolongations, cv::Vec3f empty_color, cv::Mat& quality, cv::Mat& depth_prolongation_mask, cv::Mat& inpaint_mask) {
	cv::Mat res = cv::Mat::zeros(imgs[0].size(), CV_32FC3);
	cv::Mat quality_res = cv::Mat::zeros(imgs[0].size(), CV_32F);
	cv::Mat depth_prolongation_mask_res = cv::Mat::zeros(imgs[0].size(), depth_prolongation_mask.type());
	cv::Mat inpaint_mask_res = (quality_res < -1.0);
	res.setTo(empty_color);
	for (int x = 0; x < res.cols; ++x)
		for (int y = 0; y < res.rows; ++y) {
			int best_i = -1;
			bool best_depth_prolongated = true;
			float max = -1;
			//choose best pixel amoung the input images
			for (int i = 0; i < static_cast<int>(imgs.size()); ++i) {
				bool d_prolongated = depth_prolongations[i].at<bool>(y, x);
				bool valid_d = qualities[i].at<float>(y, x) > 0;
				float a = qualities[i].at<float>(y, x);
				if ((valid_d && (best_depth_prolongated == d_prolongated && a > max)) || (best_depth_prolongated && !d_prolongated)) {
					max = a;
					best_i = i;
					best_depth_prolongated = d_prolongated;
				}
			}
			//no candidate found : fill with empty color
			if (best_i > -1) {
				res.at<cv::Vec3f>(y, x) = imgs[best_i].at<cv::Vec3f>(y, x);
				depth_prolongation_mask_res.at<float>(y, x) = best_depth_prolongated;
				inpaint_mask_res.at<bool>(y, x) = false;
				quality_res.at<float>(y, x) = max;
			}
			//else take the best available color
			else {
				res.at<cv::Vec3f>(y, x) = empty_color;
				depth_prolongation_mask_res.at<float>(y, x) = true;
				inpaint_mask_res.at<bool>(y, x) = true;
				quality_res.at<float>(y, x) = 0;
			}
		}
	inpaint_mask = inpaint_mask_res;
	depth_prolongation_mask = depth_prolongation_mask_res;
	quality = quality_res;
	return res;
}
cv::Mat blend_depth_by_max(const std::vector<cv::Mat>&  imgs, const std::vector<cv::Mat>&  depth_invs) {
	cv::Mat res = cv::Mat::zeros(imgs[0].size(), CV_32F);
	res.setTo(0);
	for (int x = 0; x < res.cols; ++x)
		for (int y = 0; y < res.rows; ++y) {
			int best_i = -1;
			float max = 0;
			for (int i = 0; i < static_cast<int>(imgs.size()); ++i) {
				float a = depth_invs[i].at<float>(y, x);
				if (a > max) {
					max = a;
					best_i = i;
				}
			}
			if (best_i > -1)
				res.at<float>(y, x) = imgs[best_i].at<float>(y, x);
		}
	return res;
}

cv::Mat blend_img(const std::vector<cv::Mat>& imgs, const std::vector<cv::Mat>& qualities, const std::vector<cv::Mat>& depth_prolongations, cv::Vec3f empty_color, cv::Mat& quality, cv::Mat& depth_prolongation_mask, cv::Mat& inpaint_mask, float blending_exp){
	if (blending_exp < 0)
		return blend_img_by_max(imgs, qualities, depth_prolongations, empty_color, quality, depth_prolongation_mask, inpaint_mask);
	cv::Mat res = cv::Mat::zeros(imgs[0].size(), CV_32FC3);
	cv::Mat quality_res = cv::Mat::zeros(imgs[0].size(), CV_32F);
	cv::Mat depth_prolongation_mask_res = cv::Mat::zeros(imgs[0].size(), depth_prolongation_mask.type());
	cv::Mat inpaint_mask_res = (quality_res < -1.0);
	for (int x = 0; x < res.cols; ++x)
		for (int y = 0; y < res.rows; ++y) {
			//computed from original depth
			float s = 0.0; 
			//computed from prolongation of the depth map
			cv::Vec3f inpainted_col = empty_color;
			cv::Vec3f inpainted_depth_col(0, 0, 0);
			cv::Vec3f col(0, 0, 0);
			float inpainted_depth_s = 0.0;
			for (int i = 0; i < static_cast<int>(imgs.size()); ++i) {
				float a = qualities[i].at<float>(y, x);
				bool valid_d = qualities[i].at<float>(y, x) > 0 && !depth_prolongations[i].at<bool>(y, x);
				if (a > 0 && valid_d) {//priority to color computed from original depth
					a = powf(a, blending_exp);
					s += a;
					col += a * imgs[i].at<cv::Vec3f>(y, x);
				}
				else {
					if (a > 0 && qualities[i].at<float>(y, x) > 0) { //second priority to color computed from inpainted depth
						a = powf(a, blending_exp);
						inpainted_depth_s += a;
						inpainted_depth_col += a*imgs[i].at<cv::Vec3f>(y, x);
					}
				}
			}
			if (s == 0) {
			//no input image has a color at this pixel
				if (inpainted_depth_s == 0.0) {
					res.at<cv::Vec3f>(y, x) = inpainted_col;
					inpaint_mask_res.at<bool>(y, x) = true;
					depth_prolongation_mask_res.at<bool>(y, x) = true;
					quality_res.at<float>(y,x) = 0.0;
				}
			//color comes from interpolated depth 
				else {
					res.at<cv::Vec3f>(y, x) = inpainted_depth_col / inpainted_depth_s;
					depth_prolongation_mask_res.at<bool>(y, x) = true;
					quality_res.at<float>(y, x) = powf(inpainted_depth_s, 1.0f/blending_exp);
				}
			}
			//color comes from original depth in at least one image
			else {
				res.at<cv::Vec3f>(y, x) = col / s;
				depth_prolongation_mask_res.at<bool>(y, x) = false;
				quality_res.at<float>(y, x) = powf(s, 1.0f/blending_exp);
			}
		}
	inpaint_mask_res.copyTo(inpaint_mask);
	depth_prolongation_mask = depth_prolongation_mask_res;
	quality = quality_res;
	return res;
}

void split_frequencies(const cv::Mat & img, cv::Mat & low_freq, cv::Mat & high_freq, const cv::Mat& mask)
{
	PROF_START("blur");
	int kernel_size = ((int)MAX(img.rows, img.cols) / 20);
	//RGB: blur all three channels
	if (color_space == COLORSPACE_RGB) {
		cv::Mat img_rgb_blurry;
	
		calcBlurring(img, low_freq, mask, kernel_size);

		high_freq = img - low_freq;
	}
	//YCrCb: blur only Y channel
	else if (color_space == COLORSPACE_YUV) {
		std::vector<cv::Mat> chans;
		cv::split(img, chans);
		chans = { chans[0], chans[1], chans[2] };
		
		calcBlurring(chans[0], chans[0], mask, kernel_size);

		cv::merge(chans, low_freq);

		high_freq = img - low_freq;
	}

	PROF_END("blur");
}


cv::Mat blend_depth(const std::vector<cv::Mat>&  imgs, const std::vector<cv::Mat>&  depth_invs, int blending_exp) {
	if (blending_exp < 0)
		return blend_depth_by_max(imgs, depth_invs);
	cv::Mat res = cv::Mat::zeros(imgs[0].size(), CV_32F);
	for (int x = 0; x < res.cols; ++x)
		for (int y = 0; y < res.rows; ++y) {
			float s = 0.0;
			for (int i = 0; i < static_cast<int>(imgs.size()); ++i) {
				float d = depth_invs[i].at<float>(y, x);
				if (d > 0) {
					float a = powf(d, (float)blending_exp);
					s += a;
					res.at<float>(y, x) += a*imgs[i].at<float>(y, x);
				}
			}
			if (s == 0)
				res.at<float>(y, x) = 0;
			else
				res.at<float>(y, x) /= s;
		}
	return res;
}
