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
