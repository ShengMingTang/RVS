/* ------------------------------------------------------------------------------ -

Copyright © 2018 Koninklijke Philips N.V.

Authors : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

SVS 3DoF+
For the purpose of the 3DoF+ Investigation, SVS is extended to match with the
description of the Reference View Synthesizer(RVS) of the 3DoF+ part of the CTC.
This includes support for unprojecting and projecting ERP images, reading and
writing of 10 - bit YUV 4:2 : 0 texture and depth, parsing of JSON files according to
the 3DoF + CfTM, and unit / integration tests.

Permission is hereby granted, free of charge, to the members of the Moving Picture
Experts Group(MPEG) obtaining a copy of this software and associated documentation
files(the "Software"), to use the Software exclusively within the framework of the
MPEG - I(immersive) and MPEG - I Visual activities, for the sole purpose of
developing the MPEG - I standard.This permission explicitly excludes the rights
to publish, distribute, sublicense, sell, embed into a product or a service and / or
otherwise commercially exploit copies of the Software without the written consent
of the owner(Koninklijke Philips N.V.).

This permission is provided subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies, substantial portions or derivative works of the Software.

------------------------------------------------------------------------------ - */

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

extern bool with_opengl;
#if WITH_OPENGL
#include "helpersGL.hpp"
#endif

#define PSNR_ONLY_Y false

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
		std::clog << "size == " << size << ", sizeof(T) == " << sizeof(T) << ", filesize == " << filesize << ", framesize == " << framesize << std::endl;
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
#if PSNR_ONLY_Y
			auto num_planes = 1;
#else
			auto num_planes = 3;
#endif

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

		std::clog << "PSNRs: " << psnr[0] << ", " << psnr[1] << std::endl;
		YAFFUT_CHECK(psnr[0] > threshold0);
		YAFFUT_CHECK(psnr[1] > threshold1);

		if (psnr[0] > threshold0 + 0.1 ||
			psnr[1] > threshold1 + 0.1) {
			std::clog << "WARNING: thresholds can be increased\n";
		}
	}

	template<typename T> void compareWithReferenceView(
		char const *filepath_actual,
		char const *filepath_reference,
		cv::Size size, int bits,
		double threshold0, double threshold1)
	{
		std::clog << "Comparing \"" << filepath_actual << "\" " << size << " with \"" << filepath_reference << "\" " << size << std::endl;
		auto actual = readYUV420<T>(filepath_actual, size);
		auto reference = readYUV420<T>(filepath_reference, size);
		compareWithReferenceView(actual, reference, bits, threshold0, threshold1);
	}
}

FUNC(ULB_Unicorn_Example)
{
	Pipeline p("./config_files/example_config_file.cfg");
	p.execute();
	// No reference
}


#if WITH_OPENGL
FUNC(ULB_Unicorn_Triangles_Simple_OpenGL)
{
	Pipeline p("./config_files/Unicorn_Triangles_Simple.cfg");
	p.execute();

	with_opengl = true;
	context_init();
	Pipeline pgl("./config_files/Unicorn_Triangles_Simple_OpenGL.cfg");
	pgl.execute();
	with_opengl = false;

	//No opengl vs ref
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_Simple.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		cv::Size(1920, 1080), 8, 20.32, 20.54); // VC14 + OpenCV 3.1.0: 20.377, 24.5955
	//Opengl vs ref
	testing::compareWithReferenceView<std::uint8_t>(
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		"030003250438_from_030003070370_030003430506_Triangles_Simple_OpenGL.yuv",
		cv::Size(1920, 1080), 8, 20.32, 24.54); // VC14 + OpenCV 3.1.0: 20.377, 24.5955
	//no opengl vs opengl
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_Simple.yuv",
		"030003250438_from_030003070370_030003430506_Triangles_Simple_OpenGL.yuv",
		cv::Size(1920, 1080), 8, 30.0, 30.54); // VC14 + OpenCV 3.1.0: 20.377, 24.5955

}
#endif

