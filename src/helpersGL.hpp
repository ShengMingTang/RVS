#pragma once

#ifdef WITH_OPENGL

#include "gl_core_4.5.hpp"
#include "Config.hpp"
#include <opencv2/core/mat.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// OpenGL Utilities

template<int rows, int cols>
bool fromCV2GLM(const cv::Mat& cvmat, glm::mat<cols, rows, float, glm::packed_highp>* glmmat) {
	assert(cvmat.cols == cols && cvmat.rows == rows && cvmat.type() == CV_32FC1);

	memcpy(glm::value_ptr(*glmmat), cvmat.data, rows*cols * sizeof(float));
	*glmmat = glm::transpose(*glmmat);
	return true;
}

template<int rows, int cols>
bool fromGLM2CV(const glm::mat<cols, rows, float, glm::packed_highp>& glmmat, cv::Mat* cvmat) {
	if (cvmat->cols != cols || cvmat->rows != rows) {
		(*cvmat) = cv::Mat(rows, cols, 4, CV_32F);
	}
	memcpy(cvmat->data, glm::value_ptr(glmmat), rows*cols * sizeof(float));
	*cvmat = cvmat->t();
	return true;
}

GLuint cvMat2glTexture(const cv::Mat& mat);

// END OpenGL Utilities

struct VAO_VBO_EBO {
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;
	size_t number_of_elements;
	
	VAO_VBO_EBO(const cv::Mat& depth, cv::Size size)
	{
		//use fast 4-byte alignment (default anyway) if possible
		glPixelStorei(GL_UNPACK_ALIGNMENT, (depth.step & 3) ? 1 : 4);

		//set length of one complete row in data (doesn't need to equal image.cols)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, depth.step / depth.elemSize());

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glBufferData(GL_ARRAY_BUFFER, depth.rows*depth.cols*depth.elemSize(), depth.ptr(), GL_STATIC_DRAW);

		generate_picture_EBO(size, number_of_elements);

		GLuint num_components_per_vertex = 1; // depth
		glVertexAttribPointer(0, num_components_per_vertex, GL_FLOAT, GL_FALSE,
			1 * sizeof(GLfloat),
			(GLvoid*)0);
		glEnableVertexAttribArray(0);


		glBindVertexArray(0);

	}

	~VAO_VBO_EBO() {
		glDeleteBuffers(1, &EBO);
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);
	}

#define INDEX_E(x,y,W) ((y)*(W) + (x))
	void generate_picture_EBO(const cv::Size &s, size_t &number_of_elements)
	{
		const int W = s.width;
		const int H = s.height;

		number_of_elements = 3 * 2 * (H - 1)*(W - 1);
		std::vector<GLuint> indices;
		indices.resize(number_of_elements);

		size_t offset = 0;
		for (size_t y = 0; y < H - 1; ++y)
		{
			for (size_t x = 0; x < W - 1; ++x)
			{
				indices[3 * INDEX_E(x, y, (W - 1)) + 0] = INDEX_E(x, y, W);
				indices[3 * INDEX_E(x, y, (W - 1)) + 1] = INDEX_E(x + 1, y, W);
				indices[3 * INDEX_E(x, y, (W - 1)) + 2] = INDEX_E(x, y + 1, W);
				offset += 3;
			}
		}

		for (size_t y = 1; y < H; ++y)
		{
			for (size_t x = 0; x < W - 1; ++x)
			{
				indices[offset + 3 * INDEX_E(x, y - 1, (W - 1)) + 0] = INDEX_E(x, y, W);
				indices[offset + 3 * INDEX_E(x, y - 1, (W - 1)) + 1] = INDEX_E(x + 1, y - 1, W);
				indices[offset + 3 * INDEX_E(x, y - 1, (W - 1)) + 2] = INDEX_E(x + 1, y, W);
			}
		}

		glGenBuffers(1, &EBO);
		// Bind index buffer to corresponding target
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		// ititialize index buffer, allocate memory, fill it with data
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, number_of_elements * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

		printf("Real number of elements %i\n", int(number_of_elements));
	}

};




#endif // WITH_OPENGL

#ifdef WITH_OPENGL
#define NOMINMAX
#include <Windows.h>
// NEVER WRITE ON IT.
struct context_NO_WRITE_H
{
#if _WIN32
	HWND fakewindow = nullptr;
	HDC gldc = nullptr;
	HGLRC glrc = nullptr;
#else
	Display * disp = nullptr;
	Window win = 0;
	GLXContext ctx = 0;
#endif
};

extern context_NO_WRITE_H context_NO_WRITE;

#ifdef SVS_DEBUG
#include "renderdoc_app.h"
extern RENDERDOC_API_1_1_2 *rdoc_api;
#endif

#endif

void context_init();
void setGLContext();

void rd_start_capture_frame();
void rd_end_capture_frame();
