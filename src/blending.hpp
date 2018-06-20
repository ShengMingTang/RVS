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


#include <opencv2/core.hpp>

/**
@file blending.hpp
\brief The file containing the image blending functions
*/

/**
\brief Blend an array of color images chosing the best pixel.

The best pixel is chosen among the input imgs: color(x,y)\f$=argmax_{c_i(x,y)}\left(w_i(x,y)\right)\f$, where \f$w_i\f$ is the quality of view i for the pixel (x,y).

@param imgs Array of color images
@param qualities Quality of the input image
@param depth_prolongations Array of mask, indicating if the depth is valid or extrapolated
@param empty_color Color for unknown pixels
@param quality Output quality of the result
@param depth_prolongation_mask Output mask indicating if the depth is valid or extrapolated
@param inpaint_mask Output mask indicating where we have no value
@return Color image containing the best pixels according to quality maps
*/
cv::Mat blend_img_by_max(const std::vector<cv::Mat>& imgs, const std::vector<cv::Mat>& qualities, const std::vector<cv::Mat>& depth_prolongations, cv::Vec3f empty_color, cv::Mat& quality, cv::Mat& depth_prolongation_mask, cv::Mat& inpaint_mask);

/**
 * \brief Split an image in low and high frequency.

 * The low frequencies is obtained by applying a mean blur of size \f$\frac{1}{20}\f$ of the input image size. img=low_freq+high_freq
 * @param img Image to process
 * @param low_freq Output containing the low frequency part of the image
 * @param high_freq Output containing the high frequency part of the image
 * @param mask Mask for part of the image to ignore
 * */
void split_frequencies(const cv::Mat & img, cv::Mat& low_freq, cv::Mat& high_freq, const cv::Mat& mask);

/**
\brief Blend an array of color images by weigthed mean with quality of the pixels. 

Weighted mean: color(x,y)=\f$=\frac{\sum\limits_{i=0}^nw_i(x,y) c_i(x,y)}{\sum\limits_{i=0}^nw_i(x,y)}\f$ with \f$w_i(x,y)=\f$qualities[i](x,y)^blending_exp.

If blending_exp < 0, calls blend_img_by_max().
@param imgs Array of color images to blend
@param qualities Array quality maps
@param depth_prolongations Array of masks, indicating if the depth is valid or extrapolated
@param empty_color Color for unknown pixels
@param quality Output quality of the result
@param depth_prolongation_mask Output mask indicating if the depth is valid or extrapolated
@param inpaint_mask Output mask indicating where we have no value
@param blending_exp
<0 : Calls blend_img_by_max().
0 : Mean of the available RGB images.
>0 : Mean weighted by quality^blending_exp.
@return Color image containing the the weighted mean of the input images. 
*/
cv::Mat blend_img(const std::vector<cv::Mat>& imgs, const std::vector<cv::Mat>& qualities, const std::vector<cv::Mat>& depth_prolongations, cv::Vec3f empty_color, cv::Mat& quality, cv::Mat& depth_prolongation_mask, cv::Mat& inpaint_mask, float blending_exp);
