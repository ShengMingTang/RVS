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

OMAF Referential coordinate system
Renamed from helpers.hpp to Parameters.hpp/cpp

Author  : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/


#pragma once

#include <opencv2/core.hpp>

/**\brief Coordinate system of the cameras configuration file*/
enum class CoordinateSystem
{
	VSRS,
	MPEG_I_OMAF,
	MPEG_H_3DAudio = MPEG_I_OMAF
};

/**
@file Parameters.hpp
\brief Definition of external and internal camera paramters
*/

/** Camera parameters*/
class Parameters {
public:
	Parameters();

	/** Camera parameters
	@param rotation External parameter of rotation
	@param translation External parameter of translation
	@param camera_matrix Internal parameters
	@param sensor Size of the sensor, in the same unit as camera_matrix
	@param system CoordinateSystem of those parameters
	*/
	Parameters(cv::Matx33f const& rotation, cv::Vec3f translation, cv::Matx33f const& camera_matrix, float sensor, CoordinateSystem system);

	/**External parameter of rotation*/
	cv::Matx33f const& get_rotation() const;

	/**External parameter of translation*/
	cv::Vec3f get_translation() const;

	/**Internal parameters*/
	cv::Matx33f const& get_camera_matrix() const;

	/**Size of the sensor, in the same unit as camera_matrix*/
	float get_sensor() const;

	/**
	@param relative_rotation
	*/
    void adapt_initial_rotation( const cv::Matx33f& relative_rotation )
    {
        rotation = relative_rotation * rotation0;
    }
	/**
	@param relative_translation
	*/
    void adapt_initial_translation( const cv::Vec3f& relative_translation )
    {
        translation = relative_translation + translation0;
    }

private:
	/**External parameter of rotation*/
	cv::Matx33f rotation0, rotation;

	/**External parameter of translation*/
	cv::Vec3f translation0, translation;

	/**Internal parameters*/
	cv::Matx33f camera_matrix;

	/**Size of the sensor, in the same unit as camera_matrix*/
	float sensor;
};
