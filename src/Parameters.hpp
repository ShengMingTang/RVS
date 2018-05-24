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
@file helpers.hpp
\brief Definition of external and internal camera paramters
*/

/** Camera parameters*/
struct Parameters {
	/**External parameter of rotation*/
	cv::Matx33f rotation;

	/**External parameter of translation*/
	cv::Vec3f translation;

	/**Internal parameters*/
	cv::Matx33f camera_matrix;

	/**Size of the sensor, in the same unit as camera_matrix*/
	float sensor;

	Parameters() {}

	/** Camera parameters
	@param rotation External parameter of rotation
	@param translation External parameter of translation
	@param camera_matrix Internal parameters
	@param sensor_size Size of the sensor, in the same unit as camera_matrix
	*/
	Parameters(cv::Matx33f const& rotation, cv::Vec3f translation, cv::Matx33f const& camera_matrix, float sensor)
		: rotation(rotation)
		, translation(translation)
		, camera_matrix(camera_matrix)
		, sensor(sensor)
	{}
};
