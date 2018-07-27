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
  Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be
  Daniele Bonatto, Daniele.Bonatto@ulb.ac.be
  Arnaud Schenkel, arnaud.schenkel@ulb.ac.be

Koninklijke Philips N.V., Eindhoven, The Netherlands:
  Bart Kroon, bart.kroon@philips.com
  Bart Sonneveldt, bart.sonneveldt@philips.com
*/

#include "Parameters.hpp"

#include <stdexcept>

Parameters::Parameters()
	: m_sensor(std::numeric_limits<float>::quiet_NaN()) 
{}

/** Camera parameters
All the parameters are internally stocked in the OMAF coordinate system
@param rotation External parameter of rotation
@param translation External parameter of translation
@param camera_matrix Internal parameters
@param sensor_size Size of the sensor, in the same unit as camera_matrix
*/
Parameters::Parameters(cv::Matx33f const& rotation, cv::Vec3f translation, cv::Matx33f const& camera_matrix, float sensor, CoordinateSystem system)
	: m_camera_matrix(camera_matrix)
	, m_sensor(sensor)
{
	if (system == CoordinateSystem::MPEG_I_OMAF) {
		// This is the internal coordinate system, so accept extrinsics without transformation
		this->m_rotation = rotation;
		this->m_translation = translation;
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

		this->m_rotation = P * rotation * P.t();
		this->m_translation = P * translation;
		this->m_rotation0 = this->m_rotation;
		this->m_translation0 = this->m_translation;

	}
	else throw std::logic_error("Unknown coordinate system");
}

cv::Matx33f const& Parameters::get_rotation() const {
	assert(m_sensor > 0.f); 
	return m_rotation; 
}

cv::Vec3f Parameters::get_translation() const {
	assert(m_sensor > 0.f); 
	return m_translation; 
}

cv::Matx33f const& Parameters::get_camera_matrix() const {
	assert(m_sensor > 0.f);
	return m_camera_matrix; 
}

float Parameters::get_sensor() const {
	assert(m_sensor > 0.f);
	return m_sensor; 
}
