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

#include "View.hpp"
#include "Parser.hpp"
#include "inpainting.hpp"
#include "image_loading.hpp"

#include <iostream>

// Initialize all maps at once
View::View(cv::Mat3f color, cv::Mat1f depth, cv::Mat1f quality)
{
	assign(color, depth, quality);
}

// Initialize all maps at once
void View::assign(cv::Mat3f color, cv::Mat1f depth, cv::Mat1f quality)
{
	_color = color;
	_depth = depth;
	_quality = quality;
	validate();
}

// Return the texture
cv::Mat3f View::get_color() const
{
	validate();
	CV_Assert(!_color.empty());
	return _color;
}

// Return the depth map (same size as texture)
cv::Mat1f View::get_depth() const 
{
	validate();
	CV_Assert(!_depth.empty());
	return _depth;
}

// Return the quality map (same size as texture)
cv::Mat1f View::get_quality() const
{
	validate();
	CV_Assert(!_quality.empty());
	return _quality;
}

// Return the size of the texture and depth map
cv::Size View::get_size() const
{
	validate();
	return _color.size();
}

// Return a mask with all valid depth values
cv::Mat1b View::get_depth_mask() const
{
	return get_depth() > 0.f; // excludes NaN's
}

// Calculate a mask for inpainting
cv::Mat1b View::get_inpaint_mask() const
{
	auto inpaint_mask = cv::Mat1b(get_size(), 255);
	inpaint_mask.setTo(0, get_quality() > 0); // excludes NaN's
	return inpaint_mask;
}

void View::validate() const
{
	auto size = _color.size();
	CV_Assert(_depth.empty() || _depth.size() == size);
	CV_Assert(_quality.empty() || _quality.size() == size);
}

InputView::InputView() {}

// Load a color image and depth map
InputView::InputView(
	std::string filepath_color,
	std::string filepath_depth,
	cv::Size size,
	int bit_depth_color,
	int bit_depth_depth,
	float z_near,
	float z_far)
{
	assign(
		read_color(filepath_color, size, bit_depth_color),
		read_depth(filepath_depth, size, bit_depth_depth, z_near, z_far),
		cv::Mat1f());
}