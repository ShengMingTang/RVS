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

#pragma once


#include "opencv2/core.hpp"

/**
@file blending.hpp
\brief The file containing the image blending functions
*/

/**
blend an array of color images chosing the best pixel
@param imgs Array of color images
@param qualities Quality of the input image
@param depth_prolongation Array of mask, indicating if the depth is valid or extrapolated
@param empty_color Color for unknown pixels
@param quality Output quality of the result
@param depth_prolongation_mask Output mask indicating if the depth is valid or extrapolated
@param inpaint_mask Output mask indicating where we have no value
*/
cv::Mat blend_img_by_max(const std::vector<cv::Mat>& imgs, const std::vector<cv::Mat>& qualities, const std::vector<cv::Mat>& depth_prolongations, cv::Vec3f empty_color, cv::Mat& quality, cv::Mat& depth_prolongation_mask, cv::Mat& inpaint_mask);

/**
 * Split an image in low and high frequency
 * @param img Image to process
 * @param low_freq Output containing the low frequency part of the image
 * @param high_freq Output containing the high frequency part of the image
 * @param mask Mask for part -s of the image to ignore
 * */
void split_frequencies(const cv::Mat & img, cv::Mat& low_freq, cv::Mat& high_freq, const cv::Mat& mask);

/**
blend an array of color images by average ponderated with quality of the pixels
@param imgs Array of color images
@param qualities Quality of the input image
@param depth_prolongation Array of mask, indicating if the depth is valid or extrapolated
@param empty_color Color for unknown pixels
@param quality Output quality of the result
@param depth_prolongation_mask Output mask indicating if the depth is valid or extrapolated
@param inpaint_mask Output mask indicating where we have no value
@param blending_exp
<0 : chose the RGB of the nearest object.
0 : mean of the available RGB.
>0 : mean (1/depth)^blending_exp.
*/
cv::Mat blend_img(const std::vector<cv::Mat>& imgs, const std::vector<cv::Mat>& qualities, const std::vector<cv::Mat>& depth_prolongations, cv::Vec3f empty_color, cv::Mat& quality, cv::Mat& depth_prolongation_mask, cv::Mat& inpaint_mask, float blending_exp);

/**
blend an array of float images by average ponderated with quality of the pixels
@param imgs Array of float images
@param depth_invs Array of disparities or inverse of depth images
@param blending_exp
<0 : chose the RGB of the nearest object.
0 : mean of the available RGB.
>0 : mean (1/depth)^blending_exp.
*/
cv::Mat blend_depth(const std::vector<cv::Mat>& imgs, const std::vector<cv::Mat>& depth_invs, int blending_exp);
