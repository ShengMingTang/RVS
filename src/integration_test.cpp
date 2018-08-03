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

// NOTE: Integration tests require the following files to be downloaded and unpacked
//       (or symlinked) in the root of the source tree:
//
//   *  Plane_B'_Kinect.zip from ULB and on the MPEG content server
//   *  Plane_B'_Kinect-10bit.tar.gz from Philips to be uploaded

#include "yaffut.hpp"

#include "Pipeline.hpp"

#include <array>
#include <fstream>

#include <opencv2/imgproc.hpp>

#if WITH_OPENGL
#include "helpersGL.hpp"
#endif

namespace testing
{
	template<typename T> std::array<cv::Mat_<T>, 3> readYUV420(char const *filepath, cv::Size size)
	{
		// Open the file stream
		std::ifstream stream;
		stream.exceptions(std::ios::badbit | std::ios::failbit);
		stream.open(filepath, std::ios::binary);

		// Test file length is correct
		stream.seekg(0, std::ios::end);
		auto filesize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0, std::ios::beg);
		auto framesize = size.area() * 3 / 2 * sizeof(T);
		std::cout << "size == " << size << ", sizeof(T) == " << sizeof(T) << ", filesize == " << filesize << ", framesize == " << framesize << std::endl;
		YAFFUT_CHECK(filesize && filesize % framesize == 0);

		// Allocate planes
		cv::Mat_<T> Y(size);
		cv::Mat_<T> Cb(size / 2);
		cv::Mat_<T> Cr(size / 2);

		// Read the frame
		stream.read(reinterpret_cast<char*>(Y.data), size.area() * sizeof(T));
		stream.read(reinterpret_cast<char*>(Cb.data), size.area() / 4 * sizeof(T));
		stream.read(reinterpret_cast<char*>(Cr.data), size.area() / 4 * sizeof(T));

		return{ Y, Cb, Cr };
	}

	// Compare two YUV420 frames and check PSNR is below a threshold	
	// Two regions are taken because errors are large at the image borders
	template<typename T> void compareWithReferenceView(
		std::array<cv::Mat_<T>, 3> const& actual,
		std::array<cv::Mat_<T>, 3> const& reference,
		int bits, double threshold0, double threshold1)
	{
		YAFFUT_CHECK(actual[0].size() == reference[0].size());
		YAFFUT_CHECK(actual[1].size() == reference[1].size());
		YAFFUT_CHECK(actual[2].size() == reference[2].size());

		auto size = actual[0].size();
		double psnr[2];

		for (int i_region = 0; i_region != 2; ++i_region) {
			auto sum_sq_error = 0.;
			std::size_t num_values = 0;
			auto num_planes = 3;

			for (int i_plane = 0; i_plane != num_planes; ++i_plane) {
				auto plane_size = i_plane ? size / 2 : size;

				cv::Range cols, rows;
				switch (i_region) {
				case 0: 
					cols = cv::Range(0, plane_size.width);
					rows = cv::Range(0, plane_size.height);
					break;

				case 1:
					cols = cv::Range(plane_size.width / 3, plane_size.width * 2 / 3);
					rows = cv::Range(plane_size.height / 3, plane_size.height * 2 / 3);
					break;
				}

				auto sqrt_sum_sq_error = norm(
					actual[i_plane](rows, cols),
					reference[i_plane](rows, cols));

				num_values += cols.size() * rows.size();
				sum_sq_error += sqrt_sum_sq_error * sqrt_sum_sq_error;
			}

			auto mean_sq_error = sum_sq_error / num_values;
			auto max_level = (1 << bits) - 1;
			psnr[i_region] = 10. * std::log10(max_level * max_level / mean_sq_error);
		}

		std::cout << "PSNRs: " << psnr[0] << ", " << psnr[1] << std::endl;
		YAFFUT_CHECK(psnr[0] > threshold0);
		YAFFUT_CHECK(psnr[1] > threshold1);

		if (psnr[0] > threshold0 + 0.1 && threshold0 < 100. ||
			psnr[1] > threshold1 + 0.1 && threshold1 < 100.) {
			std::clog << "WARNING: thresholds can be increased\n";
		}
	}

	template<typename T> void compareWithReferenceView(
		char const *filepath_actual,
		char const *filepath_reference,
		cv::Size size, int bits,
		double threshold0, double threshold1)
	{
		std::cout << "Comparing \"" << filepath_actual << "\" " << size << " with \"" << filepath_reference << "\" " << size << std::endl;
		auto actual = readYUV420<T>(filepath_actual, size);
		auto reference = readYUV420<T>(filepath_reference, size);
		compareWithReferenceView(actual, reference, bits, threshold0, threshold1);
	}

	inline bool file_exists(const std::string& name) {
		std::ifstream f(name.c_str());
		return f.good();
	}

}

