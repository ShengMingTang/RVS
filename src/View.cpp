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

// Default constructor for use with std::vector
// Assertions check against using empty color/depth matrices
View::View() {}

// Construct a View by directly providing the data members
View::View(cv::Mat3f color, cv::Mat1f depth)
	: color(color)
	, depth(depth)
{}

// Return the texture
cv::Mat3f View::get_color() const
{
	assert(!color.empty() && color.size() == depth.size());
	return color; 
}

// Return the depth map (same size as texture)
cv::Mat1f View::get_depth() const 
{
	assert(!color.empty() && color.size() == depth.size());
	return depth;
}

// Return the size of the texture and depth map
cv::Size View::get_size() const
{
	assert(!color.empty() && color.size() == depth.size());
	return color.size(); 
}

// Return a mask with all valid depth values
cv::Mat1b View::get_depth_mask() const
{
	assert(!color.empty() && color.size() == depth.size());
	return depth > 0.f;
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
: View(
	read_color(filepath_color, size, bit_depth_color),
	read_depth(filepath_depth, size, bit_depth_depth, z_near, z_far))
{}