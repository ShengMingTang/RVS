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

/**
@file PerspectiveProjector.hpp
*/
#pragma once
#include "Projector.hpp"
#include "Config.hpp"

/**\brief PerspectiveProjector*/
class PerspectiveProjector : public Projector
{
public:
	/**\brief Constructor
	@param parameters Parameters of the View
	@param size Size of the View
	*/
	PerspectiveProjector(Parameters const& parameters, cv::Size size);


	/**\brief Project from 3D to images coordinates and outputs a depth map

	world_pos in OMAF Referential: x forward, y left, z up
	depth [out] is equal to x
	result in image coordinates: u right, v down

	@param world_pos 3D coordinates of the pixels
	@param depth Output perpective depth map
	@param wrapping_method
	@return Map of the pixels in image coordinates
	*/
	cv::Mat2f project(cv::Mat3f world_pos, /*out*/ cv::Mat1f& depth, /*out*/ WrappingMethod& wrapping_method) const;

	/**\brief get the camera matrix
	@return Camera matrix
	*/
	cv::Matx33f const& get_camera_matrix() const;

private:
	Parameters parameters;
};