#if WITH_OPENGL
FUNC(ClassroomVideo_v7v8_to_v0_OpenGL)
{
	Pipeline p("./config_files/ClassroomVideo-SVS-v7v8_to_v0.cfg");
	p.execute();

	with_opengl = true;
	context_init();
	Pipeline pGL("./config_files/ClassroomVideo-SVS-v7v8_to_v0_OpenGL.cfg");
	pGL.execute();
	with_opengl = false;

	//opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"ClassroomVideo/v0_4096_2048_420_10b.yuv",
		"v0vs_from_v7v8_4096_2048_420_10b_OpenGL.yuv",
		cv::Size(4096, 2048), 10, 35.5, 36.0); // VC14 + OpenCV 3.1.0: 35.5957, 36.1051
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
#endif



#if WITH_OPENGL
FUNC(TechnicolorHijack_v1v4_to_v9_OpenGL)
{
	Pipeline p("./config_files/TechnicolorHijack-SVS-v1v4_to_v9.cfg");
	p.execute();
	with_opengl = true;
	context_init();
	Pipeline pGL("./config_files/TechnicolorHijack-SVS-v1v4_to_v9_OpenGL.cfg");
	pGL.execute();
	with_opengl = false;
	//no opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b.yuv",
		"TechnicolorHijack/v9_4096_4096_420_10b.yuv",
		cv::Size(4096, 4096), 10, 43.71, 35.88); // VC14 + OpenCV 3.1.0: 43.7695, 35.9381
												 //opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b_OpenGL.yuv",
		"TechnicolorHijack/v9_4096_4096_420_10b.yuv",
		cv::Size(4096, 4096), 10, 43.71, 35.88); // VC14 + OpenCV 3.1.0: 43.7695, 35.9381
												 //no opengl vs opengl
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b.yuv",
		"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b_OpenGL.yuv",
		cv::Size(4096, 4096), 10, 43.71, 35.88); // VC14 + OpenCV 3.1.0: 43.7695, 35.9381
}
#endif

#if WITH_OPENGL
FUNC(TechnicolorMuseum_v5_to_v6_OpenGL)
{
	Pipeline p("./config_files/TechnicoloMuseum-SVS-v0v2v13v17v19_to_v1.cfg");
	p.execute();
	with_opengl = true;
	context_init();
	Pipeline pGL("./config_files/TechnicoloMuseum-SVS-v0v2v13v17v19_to_v1_OpenGL.cfg");
	pGL.execute();
	with_opengl = false;
	//no opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v1_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 22.37, 26.54); // VC14 + OpenCV 3.1.0: 22.4218, 26.5975
												 //opengl vs ref
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b_OpenGL.yuv",
		"TechnicolorMuseum/v1_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 22.37, 26.54); // VC14 + OpenCV 3.1.0: 22.4218, 26.5975
												 //no opengl vs opengl
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b.yuv",
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b_OpenGL.yuv",
		cv::Size(2048, 2048), 10, 22.37, 26.54); // VC14 + OpenCV 3.1.0: 22.4218, 26.5975
}

#endif