FUNC(ULB_Unicorn_Example)
{
	g_with_opengl = false;
	Pipeline p("./config_files/example_config_file.cfg");
	p.execute();
	// No reference
}

FUNC(ULB_Unicorn_Triangles_Simple)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/Unicorn_Triangles_Simple.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_Simple.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		cv::Size(1920, 1080), 8, 20.32, 24.54); // VC14 + OpenCV 3.1.0: 20.377, 24.5955
}

FUNC(ULB_Unicorn_Triangles_MultiSpectral)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/Unicorn_Triangles_MultiSpectral.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_MultiSpectral.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		cv::Size(1920, 1080), 8, 20.30, 24.35); // VC14 + OpenCV 3.1.0: 20.3568, 24.4012
}

FUNC(ULB_Unicorn_Same_View)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/Unicorn_Same_View.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250506_from_030003250506_Same_View.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0506.yuv",
		cv::Size(1920, 1080), 8, 19.80, 34.03); // VC14 + OpenCV 3.1.0: 19.8518, 34.0898
}

FUNC(ClassroomVideo_v0_to_v0)
{
	g_with_opengl = false;
	std::cout << '\n';
	Pipeline p("./config_files/_integration_tests/ClassroomVideo-SVS-v0_to_v0.json");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>( // texture
		"v0vs_4096_2048_420_10b.yuv",
		"ClassroomVideo/v0_4096_2048_420_10b.yuv",
		cv::Size(4096, 2048), 10, 70.00, 78.52); // VC14 + OpenCV 3.1.0: 70.0586, 78.572
	testing::compareWithReferenceView<std::uint16_t>( // depth
		"v0vs_4096_2048_0_8_1000_0_420_10b.yuv",
		"ClassroomVideo/v0_4096_2048_0_8_1000_0_420_10b.yuv",
		cv::Size(4096, 2048), 10, 100, 100.); // VC14 + OpenCV 3.1.0: 45.2363, inf
}

FUNC(ClassroomVideo_v7v8_to_v0)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/ClassroomVideo-SVS-v7v8_to_v0.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"v0vs_from_v7v8_4096_2048_420_10b.yuv",
		"ClassroomVideo/v0_4096_2048_420_10b.yuv",
		cv::Size(4096, 2048), 10, 35.54, 36.05); // VC14 + OpenCV 3.1.0: 35.5957, 36.1051
}

FUNC(ClassroomVideo_v7v8_to_v0_270deg)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/ClassroomVideo-SVS-v7v8_to_v0_270deg.cfg");
	p.execute();

	auto actual = testing::readYUV420<std::uint16_t>("v0_270deg_from_v7v8_2304_1536_420_10b.yuv", cv::Size(2304, 1536));

	auto reference = testing::readYUV420<std::uint16_t>("ClassroomVideo/v0_4096_2048_420_10b.yuv", cv::Size(4096, 2048));
	cv::resize(reference[0], reference[0], cv::Size(3072, 1536));
	cv::resize(reference[1], reference[1], cv::Size(1536, 768));
	cv::resize(reference[2], reference[2], cv::Size(1536, 768));
	reference[0] = reference[0].colRange(384, 2688);
	reference[1] = reference[1].colRange(192, 1344);
	reference[2] = reference[2].colRange(192, 1344);

	testing::compareWithReferenceView<std::uint16_t>(actual, reference, 10, 38.02, 38.36); // VC14 + OpenCV 3.1.0: 38.0782, 38.417
}

