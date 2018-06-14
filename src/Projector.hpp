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

#pragma once
#include "Parameters.hpp"

#include <opencv2/core.hpp>

enum class WrappingMethod {
    NONE = 0,
    HORIZONTAL = 1
};

class Projector
{
public:
    Projector();
    Projector(Parameters const&, cv::Size);
	virtual ~Projector();

	// world_pos in OMAF Referential: x forward, y left, z up
	// depth [out] increases with distance from virtual camera
	// result in image coordinates: u right, v down
	virtual cv::Mat2f project(cv::Mat3f world_pos, /*out*/ cv::Mat1f& depth, /*out*/ WrappingMethod& wrapping_method) const = 0;

	// Virtual view rotation matrix (like in VSRS camparams)
	cv::Matx33f const& get_rotation() const;

	// Virtual view translation vector (like in VSRS camparams)
	cv::Vec3f get_translation() const;

	// Size of the virtual view in pixels
	cv::Size get_size() const;

private:
	cv::Matx33f rotation;
	cv::Vec3f translation;
	cv::Size size;
};
