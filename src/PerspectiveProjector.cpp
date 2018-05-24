/* ------------------------------------------------------------------------------ -

Copyright © 2018 Koninklijke Philips N.V.

Authors : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

SVS 3DoF+
For the purpose of the 3DoF+ Investigation, SVS is extended to match with the
description of the Reference View Synthesizer(RVS) of the 3DoF+ part of the CTC.
This includes support for unprojecting and projecting ERP images, reading and
writing of 10 - bit YUV 4:2 : 0 texture and depth, parsing of JSON files according to
the 3DoF + CfTM, and unit / integration tests.

Permission is hereby granted, free of charge, to the members of the Moving Picture
Experts Group(MPEG) obtaining a copy of this software and associated documentation
files(the "Software"), to use the Software exclusively within the framework of the
MPEG - I(immersive) and MPEG - I Visual activities, for the sole purpose of
developing the MPEG - I standard.This permission explicitly excludes the rights
to publish, distribute, sublicense, sell, embed into a product or a service and / or
otherwise commercially exploit copies of the Software without the written consent
of the owner(Koninklijke Philips N.V.).

This permission is provided subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies, substantial portions or derivative works of the Software.

------------------------------------------------------------------------------ - */

#include "PerspectiveProjector.hpp"

#include <limits>
auto const NaN = std::numeric_limits<float>::quiet_NaN();

PerspectiveProjector::PerspectiveProjector(Parameters const& parameters)
	: Projector(parameters)
	, parameters(parameters)
{
}

cv::Mat2f PerspectiveProjector::project(cv::Mat3f world_pos, cv::Mat1f& depth) const
{
	assert(world_pos.size() == depth.size());

	if (world_pos.cols != parameters.sensor)
		throw std::runtime_error("Situation where sensor size is different from input view width is currently not supported");

	auto fx = parameters.camera_matrix(0, 0);
	auto fy = parameters.camera_matrix(1, 1);
	auto px = parameters.camera_matrix(0, 2);
	auto py = parameters.camera_matrix(1, 2);

	cv::Mat2f image_pos(world_pos.size(), cv::Vec2f::all(NaN));
	depth = cv::Mat1f(world_pos.size(), NaN);

	for (int i = 0; i != world_pos.rows; ++i) {
		for (int j = 0; j != world_pos.cols; ++j) {
			auto xyz = world_pos(i, j);

			if (xyz[0] > 0.f) {
				image_pos(i, j) = cv::Vec2f(
					px - fx * xyz[1] / xyz[0],
					py - fy * xyz[2] / xyz[0]);
				depth(i, j) = xyz[0];
			}
		}
	}

	return image_pos;
}