FUNC(ULB_Unicorn_Triangles_Simple)
{
	Pipeline p("./config_files/Unicorn_Triangles_Simple.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_Simple.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		cv::Size(1920, 1080), 8, 20.32, 24.54); // VC14 + OpenCV 3.1.0: 20.377, 24.5955
}

FUNC(ULB_Unicorn_Triangles_MultiSpectral)
{
	Pipeline p("./config_files/Unicorn_Triangles_MultiSpectral.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_MultiSpectral.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		cv::Size(1920, 1080), 8, 20.30, 24.35); // VC14 + OpenCV 3.1.0: 20.3568, 24.4012
}

FUNC(ULB_Unicorn_10b)
{
	Pipeline p("./config_files/Unicorn_10b.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"030003250438_from_030003070370_030003430506-10b.yuv",
		"10bit/Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438-10b.yuv",
		cv::Size(1920, 1080), 10, 20.49, 24.75); // VC14 + OpenCV 3.1.0: 20.5474, 24.8075
}

FUNC(ULB_Unicorn_Same_View)
{
	Pipeline p("./config_files/Unicorn_Same_View.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250506_from_030003250506_Same_View.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0506.yuv",
		cv::Size(1920, 1080), 8, 19.80, 34.03); // VC14 + OpenCV 3.1.0: 19.8518, 34.0898
}

FUNC(ClassroomVideo_v0_to_v0)
{
	Pipeline p("./config_files/ClassroomVideo-SVS-v0_to_v0.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"v0vs_4096_2048_420_10b.yuv",
		"ClassroomVideo/v0_4096_2048_420_10b.yuv",
		cv::Size(4096, 2048), 10, 70.10, 78.52); // VC14 + OpenCV 3.1.0: 70.1539, 78.572 (with new, full depth maps)
}

FUNC(ClassroomVideo_v7v8_to_v0)
{
	Pipeline p("./config_files/ClassroomVideo-SVS-v7v8_to_v0.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"v0vs_from_v7v8_4096_2048_420_10b.yuv",
		"ClassroomVideo/v0_4096_2048_420_10b.yuv",
		cv::Size(4096, 2048), 10, 35.54, 36.05); // VC14 + OpenCV 3.1.0: 35.5957, 36.1051
}

FUNC(ClassroomVideo_v7v8_to_v0_270deg)
{
	Pipeline p("./config_files/ClassroomVideo-SVS-v7v8_to_v0_270deg.cfg");
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
	Pipeline p("./config_files/TechnicolorHijack-SVS-v1v4_to_v9.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
	"TechnicolorHijack_v9vs_from_v1v4_4096_4096_420_10b.yuv",
	"TechnicolorHijack/v9_4096_4096_420_10b.yuv",
	cv::Size(4096, 4096), 10, 43.71, 35.88); // VC14 + OpenCV 3.1.0: 43.7695, 35.9381
}

FUNC(TechnicolorHijack_BlendByMax)
{
	Pipeline p("./config_files/TechnicolorHijack-BlendByMax.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorHijack_BlendByMax.yuv",
		"TechnicolorHijack/v9_4096_4096_420_10b.yuv",
		cv::Size(4096, 4096), 10, 43.19, 35.28); // VC14 + OpenCV 3.1.0: 43.2489, 35.3323
}

FUNC(TechnicolorMuseum_v0v2v13v17v19_to_v1)
{
	Pipeline p("./config_files/TechnicoloMuseum-SVS-v0v2v13v17v19_to_v1.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v1vs_from_v0v2v13v17v19_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v1_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 31.76, 38.15); // VC14 + OpenCV 3.1.0: 31.8129, 38.2094
}

FUNC(TechnicolorMuseum_v0_to_v0)
{
	Pipeline p("./config_files/TechnicoloMuseum-SVS-v0_to_v0.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v0vs_from_v0_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v0_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 38.30, 38.95); // VC14 + OpenCV 3.1.0: 38.358, 39.0078
}

FUNC(TechnicolorMuseum_v5_to_v5)
{
	Pipeline p("./config_files/TechnicolorMuseum-SVS-v5_to_v5.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v5vs_from_v5_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v5_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 58.51, 73.73); // VC14 + OpenCV 3.1.0: 58.5664, 73.7879
}

FUNC(TechnicolorMuseum_v5_to_v6)
{
	Pipeline p("./config_files/TechnicoloMuseum-SVS-v5_to_v6.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint16_t>(
		"TechnicolorMuseum_v6vs_from_v5_2048_2048_420_10b.yuv",
		"TechnicolorMuseum/v6_2048_2048_420_10b.yuv",
		cv::Size(2048, 2048), 10, 22.37, 26.54); // VC14 + OpenCV 3.1.0: 22.4218, 26.5975
}

int main(int argc, const char* argv[])
{
	with_opengl = false;
	cv::setBreakOnError(true);
	return yaffut::main(argc, argv);
}
