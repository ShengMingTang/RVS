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

#include "transform.hpp"

#include "opencv2/imgproc.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"

#include <iostream>
#include <algorithm>


namespace {
cv::Mat translation_map(const cv::Mat& depth, const Translation & t, const cv::Mat & old_cam_mat, const cv::Mat & new_cam_mat, float sensor, float offset) {
	cv::Size s((int)(rescale*(float)depth.size().width*image_bigger_ratio), (int)(rescale*(float)depth.size().height*image_bigger_ratio));
	float w = depth.cols*rescale;
	float h = depth.rows*rescale;
	float dX = t[0];
	float dY = -t[1];
	float dZ = -t[2];
	float startW = (image_bigger_ratio - 1.0f) / 2.0f*w;
	float startH = (image_bigger_ratio - 1.0f) / 2.0f*h;
	float fx = old_cam_mat.at<float>(0, 0)*rescale;
//	float fy = old_cam_mat.at<float>(1, 1)*rescale; -- unused variable
	float px = old_cam_mat.at<float>(0, 2)*rescale;
	float py = h - old_cam_mat.at<float>(1, 2)*rescale;
	float n_fx = new_cam_mat.at<float>(0, 0)*rescale;
//	float n_fy = new_cam_mat.at<float>(1, 1)*rescale; -- unused variable
	float n_px = new_cam_mat.at<float>(0, 2)*rescale;
	float n_py = h - new_cam_mat.at<float>(1, 2)*rescale;

	cv::Mat new_pos = cv::Mat::zeros(depth.size(), CV_32FC2);

	//compute new position
#pragma omp parallel default(shared)
	{
#pragma omp for schedule(dynamic, 1000) nowait
		for (int x = 0; x < depth.cols; ++x)
			for (int y = 0; y < depth.rows; ++y) {
				float d = (depth.at<float>(y, x));
				//std::cout << d << std::endl;
				if (d - dZ <= 0.0 || d == 0.0) {
					new_pos.at<cv::Vec2f>(y, x) = cv::Vec2f(-1.0, -1.0);
					continue;
				}
				//coordinates of the translated image in plane (x,y)
				float dispX = fx / sensor / d*w;
				float dispY = fx / sensor / d*w;
				float dx = dispX*dX;
				float dy = dispY*dY;
				float x2 = ((float)x + offset)*rescale - dx; 
				float y2 = ((float)y + offset)*rescale - dy; 
				//coordinate after z translation
				cv::Vec2f v((x2) / w * sensor - px, (y2) / w * sensor - py);
				float beta = (dZ*v[0]) / (fx + -fx*dZ / d);
				float gamma = (dZ*v[1]) / (fx + -fx*dZ / d);
				//new image with old focal length (optical center at (0,0))
				cv::Vec2f V(x2 + beta*dispX - px, y2 + gamma*dispY - py);
				//new image with new focal length (optical center at (0,0))
				V *= n_fx / fx;
				//coordinate in the new mage
				float X = (V[0] + startW + n_px*w / sensor);
				float Y = (V[1] + startH + n_py*w / sensor);
				new_pos.at<cv::Vec2f>(y, x) = cv::Vec2f(X, Y);
			}}
	return new_pos;
}
cv::Mat rotation_map(const cv::Mat & R, const cv::Mat& pos, const cv::Mat & new_cam_mat, float sensor) {
	cv::Size s((int)(rescale*(float)pos.size().width), (int)(rescale*(float)pos.size().height));
	float w = pos.cols*rescale;
	float h = pos.rows*rescale;
	float startW = (image_bigger_ratio - 1.0f) / 2.0f*w;
	float startH = (image_bigger_ratio - 1.0f) / 2.0f*h;
	float n_fx = new_cam_mat.at<float>(0, 0)*rescale;
//	float n_fy = new_cam_mat.at<float>(1, 1)*rescale; -- unused variable
	float n_px = new_cam_mat.at<float>(0, 2)*rescale;
	float n_py = h - new_cam_mat.at<float>(1, 2)*rescale;
	cv::Mat Rt = R.t();
	cv::Mat new_pos = cv::Mat::zeros(pos.size(), CV_32FC2);

	//compute new position
#pragma omp parallel default(shared)
	{
#pragma omp for schedule(dynamic, 1000) nowait
		for (int x = 0; x < pos.cols; ++x)
			for (int y = 0; y < pos.rows; ++y) {
				if (pos.at<cv::Vec2f>(y, x)[0] < 0.0) {
					continue;
				}
				//coordinate of the translated image 
				cv::Vec3f vt((pos.at<cv::Vec2f>(y, x)[0]-startW - n_px) / w, (pos.at<cv::Vec2f>(y, x)[1]-startH - n_py) / w, n_fx / sensor);
				//coordinates of the rotated image
				cv::Mat Vr = Rt*(cv::Mat)vt;
				Vr /= Vr.at<float>(2, 0)*sensor / n_fx;
				float xi = (Vr.at<float>(0, 0))*w + n_px;
				float yi = (Vr.at<float>(1, 0))*w + n_py;
				new_pos.at<cv::Vec2f>(y, x) = cv::Vec2f(xi, yi);
				//std::cout << vt << " " << Vr << std::endl;
			}}
	return new_pos;
}
} // namespace

