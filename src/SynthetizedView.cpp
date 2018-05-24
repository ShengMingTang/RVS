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

This source file has been added by Koninklijke Philips N.V. for the purpose of
of the 3DoF+ Investigation.
Modifications copyright © 2018 Koninklijke Philips N.V.

Extraction of a generalized unproject -> translate/rotate -> project flow

Author  : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

#include "SynthetizedView.hpp"
#include "transform.hpp"
#include "Timer.hpp"

#include <algorithm>
#include <iostream>

namespace
{
	cv::Mat2f uvCoordinates(cv::Size size)
	{
		auto result = cv::Mat2f(size);

		for (int i = 0; i != result.rows; ++i) {
			for (int j = 0; j != result.cols; ++j) {
				result(i, j) = cv::Vec2f(i + 0.5f, j + 0.5f);
			}
		}

		return result;
	}

	// Affine transformation: x -> Rx + t
	cv::Mat3f affine_transform(cv::Mat3f x, cv::Matx33f R, cv::Vec3f t)
	{
		auto y = cv::Mat3f(x.size());

		for (int i = 0; i != y.rows; ++i) {
			for (int j = 0; j != y.cols; ++j) {
				y(i, j) = R * x(i, j) + t;
			}
		}

		return y;
	}
}

VirtualView::VirtualView() {}

VirtualView::VirtualView(cv::Mat3f color, cv::Mat1f depth, cv::Mat1f quality)
	: View(color, depth)
	, quality(quality)
{
	assert(!color.empty() && color.size() == depth.size() && depth.size() == quality.size());
}

VirtualView::~VirtualView() {}

SynthetizedView::SynthetizedView() {}

SynthetizedView::~SynthetizedView() {}

void SynthetizedView::compute(View& input)
{
	assert(projector && unprojector);
	PROF_START("warping");

	// Generate image coordinates 
	// BK: Probably specific to Triangle/Square so need to extend class interface
	auto input_size = input.get_size();
	auto input_uv = uvCoordinates(input_size);

	// Unproject: input view image to input view world coordinates
	auto input_xyz = unprojector->unproject(input_uv, input.get_depth());

	// Combine rotations and translations of input and virtual views
	auto rotation = projector->get_rotation().inv()*unprojector->get_rotation();
	auto translation = projector->get_rotation().inv()*(unprojector->get_translation() - projector->get_translation());

	// Rotate and translate from input (real) to output (virtual) view
	auto virtual_xyz = affine_transform(input_xyz, rotation, translation);

	// Project: output view world to output view image coordinates
	cv::Mat1f virtual_depth; // Depth 
	auto virtual_uv = projector->project(virtual_xyz, /*out*/ virtual_depth);

	// Resize: rasterize with oversampling
	auto output_size = cv::Size(
		int(0.5f + input_size.width * rescale),
		int(0.5f + input_size.height * rescale));
	cv::transform(virtual_uv, virtual_uv, cv::Matx22f(
		float(output_size.width) / input_size.width, 0.f,
		0.f, float(output_size.height) / input_size.height));

    // Rasterization results in a color, depth and quality map
	transform(input.get_color(), virtual_uv, virtual_depth, output_size);
	assert(!color.empty() && color.size() == depth.size() && color.size() == quality.size());
	
	PROF_END("warping");
}

SynthetizedViewTriangle::SynthetizedViewTriangle() {}

void SynthetizedViewTriangle::transform(cv::Mat3f input_color, cv::Mat2f input_positions, cv::Mat1f input_depth, cv::Size output_size)
{
	color = transform_trianglesMethod(input_color, input_depth, input_positions, output_size, /*out*/ depth, /*out*/ quality);
}

SynthetizedViewSquare::SynthetizedViewSquare() {}

void SynthetizedViewSquare::transform(cv::Mat3f input_color, cv::Mat2f input_positions, cv::Mat1f input_depth, cv::Size output_size)
{
	color = transform_squaresMethod(input_color, input_depth, input_positions, output_size, /*out*/ depth, /*out*/ quality);
}
