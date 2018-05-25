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

Support for n-bit raw texture and depth streams.

Author  : Bart Kroon
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

#pragma once

#include "Parameters.hpp"

#include <string>
#include <vector>

#include <opencv2/core.hpp>




enum ColorSpace {
	COLORSPACE_YUV = 0,
	COLORSPACE_RGB = 1
};
enum ViewSynthesisMethod {
	SYNTHESIS_TRIANGLE = 0,
	SYNTHESIS_SQUARE = 1,
	SYNTHESIS_VSRS = 2, //not implemented
};
enum BlendingMethod {
	BLENDING_SIMPLE = 0,
	BLENDING_MULTISPEC = 1
};
enum InpaintingMethod {
	INPAINTING_OFF = 0,
	INPAINTING_LINES = 1
};

enum ProjectionType {
    PROJECTION_PERSPECTIVE = 0,
    PROJECTION_EQUIRECTANGULAR = 1
};

/**Precision*/
extern float rescale;
const cv::Vec3f empty_rgb_color(0.0f, 1.0f, 0.0f);
const cv::Vec3f empty_yuv_color(0.0f, 0.0f, 0.0f);
/**Working color space (RGB or YUV). Independent of the input or output formats*/
extern ColorSpace color_space;
extern ViewSynthesisMethod vs_method;


class Config {
public:
	Config() {};
	~Config() {  };
	/** Input camera names to lookup in the config file */
	std::vector<std::string> InputCameraNames;

	/** Virtual camera names to lookup in the config file. "ALL" to synthesized all the views of the file */
	std::vector<std::string> VirtualCameraNames;

	/** real camera's parameters read from file */
	std::string camerasParameters_in;

	/** virtual camera's parameters read from file */
	std::string virtualCamerasParameters_in;

	/** folder for output */
	std::string folder_out;

	/** input cameras parameters */
	std::vector<Parameters> params_real;

	/** virtual cameras parameters */
	std::vector<Parameters> params_virtual;

	/** filenames of the input color images */
	std::vector<std::string> texture_names;

	/** filenames of the input D images */
	std::vector<std::string> depth_names;

	/** file with the z far and z near values */
	std::string zvalues;

	/** znear of every input view in case of disparity map yuv file */
	std::vector<float> znear;

	/** zfar of every input view in case of disparity map yuv file */
	std::vector<float> zfar;

	/** "ALL" to Synthesized all the views of the config file or the name of each output filename */
	std::vector<std::string> outfilenames;

	/** What is the extension of the files to use if not specified in configuration file */
	std::string extension = "png";

	/** The element bit depth of raw input and output texture streams */
	int bit_depth_color = 8;

	/** The element bit depth of raw input and output depth streams */
	int bit_depth_depth = 16;

	/** Do we use inpainting (TODO)*/
	int inpainting = 0;

	size_t size_virtual() {
		return VirtualCameraNames.size();
	}
	size_t size_real() {
		return InputCameraNames.size();
	}

	/** Size of input and output image (multiply by rescale to get the working size)*/
	cv::Size size = cv::Size(1920, 1080);


	BlendingMethod blending_method = BLENDING_SIMPLE;
	float blending_low_freq_factor = 1.0f;
	float blending_high_freq_factor = 4.0f;
	float blending_factor = 1.0f;

    ProjectionType input_projection_type   = PROJECTION_PERSPECTIVE;
    ProjectionType virtual_projection_type = PROJECTION_PERSPECTIVE;

	/**size of the cameras sensor, in the same unit as focal length*/
	float sensor_size = 1920.0f;
};