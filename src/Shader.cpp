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


#include "Shader.hpp"

#include <cassert>

#if WITH_OPENGL

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

	shaders["translate_rotate_Perspective"].vertex_source = R"(
#version 420 core

layout(location = 0) in float d;

out VS_OUT {
	vec2 uv;
	float depth;
} vs_out;

uniform vec3 translation;

uniform vec2 f;
uniform vec2 n_f;
uniform vec2 p;
uniform vec2 n_p;


uniform float sensor;
uniform float w;
uniform float h;
uniform float offset;

uniform mat3 R;

vec2 get_position_from_Vertex_ID(int id, float width) {
	int y = id / int(width);
	int x = id - y * int(width);
	return vec2(float(x), float(y));
}

void main(void)
{
	vec2 xy = get_position_from_Vertex_ID(gl_VertexID, w);
	float x = xy.x;
	float y = xy.y;
	float w_sensor = w/sensor;

	vec4 position;

	if (d - translation.z <= 0.0f || d == 0.0) {
		position = vec4(-1.0f, -1.0f, 0, 1.0f);
		gl_Position = position;
		vs_out.uv = position.xy;
		return;
	}
	//coordinates of the translated image in plane (x,y)
	float dispX = f.x / d * w_sensor;
	float dispY = f.y / d * w_sensor;
	float dx = dispX*translation.x;
	float dy = dispY*translation.y;
	float x2 = (x + offset) - dx;
	float y2 = (y + offset) - dy;
	//coordinate after z translation
	vec2 v = vec2((x2) / w_sensor - p.x, (y2) / w_sensor - p.y);
	float beta = (translation.z*v.x) / (f.x + -f.x*translation.z / d);
	float gamma = (translation.z*v.y) / (f.x + -f.x*translation.z / d);
	//new image with old focal length (optical center at (0,0))
	vec2 v2 = vec2(x2 + beta*dispX - p.x, y2 + gamma*dispY - p.y);
	//new image with new focal length (optical center at (0,0))
	v2 *= n_f.x / f.x;
	//coordinate in the new image
	float X = (v2.x + n_p.x*w_sensor);
	float Y = (v2.y + n_p.y*w_sensor);
	position = vec4(X, Y, 0.0f, 0.0f);

	//coordinate of the translated image 
	vec3 vt = vec3((position.x - n_p.x) / w, (position.y - n_p.y) / w, n_f.x / sensor);

	//coordinates of the rotated image		
	vec3 Vr = R*vt;
	Vr /= Vr[2];
	Vr*= n_f.x/sensor;
	float xi = Vr[0] * w + n_p.x;
	float yi = Vr[1] * w + n_p.y;
	//position = vec4(xi, yi, 0.0f, 0.0f);

	position = vec4((xi)*2.0f/w-1.0f, -(yi)*2.0f/h +1.0f, 0.0f, 1.0f);
	gl_Position = position;

	vs_out.uv = vec2(xy.x/w, (xy.y)/h);

	vs_out.depth = d;
}
)";

	shaders["translate_rotate_ERP"].vertex_source = R"(
#version 420 core

layout(location = 0) in float d;

out VS_OUT {
	vec2 uv;
	float depth;
} vs_out;

uniform vec3 translation;

uniform float rotation;
uniform float w;
uniform float h;
uniform float offset;

uniform mat3 R;
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

void main(void)
{
	//image coordinates
	vec2 xy = get_position_from_Vertex_ID(gl_VertexID, w);
	float x = xy.x;
	float y = xy.y;

	//spherical coordinates
	float full_width = max(w, 2.0f * h);
	float offset_w = (full_width - w) / 2.0f;
	vec2 phiTheta=vec2(2.0f*M_PI * ( 0.5f - (offset_w+offset+x) / full_width ),  M_PI * ( 0.5f - y / h ));

	//normalized euclidian coordinates
	vec3 eucl=calculate_euclidian_coordinates( phiTheta );

	//euclidian coordinates
	eucl = eucl*d;

	//transform
	eucl = R*eucl+translation;

	//normalize
	float n = length(eucl);
	eucl /= n;

	//spherical coordinates
	vec2 phiTheta2 = calculate_spherical_coordinates( eucl );

	//image coordinates (in [-1,1])
	float hic = 2.0f * full_width/w * ( 0.5f - phiTheta2.x / M_PI / 2.0f) - 1.0f - 4.0f*offset_w/full_width;
	float vic = 2.0f * phiTheta2.y / M_PI ;
	vec4 position = vec4(hic,vic,0.0f,1.0f);

	vs_out.uv = vec2(xy.x/w, xy.y/h);
	gl_Position = position;

	vs_out.depth = n;
}
)";

	shaders["translate_rotate_Perspective"].geometry_source = R"(
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

	float get_quality_old() {
		vec2 A = (gl_in[0].gl_Position).xy;
		vec2 B = (gl_in[1].gl_Position).xy;
		vec2 C = (gl_in[2].gl_Position).xy;

		float area = -((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));

		if (area < 0) { return 0.0f; };

		float dBA= dot((A-B), (A-B));
		float dBC= dot((C-B), (C-B));
		float dAC= dot((C-A), (C-A));

		float maximum = max(max(dBA, dBC), dAC);
		//return max(20000.f * area / maximum, 1.0f);

		// TODO Philips:
		// This quality works better with the Unicorn Dataset,
		// Could you look into this?
		float max_d = max(max(gs_in[0].depth,gs_in[1].depth),gs_in[2].depth);
		float min_d = min(min(gs_in[0].depth,gs_in[1].depth),gs_in[2].depth);

		return min(max(10000.0f-50000.0f*(max_d-min_d)/(max_depth-min_depth),1.0f),10000.f);

	}
	float get_quality() {
		vec2 A = (gl_in[0].gl_Position).xy;
		vec2 B = (gl_in[1].gl_Position).xy;
		vec2 C = (gl_in[2].gl_Position).xy;

		float area = -((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
		if (area < 0) { return 1.0f; };

		float dBA= dot((A-B), (A-B));
		float dBC= dot((C-B), (C-B));
		float dAC= dot((C-A), (C-A));

		float maximum = max(max(dBA, dBC), dAC);

		float quality = 10000.f - 1000.f * w*sqrt(maximum);
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
		float quality = get_quality_old();
		gen_vertex(0, quality);
		gen_vertex(1, quality);
		gen_vertex(2, quality);
		EndPrimitive();
	}

	)";
	shaders["translate_rotate_ERP"].geometry_source = shaders["translate_rotate_Perspective"].geometry_source;
	shaders["translate_rotate_Perspective"].fragment_source = R"(
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
	gl_FragDepth = ((gs_depth/max_depth)/*(gs_depth/max_depth)*(gs_depth/max_depth)*/)/gs_quality;
}
)";

	shaders["translate_rotate_ERP"].fragment_source = shaders["translate_rotate_Perspective"].fragment_source;
	shaders["translate_rotate_Perspective"].compile();
	shaders["translate_rotate_ERP"].compile();

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

	const auto blending_fragment_shader = R"(
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

	shaders["blending1"].fragment_source = std::string(R"(
#version 420 core
layout(location=3) out vec4 color;
layout(location=4) out float quality;
)") + std::string(blending_fragment_shader);

	shaders["blending1"].compile();


	shaders["blending2"].vertex_source = shaders["blending1"].vertex_source;

	shaders["blending2"].fragment_source = std::string(R"(
#version 420 core
layout(location=5) out vec4 color;
layout(location=6) out float quality;
)") + std::string(blending_fragment_shader);

	shaders["blending2"].compile();
}



#endif
