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
#ifdef WITH_OPENGL

#include "gl_core_4.5.hpp"
#include <string>

#include "helpersGL.hpp"

#include <map>

class Shader
{
public:
	std::string vertex_source;
	std::string fragment_source;
	std::string geometry_source;
	std::string tessC_source;
	std::string tessE_source;

private:
	GLuint vertexShader = 0;
	GLuint fragmentShader = 0;
	GLuint geometryShader = 0;
	GLuint tessCShader = 0;
	GLuint tessEShader = 0;

	GLuint ID = 0;


private:
	void shader_compile_errors(const GLuint &object, const char * type);

	const char * shader_type2string(GLenum shaderType);
	GLuint compile_shader(const std::string shader, GLenum shaderType);

public:
	void compile();

	const GLuint program();

};

class ShadersList
{
private:
	ShadersList() { init_shaders(); }
	~ShadersList()
	{
		// TODO delete OpenGL Buffers	
	}

public:
	static ShadersList * getInstance()
	{
		if (_singleton == nullptr)
			_singleton = new ShadersList();
		return _singleton;
	}

public:
	void init_shaders();


	Shader & operator() (const char * shader_name) {
		return shaders[shader_name];
	}

private:

public:

private:
	std::map<std::string, Shader> shaders;


private:
	static ShadersList * _singleton;
};




#endif
