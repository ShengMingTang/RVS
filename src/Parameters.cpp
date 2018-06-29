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

#include "Parameters.hpp"

#include <stdexcept>

Parameters::Parameters()
	: sensor(std::numeric_limits<float>::quiet_NaN()) 
{}

/** Camera parameters
All the parameters are internally stocked in the OMAF coordinate system
@param rotation External parameter of rotation
@param translation External parameter of translation
@param camera_matrix Internal parameters
@param sensor_size Size of the sensor, in the same unit as camera_matrix
*/
Parameters::Parameters(cv::Matx33f const& rotation, cv::Vec3f translation, cv::Matx33f const& camera_matrix, float sensor, CoordinateSystem system)
	: camera_matrix(camera_matrix)
	, sensor(sensor)
{
	if (system == CoordinateSystem::MPEG_I_OMAF) {
		// This is the internal coordinate system, so accept extrinsics without transformation
		this->rotation = rotation;
		this->translation = translation;
	}
	else if (system == CoordinateSystem::VSRS) {
		// Affine transformation: x --> R^T (x - t)
		// But now "x" is OMAF Referential: x forward, y left, z up,
		// and "t" and "R" has VSRS system: x right, y down, z forward
		// We need a P such that x == P x_VSRS:
		// x --> P R_VSRS^T P^T (x - P t_VSRS)

		//   right   down    forward
		auto P = cv::Matx33f(
			0.f, 0.f, 1.f,	// forward
			-1.f, 0.f, 0.f,   // left
			0.f, -1.f, 0.f);  // up

		this->rotation = P * rotation * P.t();
		this->translation = P * translation;
		this->rotation0 = this->rotation;
		this->translation0 = this->translation;

	}
	else throw std::logic_error("Unknown coordinate system");
}

cv::Matx33f const& Parameters::get_rotation() const {
	assert(sensor > 0.f); 
	return rotation; 
}

cv::Vec3f Parameters::get_translation() const {
	assert(sensor > 0.f); 
	return translation; 
}

cv::Matx33f const& Parameters::get_camera_matrix() const {
	assert(sensor > 0.f);
	return camera_matrix; 
}

float Parameters::get_sensor() const {
	assert(sensor > 0.f);
	return sensor; 
}
