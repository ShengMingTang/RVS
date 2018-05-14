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

const cv::Mat P = (cv::Mat_<float>(3, 3) <<
	1, 0, 0,
	0, -1, 0,
	0, 0, -1);



struct Translation : cv::Vec3f{

	Translation() {
		this->operator()(0) = 0;
		this->operator()(1) = 0;
		this->operator()(2) = 0;
	}

	inline Translation(cv::Mat translation_matrix) {
		this->operator()(0) = translation_matrix.at<float>(0, 0);
		this->operator()(1) = translation_matrix.at<float>(1, 0);
		this->operator()(2) = translation_matrix.at<float>(2, 0);
	}

	inline Translation(const cv::Vec3f &t)
	    : cv::Vec3f(t)
	{		
	}

	inline Translation(const Translation & t)
	    : cv::Vec3f(t[0], t[1], t[2])
	{
	}

	inline Translation(const float x, const float y, const float z)
	    : cv::Vec3f(x, y, z)
	{
	}

	Translation operator+(const Translation &rhs)
	{
		return Translation(this->operator()(0) + rhs[0], this->operator()(1) + rhs[1], this->operator()(2) + rhs[2]);
	}

};

/** Camera parameters*/
struct Parameters {
	/**External parameter of rotation*/
	cv::Mat rotation;
	/**External parameter of translation*/
	Translation translation;
	/**Internal parameters*/
	cv::Mat camera_matrix;
	/**Size of the sensor, in the same unit as camera_matrix*/
	float sensor;

	Parameters() {}

	/** Camera parameters
	@param rotation External parameter of rotation
	@param translation External parameter of translation
	@param camera_matrix Internal parameters
	@param sensor_size Size of the sensor, in the same unit as camera_matrix
	*/
	inline Parameters(cv::Mat & rotation, cv::Vec3f translation, cv::Mat camera_matrix, float sensor_size)
		: rotation(P*rotation),
		translation(translation),
		camera_matrix(camera_matrix) {
		sensor = sensor_size;
	};

	inline float get_focal(void) const
	{
		return get_focal_x();
	}

	inline float get_focal_x(void) const
	{
		return camera_matrix.at<float>(0, 0);
	}

	inline float get_focal_y(void) const
	{
		return camera_matrix.at<float>(1, 1);
	}


};

