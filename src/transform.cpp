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

Extraction of a generalized unproject -> translate/rotate -> project flow

Author  : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

#include "transform.hpp"

#include <opencv2/imgproc.hpp>

#include <iostream>
#include <algorithm>

extern float rescale; // Config.hpp

namespace
{
	float valid_tri(const cv::Mat & new_pos, cv::Vec2f a, cv::Vec2f b, cv::Vec2f c, float den) {
		cv::Vec2f A = new_pos.at<cv::Vec2f>((int)a[1], (int)a[0]);
		cv::Vec2f B = new_pos.at<cv::Vec2f>((int)b[1], (int)b[0]);
		cv::Vec2f C = new_pos.at<cv::Vec2f>((int)c[1], (int)c[0]);
		if ((B - A)[0] * (C - B)[1] - (B - A)[1] * (C - B)[0] < 0)
			return 0.0;
		float dst[] = {
			(A - B).dot(A - B),
			(A - C).dot(A - C),
			(B - C).dot(B - C)
		};
		if (dst[0] > dst[1]) std::swap(dst[0], dst[1]);
		if (dst[1] > dst[2]) std::swap(dst[1], dst[2]);
		if (dst[0] > dst[1]) std::swap(dst[0], dst[1]);
		float w = 2.0f*den / dst[1];//aire/aire du triangle rectangle généré par le coté médian
		return std::pow(MIN(w, 1.0f / w)*MIN(dst[2] / rescale, rescale / dst[2]), 1.0f / 8.0f);
	}

	void colorize_triangle(const cv::Mat & img, const cv::Mat & depth, const cv::Mat& depth_prologation_mask, const cv::Mat & new_pos, cv::Mat& res, cv::Mat& depth_inv, cv::Mat& new_depth_prologation_mask, cv::Mat& triangle_shape, cv::Vec2i a, cv::Vec2i b, cv::Vec2i c) {
		//	float w = (float)img.cols; -- unused variable
		//	float h = (float)img.rows; -- unused variable
		cv::Vec2f A = new_pos.at<cv::Vec2f>(a[1], a[0]);
		cv::Vec2f B = new_pos.at<cv::Vec2f>(b[1], b[0]);
		cv::Vec2f C = new_pos.at<cv::Vec2f>(c[1], c[0]);
		cv::Vec3f colA = img.at<cv::Vec3f>(a[1], a[0]);
		cv::Vec3f colB = img.at<cv::Vec3f>(b[1], b[0]);
		cv::Vec3f colC = img.at<cv::Vec3f>(c[1], c[0]);
		float dA = (depth.at<float>(a[1], a[0]));
		float dB = (depth.at<float>(b[1], b[0]));
		float dC = (depth.at<float>(c[1], c[0]));
		float den = (B[1] - C[1]) * (A[0] - C[0]) + (C[0] - B[0]) * (A[1] - C[1]);
		if (den <= 0.0f)
			return;
		float triangle_validity = valid_tri(new_pos, a, b, c, den)/* std::powf(MAX(MAX(abs(dA-dB),abs(dB-dC)),abs(dC-dA)), 1.0 / 8.0)*/;
		if (triangle_validity == 0.0)
			return;
		//triangle_validity /= std::powf(MAX(MAX(dA, dB), dC) - MIN(MIN(dA, dB), dC), 1.0 / 8.0) / 1000.0;
		triangle_validity *= 100.0;
		float Xmin, Xmax, Ymin, Ymax;
		float offset = 0.5;
		Xmin = MIN(MIN(A[0], B[0]), C[0]);
		Ymin = MIN(MIN(A[1], B[1]), C[1]);
		Xmax = MAX(MAX(A[0], B[0]), C[0]);
		Ymax = MAX(MAX(A[1], B[1]), C[1]);
		for (int x = (int)std::floor(Xmin); x <= std::ceil(Xmax); x++)
			for (int y = (int)std::floor(Ymin); y <= std::ceil(Ymax); y++)
				if (x > 0 && x < res.cols && y>0 && y < res.rows) {

					float lambda_1 = ((B[1] - C[1]) * ((float)x + offset - C[0]) + (C[0] - B[0]) * ((float)y + offset - C[1])) / den;
					float lambda_2 = ((C[1] - A[1]) * ((float)x + offset - C[0]) + (A[0] - C[0]) * ((float)y + offset - C[1])) / den;
					float lambda_3 = 1.0f - lambda_1 - lambda_2;
					if (lambda_1 >= 0.0f && lambda_1 <= 1.0f &&lambda_2 >= 0.0f && lambda_2 <= 1.0f && lambda_3 >= 0.0f && lambda_3 <= 1.0f)
					{
						float quality_inside_triangle = 1.0;// MAX(lambda_1, MAX(lambda_2, lambda_3));
						cv::Vec3f col = colA*lambda_1 + colB*lambda_2 + colC*lambda_3;
						float d = dA*lambda_1 + dB*lambda_2 + dC*lambda_3;

						//if the pixel comes from original depth map and is in foreground
						if ((depth_inv.at<float>(y, x)*depth_inv.at<float>(y, x)*depth_inv.at<float>(y, x)* triangle_shape.at<float>(y, x) < 1.0 / d / d / d * triangle_validity*quality_inside_triangle
							|| new_depth_prologation_mask.at<bool>(y, x)) && !depth_prologation_mask.at<bool>((int)a[1], (int)a[0])
							&& !depth_prologation_mask.at<bool>((int)b[1], (int)b[0])
							&& !depth_prologation_mask.at<bool>((int)c[1], (int)c[0]))
						{
							depth_inv.at<float>(y, x) = 1.0f / d;
							new_depth_prologation_mask.at<bool>(y, x) = false;
							triangle_shape.at<float>(y, x) = triangle_validity * quality_inside_triangle;
							res.at<cv::Vec3f>(y, x) = col;
						}
						//if the pixel comes from inpainted depth map and is in foreground and there is no pixel from the original depth map
						else if (depth_inv.at<float>(y, x)*depth_inv.at<float>(y, x)*depth_inv.at<float>(y, x)* triangle_shape.at<float>(y, x) < 1.0 / d / d / d * triangle_validity*quality_inside_triangle
							&& new_depth_prologation_mask.at<bool>(y, x)
							&& (depth_prologation_mask.at<bool>((int)a[1], (int)a[0])
								|| depth_prologation_mask.at<bool>((int)b[1], (int)b[0])
								|| depth_prologation_mask.at<bool>((int)c[1], (int)c[0])))
						{
							depth_inv.at<float>(y, x) = 1.0f / d;
							triangle_shape.at<float>(y, x) = triangle_validity * quality_inside_triangle;
							res.at<cv::Vec3f>(y, x) = col;
						}
					}
				}
		return;
	}
} // namespace

