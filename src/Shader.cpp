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

#include "Shader.hpp"

#include <cassert>

ShadersList * ShadersList::_singleton = nullptr;

void Shader::shader_compile_errors(const GLuint &object, const char * type) {
	std::string type_(type);
	GLint success;
	GLchar infoLog[1024];

	bool error = false;

	if (type_ != "Program") {
		glGetShaderiv(object, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(object, 1024, NULL, infoLog);
			printf("| ERROR::SHADER: Compile-time error -> Type: %s\n", type);
			error = true;
		}
	}
	else {
		glGetProgramiv(object, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(object, 1024, NULL, infoLog);
			printf("| ERROR::Shader: Link-time error -> Type: %s\n", type);
			error = true;
		}
	}

	if (error) {
		printf("%s\n", infoLog);
		printf("-- --------------------------------------------------- -- \n");
	}
}

const char * Shader::shader_type2string(GLenum shaderType) {
	std::string ret;

	switch (shaderType) {
	case GL_COMPUTE_SHADER:
		ret = "Compute";
		break;
	case GL_VERTEX_SHADER:
		ret = "Vertex";
		break;
	case GL_TESS_CONTROL_SHADER:
		ret = "Tessellation Control";
		break;
	case GL_TESS_EVALUATION_SHADER:
		ret = "Tessellation Evaluation";
		break;
	case GL_GEOMETRY_SHADER:
		ret = "Geometry";
		break;
	case GL_FRAGMENT_SHADER:
		ret = "Fragment";
		break;
	default:
		ret = "unknown";
		break;
	}
	return ret.c_str();
}

GLuint Shader::compile_shader(const std::string shader, GLenum shaderType)
{
	const GLchar * ccode = shader.c_str();

	const GLuint id = glCreateShader(shaderType);
	glShaderSource(id, 1, &ccode, NULL);
	glCompileShader(id);
	shader_compile_errors(id, shader_type2string(shaderType));

	return id;
}

void Shader::compile()
{
	assert(!vertex_source.empty() && !fragment_source.empty());

	vertexShader = compile_shader(vertex_source, GL_VERTEX_SHADER);
	fragmentShader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
	geometryShader = (!geometry_source.empty()) ? compile_shader(geometry_source, GL_GEOMETRY_SHADER) : 0;
	tessCShader = (!tessC_source.empty()) ? compile_shader(tessC_source, GL_TESS_CONTROL_SHADER) : 0;
	tessEShader = (!tessE_source.empty()) ? compile_shader(tessE_source, GL_TESS_EVALUATION_SHADER) : 0;

	GLuint local_ID = glCreateProgram();

	// Attach the shaders to the local_ID
	glAttachShader(local_ID, vertexShader);
	glAttachShader(local_ID, fragmentShader);
	if (!geometry_source.empty()) glAttachShader(local_ID, geometryShader);
	if (!tessC_source.empty()) glAttachShader(local_ID, tessCShader);
	if (!tessE_source.empty()) glAttachShader(local_ID, tessEShader);

	glLinkProgram(local_ID);
	shader_compile_errors(local_ID, "Program");

	ID = local_ID;
	printf("NEW PROGRAM WITH ID: %i\n", ID);

	// Delete the shaders as they're linked into our ID now and no longer necessery
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	if (!geometry_source.empty()) glDeleteShader(geometryShader);
	if (!tessC_source.empty()) glDeleteShader(tessCShader);
	if (!tessE_source.empty()) glDeleteShader(tessEShader);
}

GLuint Shader::program()
{
	if (ID > 0)
		return ID;
	return 0;
}

