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

#include "SynthesizedView.hpp"
#include "transform.hpp"
#include "Timer.hpp"

#include <algorithm>
#include <iostream>

extern bool g_with_opengl;
#if WITH_OPENGL
#include "helpersGL.hpp"
#include "RFBO.hpp"
#endif

#define DUMP_VALUES !defined NDEBUG

#if DUMP_VALUES
const int DUMP_I = 540;
const int DUMP_J = 960;
#endif

namespace
{
	cv::Mat2f uvCoordinates(cv::Size size)
	{
		auto result = cv::Mat2f(size);

		for (int i = 0; i != result.rows; ++i) {
			for (int j = 0; j != result.cols; ++j) {
				result(i, j) = cv::Vec2f(j + 0.5f, i + 0.5f);
			}
		}

		return result;
	}

	// Affine transformation: x -> Rx + t
	cv::Mat3f affine_transform(cv::Mat3f x, cv::Matx33f R, cv::Vec3f t)
	{
		auto y = cv::Mat3f(x.size());

		for (int i = 0; i != y.rows; ++i) {
			for (int j = 0; j != y.cols; ++j) {
				y(i, j) = R * x(i, j) + t;
			}
		}

		return y;
	}
}

SynthesizedView::SynthesizedView() {}

SynthesizedView::~SynthesizedView() {}

