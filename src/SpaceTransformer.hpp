#pragma once
#include "Projector.hpp"
#include "Unprojector.hpp"
#include "Config.hpp"

#include "opencv2/imgproc.hpp"

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
	Unprojector const *unprojector = nullptr;

	// Projector converts world to virtual view coordinates
	Projector const *projector = nullptr;
};

#include "Shader.hpp"
class OpenGLTransformer : public SpaceTransformer
{
public:
	OpenGLTransformer();

	cv::Matx33f get_input_camera_matrix() const;
	cv::Matx33f get_output_camera_matrix() const;
	ProjectionType get_projection_type() const { return projection_type; }
	char* get_shader_name() const { return shader_name; };

	void set_targetPosition(Parameters params_virtual, cv::Size virtual_size, ProjectionType virtual_projection_type);
	void set_inputPosition(Parameters params_real, cv::Size input_size, ProjectionType input_projection_type);
	
private:
	ProjectionType projection_type;
	char* shader_name;

};