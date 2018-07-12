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

#if WITH_OPENGL
#include "RFBO.hpp"

RFBO * RFBO::_singleton = nullptr;

void RFBO::toggle()
{
	validate();
	clear_working_buffers();
	value = (value + 1) % 2;
}

void RFBO::clear_working_buffers()
{
	validate();
	/* OpenGL 3.0+
	static const float transparent[] = { 0, 0, 0, 0 };
	glClearBufferfv(GL_COLOR, GL_COLOR_ATTACHMENT4 + 2 * value, transparent);
	glClearBufferfv(GL_COLOR, GL_COLOR_ATTACHMENT5 + 2 * value, transparent);
	*/
	// OpenGL 4.4+
	glClearTexImage(swap_image[value], 0, GL_RGB, GL_FLOAT, 0);
	glClearTexImage(swap_quality[value], 0, GL_RED, GL_FLOAT, 0);


	glClearTexImage(image, 0, GL_RGB, GL_FLOAT, 0);
	glClearTexImage(depth, 0, GL_RED, GL_FLOAT, 0);
	glClearTexImage(quality_triangle, 0, GL_RED, GL_FLOAT, 0);
}

void RFBO::clear_buffers()
{
	validate();
	// OpenGL 4.4+
	glClearTexImage(swap_image[0], 0, GL_RGB, GL_FLOAT, 0);
	glClearTexImage(swap_quality[0], 0, GL_RED, GL_FLOAT, 0);
	glClearTexImage(swap_image[1], 0, GL_RGB, GL_FLOAT, 0);
	glClearTexImage(swap_quality[1], 0, GL_RED, GL_FLOAT, 0);


	glClearTexImage(image, 0, GL_RGB, GL_FLOAT, 0);
	glClearTexImage(depth, 0, GL_RED, GL_FLOAT, 0);
	glClearTexImage(quality_triangle, 0, GL_RED, GL_FLOAT, 0);
}

GLuint RFBO::create_texture_buffer(cv::Size size, GLenum attachement_type, GLenum internalformat)
{
	validate();
	GLenum intf = GL_RED;
	if (internalformat == GL_RGB32F)
		intf = GL_RGB;

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, size.width, size.height, 0, intf, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glFramebufferTexture(GL_FRAMEBUFFER, attachement_type, texture, 0);

	return texture;
}
void RFBO::delete_texture_buffer(GLuint texture)
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &texture);
}

void RFBO::create_fbo(cv::Size size)
{
	setGLContext();
	glViewport(0, 0, size.width, size.height);

	glGenFramebuffers(1, &ID);
	glBindFramebuffer(GL_FRAMEBUFFER, ID);
	initialized = true;

	image = create_texture_buffer(size, GL_COLOR_ATTACHMENT0, GL_RGB32F);
	depth = create_texture_buffer(size, GL_COLOR_ATTACHMENT1, GL_R32F);
	quality_triangle = create_texture_buffer(size, GL_COLOR_ATTACHMENT2, GL_R32F);

	swap_image[0] = create_texture_buffer(size, GL_COLOR_ATTACHMENT3, GL_RGB32F);
	swap_quality[0] = create_texture_buffer(size, GL_COLOR_ATTACHMENT4, GL_R32F);

	swap_image[1] = create_texture_buffer(size, GL_COLOR_ATTACHMENT5, GL_RGB32F);
	swap_quality[1] = create_texture_buffer(size, GL_COLOR_ATTACHMENT6, GL_R32F);

	glGenRenderbuffers(1, &depth_stencil);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.width, size.height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil);

	GLuint attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 ,GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6 };
	glDrawBuffers(7, attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		initialized = false;
		printf("FATAL ERROR WHILE CREATING THE FRAMEBUFFER. %u\n", glGetError());
		exit(1);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void RFBO::free()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &ID);
	initialized = false;

	delete_texture_buffer(image);
	delete_texture_buffer(depth);
	delete_texture_buffer(quality_triangle);

	delete_texture_buffer(swap_image[0]);
	delete_texture_buffer(swap_quality[0]);

	delete_texture_buffer(swap_image[1]);
	delete_texture_buffer(swap_quality[1]);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glDeleteRenderbuffers(1, &depth_stencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 0, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);

	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

#endif