/*translate camera in camera coordinate system, in any 3 directions, the result is a bigger image, to keep more information for a future rotation.
-depth_inv: image in which the inverse of the depth for the new image will be stored, can be used for blending
-image_bigger_ratio: multiplies size of img, to get a bigger image. The resulting image is centered*/
cv::Mat translateBigger_squaresMethod(const cv::Mat& img, const cv::Mat& depth, const cv::Mat& depth_prologation_mask, const cv::Mat& R, const Translation & t, const cv::Mat & old_cam_mat, const cv::Mat & new_cam_mat, float sensor, cv::Mat& depth_inv, cv::Mat& new_depth_prologation_mask, bool with_rotation) {
	cv::Size s((int)(rescale*(float)depth.size().width), (int)(rescale*(float)depth.size().height));
	if (!with_rotation)
		s = cv::Size(static_cast<int>(s.width*image_bigger_ratio), static_cast<int>(s.height*image_bigger_ratio));
	float w = depth.cols*rescale;
	float h = depth.rows*rescale;

	depth_inv = cv::Mat::zeros(s, CV_32F);
	new_depth_prologation_mask = cv::Mat::ones(s, depth_prologation_mask.type());
	cv::Mat res = cv::Mat::zeros(s, CV_32FC3);

	//compute new position
	cv::Mat new_pos_m = translation_map(depth, t, old_cam_mat, new_cam_mat, sensor, 0.0f);
	cv::Mat new_pos_M = translation_map(depth, t, old_cam_mat, new_cam_mat, sensor, 0.99f);
	if (with_rotation) {
		new_pos_m = rotation_map(R, new_pos_m, new_cam_mat, sensor);
		new_pos_M = rotation_map(R, new_pos_M, new_cam_mat, sensor);
	}
	cv::Mat img_big;
	cv::resize(img, img_big, cv::Size((int)(rescale*(float)img.cols), (int)(rescale*(float)img.rows)),0,0,CV_INTER_CUBIC);

	for (int x = 0; x < img.cols; ++x)
		for (int y = 0; y < img.rows; ++y) {
			float d = depth.at<float>(y, x);
			//coordinate in the new image
			float Xm = new_pos_m.at<cv::Vec2f>(y, x)[0];
			float Ym = new_pos_m.at<cv::Vec2f>(y, x)[1];
			float XM = new_pos_M.at<cv::Vec2f>(y, x )[0];
			float YM = new_pos_M.at<cv::Vec2f>(y, x)[1];
			if (Xm > XM) {
				continue;
			}
			if (Ym > YM) {
				continue;
			}
			if (YM < 0 || XM < 0 || Ym > image_bigger_ratio*h || Xm > image_bigger_ratio*w)
				continue;
			float stepX = (XM - Xm) / rescale;
			float stepY = (YM - Ym) / rescale;
			for (float frac_x = 0; frac_x < rescale; frac_x +=1.0)
				for (float frac_y = 0; frac_y < rescale; frac_y += 1.0){
					cv::Vec3f col = img_big.at<cv::Vec3f>((int)(rescale*(float)y+frac_y),(int) (rescale*(float)x+frac_x));
					for (int xint = MAX((int)(Xm+frac_x*stepX),0); xint <= MIN((int)(Xm + (frac_x+1.0)*stepX), s.width - 1); xint++)
						for (int yint = MAX((int)(Ym + frac_y*stepY),0); yint <= MIN((int)(Ym + (frac_y + 1.0)*stepY), s.height - 1); yint++) {
							int X = xint;
							int Y = yint;
							//if (X > 0 && X < image_bigger_ratio*w && Y>0 && Y < image_bigger_ratio*h) {
								//if the pixel comes from original depth map and is in foreground
							if ((depth_inv.at<float>(Y, X) < 1.0f / d || new_depth_prologation_mask.at<bool>(Y, X)) && !depth_prologation_mask.at<bool>(y, x))
							{
								depth_inv.at<float>(Y, X) = 1.0f / d;
								new_depth_prologation_mask.at<bool>(Y, X) = false;
								res.at<cv::Vec3f>(Y, X) = col;
							}
							//if the pixel comes from inpainted depth map and is in foreground and there is no pixel from the original depth map
							else if (depth_inv.at<float>(Y, X) < 1.0f / d && new_depth_prologation_mask.at<bool>(Y, X) && depth_prologation_mask.at<bool>(y, x)) {
								depth_inv.at<float>(Y, X) = 1.0f / d;
								res.at<cv::Vec3f>(Y, X) = col;
							}
						}
				}
		}
	return res;
}

