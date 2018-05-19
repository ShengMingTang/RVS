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

#include "yaffut.hpp"

#include "Pipeline.hpp"

#include <array>
#include <fstream>

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
		char const *filepath_actual,
		char const *filepath_reference,
		cv::Size size, int bits,
		double threshold0, double threshold1)
	{
		auto actual = readYUV420<T>(filepath_actual, size);
		auto reference = readYUV420<T>(filepath_reference, size);
		double psnr[2];

		for (int i_region = 0; i_region != 2; ++i_region) {
			auto sum_sq_error = 0.;
			std::size_t num_values = 0;

			for (int i_plane = 0; i_plane != 3; ++i_plane) {
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

		std::clog << "compareWithReferenceView(" << filepath_actual << ", " << filepath_reference << ", "
			<< threshold0 << ", " << threshold1 << "): " << psnr[0] << ", " << psnr[1] << std::endl;
		YAFFUT_CHECK(psnr[0] > threshold0);
		YAFFUT_CHECK(psnr[1] > threshold1);
	}
}

FUNC(ULB_Unicorn_Example)
{
	Pipeline p("./config_files/example_config_file.cfg");
	p.execute();
	// No reference
}

FUNC(ULB_Unicorn_Triangles_Simple)
{
	Pipeline p("./config_files/Unicorn_Triangles_Simple.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_Simple.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		cv::Size(1920, 1080), 8, 21.13, 25.99); // VC14 + OpenCV 3.1.0: 21.1814, 26.0475
}

FUNC(ULB_Unicorn_Triangles_MultiSpectral)
{
	Pipeline p("./config_files/Unicorn_Triangles_MultiSpectral.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Triangles_MultiSpectral.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		cv::Size(1920, 1080), 8, 21.12, 25.97); // VC14 + OpenCV 3.1.0: 21.1701, 26.0268
}

FUNC(ULB_Unicorn_Squares_Simple)
{
	Pipeline p("./config_files/Unicorn_Squares_Simple.cfg");
	p.execute();
	testing::compareWithReferenceView<std::uint8_t>(
		"030003250438_from_030003070370_030003430506_Squares_Simple.yuv",
		"Plane_B'/Plane_B'_Texture/Kinect_z0300y0325x0438.yuv",
		cv::Size(1920, 1080), 8, 20.94, 25.00); // VC14 + OpenCV 3.1.0: 20.9853, 25.05
}

int main(int argc, const char* argv[])
{
	cv::setBreakOnError(true);
	return yaffut::main(argc, argv);
}