cv::Mat3f transform_trianglesMethod(cv::Mat3f input_color, cv::Mat1f input_depth, cv::Mat2f input_positions, cv::Size output_size, cv::Mat1f& depth, cv::Mat1f& quality, bool horizontalWrap)
{
	auto input_size = input_color.size();
    int w = input_size.width;
	cv::Mat1b input_depth_mask = input_depth > 0.f;
	cv::Mat3f color = cv::Mat3f::zeros(output_size);
	cv::Mat1f depth_inv = cv::Mat1f::zeros(output_size);
	quality = cv::Mat1f::zeros(output_size);
	cv::Mat1b new_depth_prologation_mask = cv::Mat1b::ones(output_size);

	// Compute triangulation
	for (int i = 0; i < input_size.height - 1; ++i) {
		for (int j = 0; j < input_size.width - 1; ++j) {
			if (input_depth(i, j + 1) > 0.f && input_depth(i + 1, j) > 0.f && /*why?*/ input_positions(i, j + 1)[0] > 0.f && /*why?*/ input_positions(i + 1, j)[0] > 0.f) {
				if (input_depth(i, j) > 0.f && /*why?*/ input_positions(i, j)[0] > 0.f)
					colorize_triangle(input_color, input_depth, input_depth_mask, input_positions, color, depth_inv, new_depth_prologation_mask, quality,
						/*why float?*/ cv::Vec2i(j, i), cv::Vec2i(j + 1, i), cv::Vec2i(j, i + 1));
				if (input_depth(i + 1, j + 1) > 0.f && input_positions(i + 1, j + 1)[0] > 0.f)
					colorize_triangle(input_color, input_depth, input_depth_mask, input_positions, color, depth_inv, new_depth_prologation_mask, quality,
						/*why float?*/ cv::Vec2i(j + 1, i + 1), cv::Vec2i(j, i + 1), cv::Vec2i(j + 1, i));
			
                // stitch left and right borders with triangles (e.g. for equirectangular projection)
                if( horizontalWrap && j == 0 ) { 
                    colorize_triangle(input_color, input_depth, input_depth_mask, input_positions, color, depth_inv, new_depth_prologation_mask, quality, 
                        cv::Vec2i(w-1, i), cv::Vec2i( 0, i), cv::Vec2i(w-1, i + 1));
                    colorize_triangle(input_color, input_depth, input_depth_mask, input_positions, color, depth_inv, new_depth_prologation_mask, quality, 
                        cv::Vec2i(0, i + 1),   cv::Vec2i(w-1, i + 1), cv::Vec2i(0, i));
                }
            }
		}
	}

	depth = 1.f / depth_inv;
	cv::multiply(quality, depth_inv, quality, /*why?*/ 100.f);

	return color;
}

