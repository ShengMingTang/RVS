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

#if WITH_OPENGL
#include "helpersGL.hpp"

class RFBO
{
private:
	RFBO() {}
	~RFBO()
	{
		// TODO delete OpenGL Buffers	
		delete _singleton;
		initialized = false;
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
		if (!initialized)
			create_fbo(size);
		initialized = true;
	}

public:

	void toggle();

	void clear_working_buffers();

	void clear_buffers();

	void free();

private:
	GLuint create_texture_buffer(cv::Size size, GLenum attachement_type, GLenum internalformat);
	void RFBO::delete_texture_buffer(GLuint texture);

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

	GLint value = 0;

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
