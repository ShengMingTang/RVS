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

This source file has been modified by Université Libre de Bruxelles(ULB) for the purpose of
adding GPU acceleration through OpenGL.
Modifications copyright © 2018 Université Libre de Bruxelles(ULB)

Authors : Daniele Bonatto, Sarah Fachada
Contact : Gauthier.Lafruit@ulb.ac.be

------------------------------------------------------------------------------ -*/


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

	// TODO the rotation is still not the same in 360 config files and in ULB unicorn
	auto t = -R_to.t()*(t_to - t_from);

	//VSRS style
	//auto t = -R_to * (t_to - t_from);
	return t;
}

cv::Matx33f SpaceTransformer::get_rotation() const
{
	auto R_from = input_param.get_rotation();
	auto R_to = output_param.get_rotation();

	// TODO the rotation is still not the same in 360 config files and in ULB unicorn
	auto R = R_to.t()*R_from;

	//VSRS style
	//auto R = R_to * R_from.t();
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
	this->projection_type = virtual_projection_type;
	if (virtual_projection_type == PROJECTION_PERSPECTIVE)
		this->shader_name = "translate_rotate_Perspective";
	else if (virtual_projection_type == PROJECTION_EQUIRECTANGULAR)
		this->shader_name = "translate_rotate_ERP";
}

void OpenGLTransformer::set_inputPosition(Parameters params_real, cv::Size /*input_size*/, ProjectionType input_projection_type)
{
	this->input_param = params_real;
	this->sensor_size = params_real.get_sensor();
	if (input_projection_type != projection_type)
		throw std::runtime_error("Different input/output projection types not implemented in OpenGL!");
}

void PUTransformer::set_targetPosition(Parameters params_virtual, cv::Size virtual_size, ProjectionType virtual_projection_type)
{
	if (virtual_projection_type == PROJECTION_PERSPECTIVE)
		this->projector = new PerspectiveProjector(params_virtual, virtual_size);
	else if (virtual_projection_type == PROJECTION_EQUIRECTANGULAR)
		this->projector = new erp::Projector(params_virtual, virtual_size);
	this->size = virtual_size;
	this->output_param = params_virtual;
}

void PUTransformer::set_inputPosition(Parameters params_real, cv::Size input_size, ProjectionType input_projection_type)
{
	if (input_projection_type == PROJECTION_PERSPECTIVE)
		this->unprojector = new PerspectiveUnprojector(params_real);
	else if (input_projection_type == PROJECTION_EQUIRECTANGULAR)
		this->unprojector = new erp::Unprojector(params_real, input_size);

	this->input_param = params_real;
	this->sensor_size = params_real.get_sensor();
}