void ShadersList::init_shaders()
{
	shaders["synthesis"].vertex_source = R"(
#version 420 core

layout(location = 0) in float d;

out VS_OUT {
	vec2 uv;
	float depth;
} vs_out;

uniform vec3 translation;

uniform float w;
uniform float h;
uniform float offset;

uniform int erp_in;
uniform int erp_out;

uniform vec2 f;
uniform vec2 n_f;
uniform vec2 p;
uniform vec2 n_p;

uniform mat3 R;

uniform float max_depth;
vec3 calculate_euclidian_coordinates( vec2 phiTheta )
{
    float phi   = phiTheta.x;
    float theta = phiTheta.y;
	vec2 sc_phi = vec2(sin(phi),cos(phi));
	vec2 sc_theta = vec2(sin(theta),cos(theta));

    return vec3(  sc_phi.y * sc_theta.y,
                       sc_phi.x*sc_theta.y,
                       sc_theta.x );
}
vec2 calculate_spherical_coordinates( vec3 xyz_norm )
{
    float x = xyz_norm.x;
    float y = xyz_norm.y;
    float z = xyz_norm.z;

    float phi   = atan(y,x);
    float theta = asin( z );

    return vec2(phi, theta);
}
#define M_PI 3.1415926535897932384626433832795f

vec2 get_position_from_Vertex_ID(int id, float width) {
	int y = id / int(width);
	int x = id - y * int(width);
	return vec2(float(x), float(y));
}

vec3 unproject_erp(){
	//image coordinates
	vec2 xy = get_position_from_Vertex_ID(gl_VertexID, w);
	float x = xy.x;
	float y = xy.y;
	float full_width = max(w, 2.0f * h);
	float offset_w = (full_width - w) / 2.0f;

	//spherical coordinates
	vec2 phiTheta=vec2(2.0f*M_PI * ( 0.5f - (offset_w+offset+x) / full_width ),  M_PI * ( 0.5f - y / h ));

	//normalized euclidian coordinates
	vec3 eucl=calculate_euclidian_coordinates( phiTheta );

	//euclidian coordinates
	eucl = eucl*d;

	return eucl;
}
vec3 unproject_perspective(){
	//image coordinates
	vec2 xy = get_position_from_Vertex_ID(gl_VertexID, w);
	float x = xy.x;
	float y = xy.y;

	// OMAF Referential: x forward, y left, z up
	// Image plane: x right, y down
	//if (d > 0.f) {
		return vec3(d, -(d / f.x) * (x - p.x), -(d / f.y) * (y - p.y));
	//}
}
void project_erp(vec3 eucl){
	//image coordinates
	vec2 xy = get_position_from_Vertex_ID(gl_VertexID, w);
	float x = xy.x;
	float y = xy.y;
	float full_width = max(w, 2.0f * h);
	float offset_w = (full_width - w) / 2.0f;

	//normalize
	float n = length(eucl);
	eucl /= n;

	//spherical coordinates
	vec2 phiTheta2 = calculate_spherical_coordinates( eucl );

	//image coordinates (in [-1,1])
	float hic = 2.0f * full_width/w * ( 0.5f - phiTheta2.x / M_PI / 2.0f) - 1.0f - 4.0f*offset_w/full_width;
	float vic = 2.0f * phiTheta2.y / M_PI ;
	vec4 position = vec4(hic,vic,0.0f,1.0f);

	vs_out.uv = vec2(x/w, y/h);
	gl_Position = position;

	vs_out.depth = n;
}
void project_perspective(vec3 eucl){
	//image coordinates
	vec2 xy = get_position_from_Vertex_ID(gl_VertexID, w);
	float x = xy.x;
	float y = xy.y;

	//image coordinates (in [-1,1])
	vec2 uv = vec2 (-n_f.x * eucl.y / eucl.x + n_p.x, -n_f.y * eucl.z / eucl.x + n_p.y);

	gl_Position = vec4(2.0*uv.x/w-1.0, -2.0*uv.y/h+1.0, 0.f, 1.f);
	vs_out.uv = vec2(x/w, y/h);
	vs_out.depth = 1.0;
}

void main(void)
{
	vec3 eucl;
	if (erp_in == 1)
		eucl = unproject_erp();
	else
		eucl = unproject_perspective();
	//transform
	eucl = R*eucl+translation;
	if (erp_out == 1)
		project_erp(eucl);
	else
		project_perspective(eucl);
}

)";

	shaders["synthesis"].geometry_source = R"(