FUNC(TechnicolorHijack_v1v4_to_v9)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/TechnicolorHijack-SVS-v1v4_to_v9.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
	"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b.yuv",
	"TechnicolorHijack/v9_4096_4096_420_10b.yuv",
	cv::Size(4096, 4096), 10, 43.71, 35.88); // VC14 + OpenCV 3.1.0: 43.7695, 35.9381
}

FUNC(TechnicolorHijack_BlendByMax)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/TechnicolorHijack-BlendByMax.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorHijack_BlendByMax.yuv",
		"TechnicolorHijack/v9_4096_4096_420_10b.yuv",
		cv::Size(4096, 4096), 10, 43.19, 35.28); // VC14 + OpenCV 3.1.0: 43.2489, 35.3323
}

FUNC(TechnicolorMuseum_v0v2v13v17v19_to_v1)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/TechnicolorMuseum-SVS-v0v2v13v17v19_to_v1.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v1_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 31.76, 38.15); // VC14 + OpenCV 3.1.0: 31.8129, 38.2094
}

FUNC(TechnicolorMuseum_v0_to_v0)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/TechnicolorMuseum-SVS-v0_to_v0.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v0vs_from_v0_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v0_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 38.30, 38.95); // VC14 + OpenCV 3.1.0: 38.358, 39.0078
}

FUNC(TechnicolorMuseum_v5_to_v5)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/TechnicolorMuseum-SVS-v5_to_v5.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v5vs_from_v5_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v5_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 56.74, 73.73); // VC14 + OpenCV 3.1.0: 56.7996, 73.7879
}

FUNC(TechnicolorMuseum_v5_to_v6)
{
	g_with_opengl = false;
	Pipeline p("./config_files/_integration_tests/TechnicolorMuseum-SVS-v5_to_v6.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v6vs_from_v5_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v6_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 22.37, 26.54); // VC14 + OpenCV 3.1.0: 22.4218, 26.5975
}

#if WITH_OPENGL
FUNC(ULB_Unicorn_Triangles_Simple_OpenGL)
{

	if (!testing::file_exists("030003250438_from_030003070370_030003430506_Triangles_Simple.yuv")) {
		g_with_opengl = false;
		Pipeline p("./config_files/_integration_tests/Unicorn_Triangles_Simple.cfg");
		p.execute();
	}

	g_with_opengl = true;
	context_init();
	Pipeline pgl("./config_files/_integration_tests/Unicorn_Triangles_Simple_OpenGL.cfg");
	pgl.execute();

	//No opengl vs ref
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_Simple.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		cv::Size(1920, 1080), 8, 20.32, 20.54); // VC14 + OpenCV 3.1.0: 20.377, 24.5955
												//Opengl vs ref
	testing::compareWithReferenceView<std::uint8_t>(
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		"030003250438_from_030003070370_030003430506_Triangles_Simple_OpenGL.yuv",
		cv::Size(1920, 1080), 8, 20.32, 24.49); // VC14 + OpenCV 3.1.0: 20.377, 24.5955
												//no opengl vs opengl
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_Simple.yuv",
		"030003250438_from_030003070370_030003430506_Triangles_Simple_OpenGL.yuv",
		cv::Size(1920, 1080), 8, 20.0, 20.54); // VC14 + OpenCV 3.1.0: 20.377, 24.5955

}

