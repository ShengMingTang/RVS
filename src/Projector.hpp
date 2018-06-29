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
@file Projector.hpp
*/

#pragma once
#include "Parameters.hpp"

#include <opencv2/core.hpp>

/**\brief Wrapping method*/
enum class WrappingMethod {
    NONE = 0,
    HORIZONTAL = 1
};

/**\brief Projector.

Unroject the pixels from euclidian coordinate system to image space and depth map.*/
class Projector
{
public:
	/**\brief Constructor*/
    Projector();

	/**\brief Constructor
	@param parameters Parameters of the View
	@param size Size of the View 
	*/
    Projector(Parameters const&, cv::Size);

	/**\brief Destructor*/
	virtual ~Projector();

	/** 
	@param world_pos in OMAF Referential: x forward, y left, z up
	@param[out] depth increases with distance from virtual camera
	@param[out] wrapping_method Equirectangular or Perspective
	@return Result in image coordinates: u right, v down
	 */
	virtual cv::Mat2f project(cv::Mat3f world_pos, /*out*/ cv::Mat1f& depth, /*out*/ WrappingMethod& wrapping_method) const = 0;

	/**@return Size of the virtual view in pixels*/
	cv::Size get_size() const;

	/**\brief Return the camera matrix
	@return The camera matrix. If we don't have one, we return an eye matrix.
	*/
	virtual cv::Matx33f const& get_camera_matrix() const;

private:
	cv::Size size;
};