#version 420 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in VS_OUT {
	vec2 uv;
	float depth;
} gs_in[];

out vec2 gs_uv;
out float gs_quality;
out float gs_depth;

uniform float w;
uniform float max_depth;
uniform float min_depth;

//TODO Philips+ULB find the best quality possible (a combinaison of this 3 functions get_quality())

float get_quality_old() {//quality based on area/longuest side
	vec2 A = (gl_in[0].gl_Position).xy;
	vec2 B = (gl_in[1].gl_Position).xy;
	vec2 C = (gl_in[2].gl_Position).xy;

	float area = -((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));

	if (area < 0) { return 0.0f; };

	float dBA= dot((A-B), (A-B));
	float dBC= dot((C-B), (C-B));
	float dAC= dot((C-A), (C-A));

	float maximum = max(max(dBA, dBC), dAC);
	return max(20000.f * area / maximum, 1.0f);
}
float get_quality_test() {//based of the depth difference in the triangle
	vec2 A = (gl_in[0].gl_Position).xy;
	vec2 B = (gl_in[1].gl_Position).xy;
	vec2 C = (gl_in[2].gl_Position).xy;

	float area = -((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));

	if (area < 0) { return 0.0f; };

	float dBA= dot((A-B), (A-B));
	float dBC= dot((C-B), (C-B));
	float dAC= dot((C-A), (C-A));

	float maximum = max(max(dBA, dBC), dAC);
	// This quality works better with the Unicorn Dataset
	//TODO remove this comment
	float max_d = max(max(gs_in[0].depth,gs_in[1].depth),gs_in[2].depth);
	float min_d = min(min(gs_in[0].depth,gs_in[1].depth),gs_in[2].depth);

	return min(max(10000.0f-50000.0f*(max_d-min_d)/(min_depth),1.0f),10000.f);

}
float get_quality() { //quality based only on the longest side of the triangle
	vec2 A = (gl_in[0].gl_Position).xy;
	vec2 B = (gl_in[1].gl_Position).xy;
	vec2 C = (gl_in[2].gl_Position).xy;

	float area = -((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
	if (area < 0) { return 1.0f; };

	float dBA= dot((A-B), (A-B));
	float dBC= dot((C-B), (C-B));
	float dAC= dot((C-A), (C-A)); 

	float maximum = max(max(dBA, dBC), dAC);

	float quality = 10000.f / maximum / maximum;
	quality = max(1.f, quality); // always > 0
	quality = min(10000.f, quality);

	return quality;
}

void gen_vertex(int index, float quality) {
	gs_quality = quality;
	gs_uv = gs_in[index].uv;
	gs_depth = gs_in[index].depth;
	gl_Position = gl_in[index].gl_Position;
	EmitVertex();
}

void main() {
	// TODO change to the right quality function when the issue is closed
	float quality = sqrt(get_quality_test()*get_quality());
	gen_vertex(0, quality);
	gen_vertex(1, quality);
	gen_vertex(2, quality);
	EndPrimitive();
}


)";

	shaders["synthesis"].fragment_source = R"(
#version 420 core

uniform sampler2D image_texture;

in vec2 gs_uv;
in float gs_quality;
in float gs_depth;

layout(location=0) out vec3 color;
layout(location=1) out float depth;
layout(location=2) out float quality;


uniform float max_depth;

void main(void)
{
	color = texture(image_texture, gs_uv).rgb;
	depth = gs_depth;
	quality = gs_quality;
	// TODO: Check with quality
	//works ? with unicorn
	gl_FragDepth = ((gs_depth/max_depth)/*(gs_depth/max_depth)*(gs_depth/max_depth)*/)/gs_quality;
	//work with 360
	//gl_FragDepth = ((gs_depth/max_depth)*(gs_depth/max_depth)*(gs_depth/max_depth))/gs_quality;
}

)";

	shaders["blending1"].vertex_source = R"(