namespace {
float valid_tri(const cv::Mat & new_pos, cv::Vec2f a, cv::Vec2f b, cv::Vec2f c, float den) {
	cv::Vec2f A = new_pos.at<cv::Vec2f>((int)a[1], (int)a[0]);
	cv::Vec2f B = new_pos.at<cv::Vec2f>((int)b[1], (int)b[0]);
	cv::Vec2f C = new_pos.at<cv::Vec2f>((int)c[1], (int)c[0]);
	if ((B - A)[0] * (C - B)[1] - (B - A)[1] * (C - B)[0] < 0)
		return 0.0;
	std::vector<float> dst;
	dst.push_back((A - B).dot(A - B));
	dst.push_back((A - C).dot(A - C));
	dst.push_back((B - C).dot(B - C));
	std::sort(dst.begin(), dst.end());
	//return std::powf(dst[0] / dst[2],1.0/8.0); 
	//return std::powf(MIN(MIN(dst[0], dst[1]), dst[2]), 1.0 / 8.0)*std::powf(den / (MAX(MAX((A - B).dot(A - B), (A - C).dot(A - C)), (B - C).dot(B - C))), 1.0 / 8.0);
	//if (MAX(MAX((A - B).dot(A - B), (A - C).dot(A - C)), (B - C).dot(B - C)) > 10.0)
	//	return 0;
	//return 1;
	//return std::powf(den / (MAX(MAX((A - B).dot(A - B), (A - C).dot(A - C)), (B - C).dot(B - C))), 1.0 / 8.0); //hauteur/plus long coté
	float w = 2.0f*den / dst[1];//aire/aire du triangle rectangle généré par le coté médian
	return std::pow(MIN(w, 1.0f / w)*MIN(dst[2] / rescale, rescale / dst[2]), 1.0f / 8.0f);
}

void colorize_triangle(const cv::Mat & img, const cv::Mat & depth, const cv::Mat& depth_prologation_mask, const cv::Mat & new_pos, cv::Mat& res, cv::Mat& depth_inv, cv::Mat& new_depth_prologation_mask, cv::Mat& triangle_shape, cv::Vec2f a, cv::Vec2f b, cv::Vec2f c) {
//	float w = (float)img.cols; -- unused variable
//	float h = (float)img.rows; -- unused variable
	cv::Vec2f A = new_pos.at<cv::Vec2f>((int)a[1], (int)a[0]);
	cv::Vec2f B = new_pos.at<cv::Vec2f>((int)b[1], (int)b[0]);
	cv::Vec2f C = new_pos.at<cv::Vec2f>((int)c[1], (int)c[0]);
	cv::Vec3f colA = img.at<cv::Vec3f>((int)a[1], (int)a[0]);
	cv::Vec3f colB = img.at<cv::Vec3f>((int)b[1], (int)b[0]);
	cv::Vec3f colC = img.at<cv::Vec3f>((int)c[1], (int)c[0]);
	float dA = (depth.at<float>((int)a[1], (int)a[0]));
	float dB = (depth.at<float>((int)b[1], (int)b[0]));
	float dC = (depth.at<float>((int)c[1], (int)c[0]));
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

cv::Mat translateBigger_trianglesMethod(const cv::Mat& img, const cv::Mat& depth, const cv::Mat& depth_prologation_mask, const cv::Mat& R, const Translation & t, const cv::Mat & old_cam_mat, const cv::Mat & new_cam_mat, float sensor, cv::Mat& depth_inv, cv::Mat& new_depth_prologation_mask, cv::Mat& triangle_shape, bool with_rotation) {
	cv::Size s((int)(rescale*(float)depth.size().width), (int)(rescale*(float)depth.size().height));
	if (!with_rotation)
		s = cv::Size(static_cast<int>(s.width*image_bigger_ratio), static_cast<int>(s.height*image_bigger_ratio));
	
	depth_inv = cv::Mat::zeros(s, CV_32F);
	new_depth_prologation_mask = cv::Mat::ones(s, depth_prologation_mask.type());
	triangle_shape = cv::Mat::zeros(s, CV_32F);
	cv::Mat res = cv::Mat::zeros(s, CV_32FC3);

	//compute new position
	cv::Mat new_pos = translation_map(depth, t, old_cam_mat, new_cam_mat, sensor, 0.5);
	if (with_rotation)
		new_pos = rotation_map(R, new_pos, new_cam_mat, sensor);
	
	//compute triangulation
	for (int x = 0; x < img.cols - 1; ++x)
		for (int y = 0; y < img.rows - 1; ++y) {
			if (depth.at<float>(y, x + 1) > 0.0 && depth.at<float>(y + 1, x) > 0.0 && new_pos.at<cv::Vec2f>(y, x + 1)[0] > 0.0 && new_pos.at<cv::Vec2f>(y + 1, x)[0] > 0.0) {
				if (depth.at<float>(y, x) > 0.0 && new_pos.at<cv::Vec2f>(y, x)[0] > 0.0)
					colorize_triangle(img, depth, depth_prologation_mask, new_pos, res, depth_inv, new_depth_prologation_mask, triangle_shape, cv::Vec2f((float)x, (float)y), cv::Vec2f((float)x + 1.0f, (float)y), cv::Vec2f((float)x, (float)y + 1.0f));
				if (depth.at<float>(y + 1, x + 1) > 0.0 && new_pos.at<cv::Vec2f>(y + 1, x + 1)[0] > 0.0)
					colorize_triangle(img, depth, depth_prologation_mask, new_pos, res, depth_inv, new_depth_prologation_mask, triangle_shape, cv::Vec2f((float)x + 1.0f, (float)y + 1.0f), cv::Vec2f((float)x, (float)y + 1.0f), cv::Vec2f((float)x + 1.0f, (float)y));
			}
		}
	return res;
}

void translateZ_disp(cv::Mat& d, float z) {
	for (int x = 0; x < d.cols; x++)
		for (int y = 0; y < d.rows; y++) {
			float d1 = d.at<float>(y, x);
			if (d1 > 0 && d1 - z > 0)
				d.at<float>(y, x) = 1.0f / (1.0f / d1 - z);
		}
}

cv::Mat rotate_disp(const cv::Mat& d, cv::Mat new_cam_mat, cv::Mat R) {
	cv::Mat disp = cv::Mat::zeros(d.size(), CV_32F);
	cv::Size s = d.size();
	float h = (float)s.height;
	float n_fx = new_cam_mat.at<float>(0, 0);
//	float n_fy = new_cam_mat.at<float>(1, 1); -- unused variable
	float n_px = new_cam_mat.at<float>(0, 2);
	float n_py = h - new_cam_mat.at<float>(1, 2);
	cv::Mat Rt = R.t(); cv::Mat t = Rt * cv::Mat(cv::Vec3f(-n_px, -n_py, n_fx));
	float c = (t.at<float>(2, 0));
	for (int x = 0; x < d.cols; x++)
		for (int y = 0; y < d.rows; y++)
			if (d.at<float>(y, x)>0.0) {
				float cos_phi = n_fx;
				float cos_psi = Rt.at<float>(2, 0)*x + Rt.at<float>(2, 1)*y + c;
				disp.at<float>(y, x) = d.at<float>(y, x)*cos_psi / cos_phi;
			}
	return disp;
}
