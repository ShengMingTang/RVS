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

#include "SpaceTransformer.hpp"
#include "EquirectangularProjection.hpp"
#include "PerspectiveProjector.hpp"
#include "PerspectiveUnprojector.hpp"


SpaceTransformer::SpaceTransformer()
{
}


SpaceTransformer::~SpaceTransformer()
{
}

OpenGLTransformer::OpenGLTransformer()
{
}

cv::Vec3f SpaceTransformer::get_translation() const
{
	//auto R_from = unprojector->get_rotation();
	auto t_from = input_param.get_translation();
	auto R_to = output_param.get_rotation();
	auto t_to = output_param.get_translation();

	auto t = -R_to.t()*(t_to - t_from);

	return t;
}

cv::Matx33f SpaceTransformer::get_rotation() const
{
	auto R_from = input_param.get_rotation();
	auto R_to = output_param.get_rotation();

	auto R = R_to.t()*R_from;

	return R;
}

cv::Matx33f OpenGLTransformer::get_input_camera_matrix() const
{
	return input_param.get_camera_matrix();
}

cv::Matx33f OpenGLTransformer::get_output_camera_matrix() const
{
	return output_param.get_camera_matrix();
}

void OpenGLTransformer::set_targetPosition(Parameters params_virtual, cv::Size virtual_size, ProjectionType virtual_projection_type)
{
	this->output_param = params_virtual;
	this->size = virtual_size;
	this->output_projection_type = virtual_projection_type;
	//uncomment to use the shader without reprojection
	/*if (virtual_projection_type == PROJECTION_PERSPECTIVE)
		this->shader_name = "translate_rotate_Perspective";
	else if (virtual_projection_type == PROJECTION_EQUIRECTANGULAR)*/
		this->shader_name = "translate_rotate_ERP";
}

void OpenGLTransformer::set_inputPosition(Parameters params_real, cv::Size /*input_size*/, ProjectionType input_projection_type_)
{
	this->input_param = params_real;
	this->sensor_size = params_real.get_sensor();
	this->input_projection_type = input_projection_type_;
	if (input_projection_type_ != this->output_projection_type) {
		this->shader_name = "translate_rotate_ERP";
	}
}

void PUTransformer::set_targetPosition(Parameters params_virtual, cv::Size virtual_size, ProjectionType virtual_projection_type)
{
	if (virtual_projection_type == PROJECTION_PERSPECTIVE)
		this->projector.reset(new PerspectiveProjector(params_virtual, virtual_size));
	else if (virtual_projection_type == PROJECTION_EQUIRECTANGULAR)
		this->projector.reset(new erp::Projector(params_virtual, virtual_size));
	this->size = virtual_size;
	this->output_param = params_virtual;
}

void PUTransformer::set_inputPosition(Parameters params_real, cv::Size input_size, ProjectionType input_projection_type)
{
	if (input_projection_type == PROJECTION_PERSPECTIVE)
		this->unprojector.reset(new PerspectiveUnprojector(params_real));
	else if (input_projection_type == PROJECTION_EQUIRECTANGULAR)
		this->unprojector.reset(new erp::Unprojector(params_real, input_size));

	this->input_param = params_real;
	this->sensor_size = params_real.get_sensor();
}