#version 420 core

// two triangles to form a quad
layout(location = 0) in vec2 position;

out vec2 vs_position;

void main(void)
{
	vs_position = (1.0f+position.xy)/2.0f;
	gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);
}

)";

	shaders["blending1"].fragment_source = R"(
#version 420 core
layout(location=3) out vec4 color;
layout(location=4) out float quality;
uniform sampler2D accumulator_image;
uniform sampler2D new_image;

uniform sampler2D accumulator_quality;

uniform sampler2D new_triangle_quality;

uniform sampler2D new_depth;

uniform float blending_factor;

in vec2 vs_position;

void main(void)
{
	float accu_quality = texture(accumulator_quality, vs_position).x;

	float n_triangle_quality = texture(new_triangle_quality, vs_position).x;
	float n_depth = texture(new_depth, vs_position).x;

	vec4 accu_color = texture(accumulator_image, vs_position);
	vec4 n_color = texture(new_image, vs_position);

	float accu_w;
	float new_w;
	if(blending_factor > 0.5f)
	{
		accu_w = pow(accu_quality, blending_factor);
		new_w = pow(n_triangle_quality/n_depth, blending_factor);
	}
	else{
		accu_w = 1.0;
		new_w = 1.0;
	}
	if (n_depth == 0.0f || n_triangle_quality == 0.0f)
		new_w=0.0f;
	if (accu_quality == 0.0f)
		accu_w=0.0f;

	vec4 tmp_color = accu_w*accu_color + new_w*n_color;
	if (accu_w + new_w >0.0f)
		tmp_color /= (accu_w + new_w);
	color = tmp_color;

	float tmp_quality = accu_w + new_w;
	if (tmp_quality > 0.0f)
	{
		if(blending_factor > 0.5f)
			{
				quality = pow(tmp_quality, 1.0f/blending_factor);
			}
			else{
				quality = tmp_quality;
			}
	}
	else 
		quality = 0.0f;
}
)";
	
	shaders["blending2"].vertex_source = shaders["blending1"].fragment_source;

	shaders["blending2"].fragment_source = R"(
#version 420 core
layout(location=5) out vec4 color;
layout(location=6) out float quality;
uniform sampler2D accumulator_image;
uniform sampler2D new_image;

uniform sampler2D accumulator_quality;

uniform sampler2D new_triangle_quality;

uniform sampler2D new_depth;

uniform float blending_factor;

in vec2 vs_position;

void main(void)
{
	float accu_quality = texture(accumulator_quality, vs_position).x;

	float n_triangle_quality = texture(new_triangle_quality, vs_position).x;
	float n_depth = texture(new_depth, vs_position).x;

	vec4 accu_color = texture(accumulator_image, vs_position);
	vec4 n_color = texture(new_image, vs_position);

	float accu_w;
	float new_w;
	if(blending_factor > 0.5f)
	{
		accu_w = pow(accu_quality, blending_factor);
		new_w = pow(n_triangle_quality/n_depth, blending_factor);
	}
	else{
		accu_w = 1.0;
		new_w = 1.0;
	}
	if (n_depth == 0.0f || n_triangle_quality == 0.0f)
		new_w=0.0f;
	if (accu_quality == 0.0f)
		accu_w=0.0f;

	vec4 tmp_color = accu_w*accu_color + new_w*n_color;
	if (accu_w + new_w >0.0f)
		tmp_color /= (accu_w + new_w);
	color = tmp_color;

	float tmp_quality = accu_w + new_w;
	if (tmp_quality > 0.0f)
	{
		if(blending_factor > 0.5f)
			{
				quality = pow(tmp_quality, 1.0f/blending_factor);
			}
			else{
				quality = tmp_quality;
			}
	}
	else 
		quality = 0.0f;
}
)";

	shaders["synthesis"].compile();
	shaders["blending1"].compile();
	shaders["blending2"].compile();
}
