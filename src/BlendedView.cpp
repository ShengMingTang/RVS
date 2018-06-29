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

This source file has been modified by Koninklijke Philips N.V. for the purpose of
of the 3DoF+ Investigation.
Modifications copyright © 2018 Koninklijke Philips N.V.

Extraction of a generalized unproject -> translate/rotate -> project flow

Author  : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

/*------------------------------------------------------------------------------ -

This source file has been modified by Université Libre de Bruxelles(ULB) for the purpose of
adding GPU acceleration through OpenGL.
Modifications copyright © 2018 Université Libre de Bruxelles(ULB)

Authors : Daniele Bonatto, Sarah Fachada
Contact : Gauthier.Lafruit@ulb.ac.be

------------------------------------------------------------------------------ -*/

#include "BlendedView.hpp"
#include "blending.hpp"
#include "Timer.hpp"

#include "iostream"

extern bool with_opengl;
#if WITH_OPENGL
#include "helpersGL.hpp"
#include "RFBO.hpp"
#endif

BlendedView::~BlendedView() {}

BlendedViewMultiSpec::BlendedViewMultiSpec(float exp_low_freq, float exp_high_freq)
	: low_freq(exp_low_freq)
	, high_freq(exp_high_freq)
{}

BlendedViewMultiSpec::~BlendedViewMultiSpec() {}

void BlendedViewMultiSpec::blend(View const& view)
{
	// Split color frequencies in low and high band
	cv::Mat3f low_color;
	cv::Mat3f high_color;
	auto mask = cv::Mat1b(view.get_quality() > 0.f);
	split_frequencies(view.get_color(), low_color, high_color, mask);

	// Repack as views
	auto low_view = View(low_color, view.get_depth(), view.get_quality(), view.get_validity());
	auto high_view = View(high_color, view.get_depth(), view.get_quality(), view.get_validity());

	// Blend low and high frequency separately
	low_freq.blend(low_view);
	high_freq.blend(high_view);

	// Combine result
	assign(low_freq.get_color() + high_freq.get_color(), cv::Mat1f(), low_freq.get_quality(), low_freq.get_validity());
}

BlendedViewSimple::BlendedViewSimple(float blending_exp)
	: is_empty(true)
	, blending_exp(blending_exp)
{
	assign(cv::Mat3f(), cv::Mat1f(), cv::Mat1f(), cv::Mat1f());
}

BlendedViewSimple::~BlendedViewSimple() {}

void BlendedViewSimple::blend(View const& view)
{ 
	if (WITH_OPENGL && with_opengl) {
#if WITH_OPENGL
		PROF_START("BLENDING_OPENGL");

		auto FBO = RFBO::getInstance();
		auto & shaders = *(ShadersList::getInstance());
		// QUAD VAO
		GLfloat quadVertices[] = {
			-1.0f, 1.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f,
			1.0f, -1.0f, 1.0f, 0.0f,
			-1.0f, 1.0f, 0.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 0.0f,
			1.0f, 1.0f, 1.0f, 1.0f };
		GLuint quadVAO, quadVBO;
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
			4 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
			4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
		glBindVertexArray(0);
		// END QUAD VAO

		GLuint program = shaders("blending2").program();
		if (FBO->value) program = shaders("blending1").program();

		assert(program != 0);

		glBindFramebuffer(GL_FRAMEBUFFER, FBO->ID);
		//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(program);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBO->image);
		glUniform1i(glGetUniformLocation(program, "new_image"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBO->depth);
		glUniform1i(glGetUniformLocation(program, "new_depth"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBO->quality_triangle);
		glUniform1i(glGetUniformLocation(program, "new_triangle_quality"), 2);

		glActiveTexture(GL_TEXTURE3 + 2 * FBO->value);
		glBindTexture(GL_TEXTURE_2D, FBO->swap_image[FBO->value]);
		glUniform1i(glGetUniformLocation(program, "accumulator_image"), 3 + 2 * FBO->value);

		glActiveTexture(GL_TEXTURE4 + 2 * FBO->value);
		glBindTexture(GL_TEXTURE_2D, FBO->swap_quality[FBO->value]);
		glUniform1i(glGetUniformLocation(program, "accumulator_quality"), 4 + 2 * FBO->value);

		// parameters
		glUniform1f(glGetUniformLocation(program, "blending_factor"), blending_exp);
		// end parameters

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glUseProgram(0);
		glBindVertexArray(0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		PROF_END("BLENDING_OPENGL");
		FBO->toggle();
#endif
	}
	else {
		if (is_empty) {
			is_empty = false;
			assign(view.get_color(), cv::Mat1b(), view.get_quality(), view.get_validity());
			_depth_mask = view.get_depth_mask();
		}
		else {
			std::vector<cv::Mat> colors = { get_color(), view.get_color() };
			std::vector<cv::Mat> qualities = { get_quality(), view.get_quality() };
			std::vector<cv::Mat> depth_masks = { _depth_mask, view.get_depth_mask() };

			cv::Mat quality;
			cv::Mat depth_prolongation_mask;
			cv::Mat inpaint_mask; // don't care

			auto color = blend_img(
				colors, qualities, depth_masks,
				empty_rgb_color,
				/*out*/ quality,
				/*out*/ depth_prolongation_mask,
				/*out*/ inpaint_mask,
				blending_exp);

			assign(color, cv::Mat1f(), quality, max(get_validity(), view.get_validity()));
			_depth_mask = depth_prolongation_mask;
		}
	}
}

#if WITH_OPENGL
void BlendedView::assignFromGL2CV(cv::Size size)
{
	auto FBO = RFBO::getInstance();
	cv::Mat3f img(size, CV_32FC3);

	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glPixelStorei(GL_PACK_ROW_LENGTH, img.step / img.elemSize());

	glBindFramebuffer(GL_FRAMEBUFFER, FBO->ID);
	glReadBuffer(GL_COLOR_ATTACHMENT3 + 2 * FBO->value);
	glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_FLOAT, img.ptr());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	cv::flip(img, img, 0);

	cv::Mat validity = cv::Mat::ones(size, CV_32FC1);

	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glPixelStorei(GL_PACK_ROW_LENGTH, validity.step / validity.elemSize());

	glBindFramebuffer(GL_FRAMEBUFFER, FBO->ID);
	glReadBuffer(GL_COLOR_ATTACHMENT4 + 2 * (FBO->value));
	glReadPixels(0, 0, validity.cols, validity.rows, GL_RED, GL_FLOAT, validity.ptr());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	cv::flip(validity, validity, 0);

	assign(img, cv::Mat1f(), validity, validity);
	FBO->clear_buffers();
}
#endif
