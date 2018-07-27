/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2010-2018, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
Original authors:

Universite Libre de Bruxelles, Brussels, Belgium:
  Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be  Daniele Bonatto, Daniele.Bonatto@ulb.ac.be  Arnaud Schenkel, arnaud.schenkel@ulb.ac.be

Koninklijke Philips N.V., Eindhoven, The Netherlands:
  Bart Kroon, bart.kroon@philips.com
  Bart Sonneveldt, bart.sonneveldt@philips.com
*/

#ifndef _PARAMETERS_HPP_
#define _PARAMETERS_HPP_

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

#endif