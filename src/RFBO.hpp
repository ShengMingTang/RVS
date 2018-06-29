#pragma once

#if WITH_OPENGL
#include "helpersGL.hpp"

class RFBO
{
private:
	RFBO() {}
	~RFBO()
	{
		// TODO delete OpenGL Buffers	
	}

public:
	static RFBO * getInstance()
	{
		if (_singleton == nullptr)
			_singleton = new RFBO();
		return _singleton;
	}

	void init(cv::Size size)
	{
		initialized = true;
		create_fbo(size);
	}

public:

	void toggle();

	void clear_working_buffers();

	void clear_buffers();

private:
	GLuint create_texture_buffer(cv::Size size, GLenum attachement_type, GLenum internalformat);

	void create_fbo(cv::Size size);

public:
	GLuint ID = 0;

	// Input image
	GLuint image = 0;
	GLuint depth = 0;
	GLuint quality_triangle = 0;

	// Swapping images blending
	GLuint swap_image[2] = { 0, 0 };
	GLuint swap_quality[2] = { 0, 0 };

	GLuint depth_stencil = 0;

	size_t value = 0;

private:
	void validate(void) const
	{
		if (!initialized)
			throw std::logic_error("Framebuffer Object not initialized.");
	}


private:
	static RFBO * _singleton;
	bool initialized = false;
};

#endif
