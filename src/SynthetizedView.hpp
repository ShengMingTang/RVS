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

#include "Config.hpp"
#include "View.hpp"

/**
 * View computed with DIBR
 * */
class SynthetizedView : public View
{
public:
	using View::View;
	/**
	\brief Constructor
	@param color Color image
	@param depth_inverse Depth inverse
	@param mask_depth Mask indicating empty values on the depth (for example in a Kinect depth map)
	@param size View final size
	*/
	SynthetizedView(cv::Mat color, cv::Mat depth_inverse, cv::Mat depth_prolongation_mask, cv::Size size);
	/**
	\brief Constructor
	@param parameters Camera parameters
	@param size View final size
	*/
	SynthetizedView(Parameters parameters, cv::Size size) { this->parameter = parameters; this->size = size; };
	/**
	\brief Copies this view
	*/
	virtual SynthetizedView* copy() const = 0;
	/**
	\brief Destructor
	*/
	virtual ~SynthetizedView();
	/**
	\brief Computes this view from the input View
	*/
	virtual void compute(View& img) = 0;
	/**
	\brief Quality per pixel
	@return An image of the same size than the view, indicating the quality of each pixel. 
	Pixel of higher quality will be prioritized during blending.
	*/
	virtual cv::Mat& get_quality() = 0;

	cv::Mat& get_depth_inverse() { return depth_inverse; };
	cv::Mat& get_depth() { depth = 1.0f / depth_inverse;  return depth; };

	
protected:
	/**
	\brief Disparity
	*/
	cv::Mat depth_inverse; 
};

/**
 * The algorithm to compute the view divides the input view in triangles, to avoid non disocclusion holes and get a first inpainting
 * The quality of each pixel is given by the shape of the triangle it belongs to and the depth of the pixel
 * */
class SynthetizedViewTriangle :public SynthetizedView {
public:
	/**
	\brief Constructor
	@param color Color image
	@param depth_inverse Depth inverse
	@param mask_depth Mask indicating empty values on the depth (for example in a Kinect depth map)
	@param triangle_shape Quality map of each of the triangles
	@param size View final size
	*/
	SynthetizedViewTriangle(cv::Mat color, cv::Mat depth_inverse, cv::Mat mask_depth, cv::Mat triangle_shape, cv::Size size);
	using SynthetizedView::SynthetizedView;
	/**
	Compute the view by dividing the input view in triangles of 3 adjacent pixels, to avoid non disocclusion holes and get a first inpainting
	*/
	void compute(View& img);
	SynthetizedViewTriangle* copy() const;
	cv::Mat& get_triangle_shape() { return triangle_shape; };
	cv::Mat& get_quality() {quality = depth_inverse.mul(triangle_shape); return quality; };
private:
	cv::Mat triangle_shape;
	cv::Mat quality;
};

/**
 * The algorithm to compute the view maps every pixel to its new positions. The pixels are considered as squares.
 * The quality of each pixel is given by the depth of the pixel
 * */
class SynthetizedViewSquare :public SynthetizedView{
public:
	using SynthetizedView::SynthetizedView;
	/**
	Compute the view by warping every pixel: every pixel is considered as a rectangle
	*/
	void compute(View& img);
	cv::Mat& get_quality() { return depth_inverse; };
	SynthetizedViewSquare* copy() const;
private:
	int rescale_method_RGB;
	int rescale_method_Depth;
};