void SynthesizedView::compute(View& input)
{
	assert(m_space_transformer);
	PROF_START("warping");

	auto R = m_space_transformer->get_rotation();
	auto t = m_space_transformer->get_translation();

#if WITH_OPENGL
	if (g_with_opengl) {
		auto ogl_transformer = static_cast<const OpenGLTransformer*>(m_space_transformer);
		GLuint image_texture = cvMat2glTexture(input.get_color());

		const float offset = 0.0; // Center of a pixel
		const float sensor = ogl_transformer->get_sensor_size();
		auto FBO = RFBO::getInstance();
		auto & shaders = *(ShadersList::getInstance());

		float w = float(input.get_depth().cols);
		float h = float(input.get_depth().rows);

		//rotation and translation
		glm::mat3x3 Rt(0);
		glm::vec3 translation;		
		translation= glm::vec3(t[0], t[1], t[2]);//OMAF
		fromCV2GLM<3, 3>(cv::Mat(R), &Rt);//OMAF
		

		cv::Mat old_cam_mat = cv::Mat(ogl_transformer->get_input_camera_matrix());
		cv::Mat new_cam_mat = cv::Mat(ogl_transformer->get_output_camera_matrix());
		glm::vec2 f(old_cam_mat.at<float>(0, 0), old_cam_mat.at<float>(1, 1));
		glm::vec2 p(old_cam_mat.at<float>(0, 2), h - old_cam_mat.at<float>(1, 2));
		glm::vec2 n_f(new_cam_mat.at<float>(0, 0), new_cam_mat.at<float>(1, 1));
		glm::vec2 n_p(new_cam_mat.at<float>(0, 2), h - new_cam_mat.at<float>(1, 2));


		const VAO_VBO_EBO vve(input.get_depth(), input.get_depth().size());

		GLuint program = shaders(ogl_transformer->get_shader_name()).program();
		assert(program != 0);

		glEnable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_FRAMEBUFFER, FBO->ID);
		//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(program);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, image_texture);
		glUniform1i(glGetUniformLocation(program, "image_texture"), 0);
		// parameters

		glUniformMatrix3fv(glGetUniformLocation(program, "R"), 1, GL_FALSE, glm::value_ptr(Rt));
		glUniform3fv(glGetUniformLocation(program, "translation"), 1, glm::value_ptr(translation));
		glUniform1f(glGetUniformLocation(program, "w"), w);
		glUniform1f(glGetUniformLocation(program, "h"), h);
		glUniform1f(glGetUniformLocation(program, "offset"), offset);
		glUniform1f(glGetUniformLocation(program, "max_depth"), input.get_max_depth());
		glUniform1f(glGetUniformLocation(program, "min_depth"), input.get_min_depth());

		//only usefull for perpespective shader
		glUniform2fv(glGetUniformLocation(program, "f"), 1, glm::value_ptr(f));
		glUniform2fv(glGetUniformLocation(program, "n_f"), 1, glm::value_ptr(n_f));
		glUniform2fv(glGetUniformLocation(program, "p"), 1, glm::value_ptr(p));
		glUniform2fv(glGetUniformLocation(program, "n_p"), 1, glm::value_ptr(n_p));
		glUniform1f(glGetUniformLocation(program, "sensor"), sensor);


		//only usefull for erp shader
		glUniform1i(glGetUniformLocation(program, "erp_in"), ogl_transformer->get_input_projection_type());
		glUniform1i(glGetUniformLocation(program, "erp_out"), ogl_transformer->get_output_projection_type());

		// end parameters

		glBindVertexArray(vve.VAO);
		printf("Number of elements %i\n", int(vve.number_of_elements));
		glDrawElements(GL_TRIANGLES, int(vve.number_of_elements), GL_UNSIGNED_INT, nullptr);
		glUseProgram(0);
		glBindVertexArray(0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glDisable(GL_DEPTH_TEST);
		
		glDeleteTextures(GLsizei(1), &image_texture);
	}
#endif
	if (!g_with_opengl) {
		auto pu_transformer = static_cast<const PUTransformer*>(m_space_transformer);

		// Generate image coordinates 
		auto input_size = input.get_size();
		auto input_uv = uvCoordinates(input_size); // TODO: Move into PerspectiveUnproject
		
												   // Unproject: input view image to input view world coordinates
		auto input_xyz = pu_transformer->unproject(input_uv, input.get_depth());
		
		// Rotate and translate from input (real) to output (virtual) view
		auto virtual_xyz = affine_transform(input_xyz, R, t);

		// Project: output view world to output view image coordinates
		cv::Mat1f virtual_depth; // Depth 
		WrappingMethod wrapping_method;
		auto virtual_uv = pu_transformer->project(virtual_xyz, /*out*/ virtual_depth, /*out*/ wrapping_method);

		// Resize: rasterize with oversampling
		auto virtual_size = pu_transformer->get_size();
		auto output_size = cv::Size(
			int(0.5f + virtual_size.width * g_rescale),
			int(0.5f + virtual_size.height * g_rescale));
		cv::Mat2f scaled_uv;
		cv::transform(virtual_uv, scaled_uv, cv::Matx22f(
			float(output_size.width) / virtual_size.width, 0.f,
			0.f, float(output_size.height) / virtual_size.height));

		// Rasterization results in a color, depth and quality map
		transform(input.get_color(), scaled_uv, virtual_depth, output_size, wrapping_method);

#if DUMP_VALUES
		std::clog << "(i, j) == (" << DUMP_I << ", " << DUMP_J << ")" << '\n';
		std::clog << "input_uv(i, j) == " << input_uv(DUMP_I, DUMP_J) << '\n';
		std::clog << "input_get_depth()(i, j) == " << input.get_depth()(DUMP_I, DUMP_J) << '\n';
		std::clog << "input_xyz(i, j) == " << input_xyz(DUMP_I, DUMP_J) << '\n';
		std::clog << "R == " << R << '\n';
		std::clog << "t == " << t << '\n';
		std::clog << "virtual_xyz(i, j) == " << virtual_xyz(DUMP_I, DUMP_J) << '\n';
		std::clog << "virtual_uv(i, j) == " << virtual_uv(DUMP_I, DUMP_J) << '\n';
		std::clog << "virtual_depth(i, j) == " << virtual_depth(DUMP_I, DUMP_J) << '\n';
		std::clog << "scaled_uv(i, j) == " << scaled_uv(DUMP_I, DUMP_J) << std::endl;
#endif // DUMP_VALUES
	}

	PROF_END("warping");
}

SynthetisedViewTriangle::SynthetisedViewTriangle() {}

void SynthetisedViewTriangle::transform(cv::Mat3f input_color, cv::Mat2f input_positions, cv::Mat1f input_depth, 
    cv::Size output_size, WrappingMethod wrapping_method)
{
	cv::Mat1f depth;
	cv::Mat1f validity;

    bool wrapHorizontal = wrapping_method == WrappingMethod::HORIZONTAL ? true : false; 

    auto color = transform_trianglesMethod(input_color, input_depth, input_positions, output_size, /*out*/ depth, /*out*/ validity, wrapHorizontal);
	
	assign(color, depth, validity / depth, validity);
}