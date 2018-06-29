#pragma once

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

void RFBO::create_fbo(cv::Size size)
{
	setGLContext();
	glViewport(0, 0, size.width, size.height);

	glGenFramebuffers(1, &ID);
	glBindFramebuffer(GL_FRAMEBUFFER, ID);

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
		printf("FATAL ERROR WHILE CREATING THE FRAMEBUFFER. %u\n", glGetError());
		exit(1);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}


#endif