FUNC(ClassroomVideo_v7v8_to_v0_OpenGL)
{
	if (!testing::file_exists("v0vs_from_v7v8_4096_2048_420_10b.yuv")) {
		g_with_opengl = false;
		Pipeline p("./config_files/_integration_tests/ClassroomVideo-SVS-v7v8_to_v0.cfg");
		p.execute();
	}

	g_with_opengl = true;
	context_init();
	Pipeline pGL("./config_files/_integration_tests/ClassroomVideo-SVS-v7v8_to_v0_OpenGL.cfg");
	pGL.execute();

	//opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"ClassroomVideo/v0_4096_2048_420_10b.yuv",
		"v0vs_from_v7v8_4096_2048_420_10b_OpenGL.yuv",
		cv::Size(4096, 2048), 10, 34.41, 34.67); // VC14 + OpenCV 3.1.0: 35.5957, 36.1051
												 //no opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"ClassroomVideo/v0_4096_2048_420_10b.yuv",
		"v0vs_from_v7v8_4096_2048_420_10b.yuv",
		cv::Size(4096, 2048), 10, 35.5, 36.0); // VC14 + OpenCV 3.1.0: 35.5957, 36.1051
											   //no opengl vs opengl
	testing::compareWithReferenceView<std::uint16_t>(
		"v0vs_from_v7v8_4096_2048_420_10b.yuv",
		"v0vs_from_v7v8_4096_2048_420_10b_OpenGL.yuv",
		cv::Size(4096, 2048), 10, 35.54, 36.05);
}

FUNC(TechnicolorHijack_v1v4_to_v9_OpenGL)
{
	if (!testing::file_exists("TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b.yuv")) {
		g_with_opengl = false;
		Pipeline p("./config_files/_integration_tests/TechnicolorHijack-SVS-v1v4_to_v9.cfg");
		p.execute();
	}

	g_with_opengl = true;
	context_init();
	Pipeline pGL("./config_files/_integration_tests/TechnicolorHijack-SVS-v1v4_to_v9_OpenGL.cfg");
	pGL.execute();

	//no opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b.yuv",
		"TechnicolorHijack/v9_4096_4096_420_10b.yuv",
		cv::Size(4096, 4096), 10, 43.71, 35.88); // VC14 + OpenCV 3.1.0: 43.7695, 35.9381
												 //opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b_OpenGL.yuv",
		"TechnicolorHijack/v9_4096_4096_420_10b.yuv",
		cv::Size(4096, 4096), 10, 41.03, 34.73); // VC14 + OpenCV 3.1.0: 43.7695, 35.9381
												 //no opengl vs opengl
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b.yuv",
		"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b_OpenGL.yuv",
		cv::Size(4096, 4096), 10, 43.71, 35.88); // VC14 + OpenCV 3.1.0: 43.7695, 35.9381
}

FUNC(TechnicoloMuseum_SVS_v0v2v13v17v19_to_v1_OpenGL)
{
	if (!testing::file_exists("TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b.yuv")) {
		g_with_opengl = false;
		Pipeline p("./config_files/_integration_tests/TechnicolorMuseum-SVS-v0v2v13v17v19_to_v1.cfg");
		p.execute();
	}
	
	g_with_opengl = true;
	context_init();
	Pipeline pGL("./config_files/_integration_tests/TechnicolorMuseum-SVS-v0v2v13v17v19_to_v1_OpenGL.cfg");
	pGL.execute();

	//no opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v1_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 31.81, 38.20); // VC14 + OpenCV 3.1.0: 22.4218, 26.5975
												 //opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b_OpenGL.yuv",
		"TechnicolorMuseum/v1_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 26.96, 36.88); // VC14 + OpenCV 3.1.0: 22.4218, 26.5975
												 //no opengl vs opengl
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b.yuv",
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b_OpenGL.yuv",
		cv::Size(2048, 2048), 10, 22.37, 26.54); // VC14 + OpenCV 3.1.0: 22.4218, 26.5975
}

#endif

int main(int argc, const char* argv[])
{
	cv::setBreakOnError(true);
	return yaffut::main(argc, argv);
}
