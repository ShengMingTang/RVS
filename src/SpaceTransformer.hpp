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
	ProjectionType get_projection_type() const { return projection_type; }
	char const* get_shader_name() const { return shader_name; };

	void set_targetPosition(Parameters params_virtual, cv::Size virtual_size, ProjectionType virtual_projection_type);
	void set_inputPosition(Parameters params_real, cv::Size input_size, ProjectionType input_projection_type);
	
private:
	ProjectionType projection_type;
	char const* shader_name;

};
