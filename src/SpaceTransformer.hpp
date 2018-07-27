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

#pragma once
#include "Projector.hpp"
#include "Unprojector.hpp"
#include "Config.hpp"

#include "opencv2/imgproc.hpp"

#include <memory>

class SpaceTransformer
{
public:
	SpaceTransformer();
	virtual ~SpaceTransformer();
	cv::Vec3f get_translation() const;
	cv::Matx33f get_rotation() const;

	virtual void set_targetPosition(Parameters params_virtual, cv::Size virtual_size, ProjectionType virtual_projection_type) = 0;
	virtual void set_inputPosition(Parameters params_real, cv::Size input_size, ProjectionType input_projection_type) = 0;
	float get_sensor_size() const { return sensor_size; };
	cv::Size get_size() const { return size; };
protected:
	float sensor_size;
	cv::Size size;
	Parameters input_param;
	Parameters output_param;
};

class PUTransformer : public SpaceTransformer 
{
public:
	cv::Mat2f project(cv::Mat3f world_pos, /*out*/ cv::Mat1f& depth, /*out*/ WrappingMethod& wrapping_method) const
	{ return projector->project(world_pos, depth, wrapping_method); };
	cv::Mat3f unproject(cv::Mat2f image_pos, cv::Mat1f depth) const
	{ return unprojector->unproject(image_pos, depth); };

	void set_targetPosition(Parameters params_virtual, cv::Size virtual_size, ProjectionType virtual_projection_type);
	void set_inputPosition(Parameters params_real, cv::Size input_size, ProjectionType input_projection_type);

private:

	// Unprojector converts input view to world coordinates
	std::unique_ptr<Unprojector> unprojector = nullptr;

	// Projector converts world to virtual view coordinates
	std::unique_ptr<Projector> projector = nullptr;
};

#include "Shader.hpp"
class OpenGLTransformer : public SpaceTransformer
{
public:
	OpenGLTransformer();

	cv::Matx33f get_input_camera_matrix() const;
	cv::Matx33f get_output_camera_matrix() const;
	ProjectionType get_input_projection_type() const { return input_projection_type; }
	ProjectionType get_output_projection_type() const { return output_projection_type; }
	char const* get_shader_name() const { return shader_name; };

	void set_targetPosition(Parameters params_virtual, cv::Size virtual_size, ProjectionType virtual_projection_type);
	void set_inputPosition(Parameters params_real, cv::Size input_size, ProjectionType input_projection_type);
	
private:
	ProjectionType input_projection_type;
	ProjectionType output_projection_type;
	char const* shader_name;

};
