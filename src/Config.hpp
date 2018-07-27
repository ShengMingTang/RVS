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
  Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be  Daniele Bonatto, Daniele.Bonatto@ulb.ac.be  Arnaud Schenkel, arnaud.schenkel@ulb.ac.be

Koninklijke Philips N.V., Eindhoven, The Netherlands:
  Bart Kroon, bart.kroon@philips.com
  Bart Sonneveldt, bart.sonneveldt@philips.com
*/

#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

#include "Parameters.hpp"
#include "PoseTraces.hpp"

#include <string>
#include <vector>

#include <opencv2/core.hpp>

/**
@file Config.hpp
\brief The file containing the configuration
*/

/**\brief Working color space

Doesn't need to be the same as the input our output color space
*/
enum ColorSpace {
	COLORSPACE_YUV = 0,
	COLORSPACE_RGB = 1
};
/**\brief View synthesis method

For now only the triangle method is available
*/
enum ViewSynthesisMethod {
	SYNTHESIS_TRIANGLE = 0
};

/**\brief Blending method

\see BlendedView
*/
enum BlendingMethod {
	BLENDING_SIMPLE = 0,
	BLENDING_MULTISPEC = 1
};

/**\brief Inpainting method*/
enum InpaintingMethod {
	INPAINTING_OFF = 0,
	INPAINTING_LINES = 1
};

/**\brief Projection type

\see Projector*/
enum ProjectionType {
    PROJECTION_PERSPECTIVE = 0,
    PROJECTION_EQUIRECTANGULAR = 1
};

/**Precision*/
extern float rescale;

/**\brief RGB color for empty pixel (when no inpainting)*/
const cv::Vec3f empty_rgb_color(0.0f, 1.0f, 0.0f);

/**\brief YUV color for empty pixel (when no inpainting)*/
const cv::Vec3f empty_yuv_color(0.0f, 0.0f, 0.0f);

/**Working color space (RGB or YUV). Independent of the input or output formats*/
extern ColorSpace color_space;

/**\brief Method for view synthesis*/
extern ViewSynthesisMethod vs_method;

/**
\brief Configuration parameters
*/
class Config {
public:
	/**
	\brief Constructor
	*/
	Config() {};

	/**
	\brief Destructor
	*/
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

	/** 
	Name of the output files
	
	Set to "ALL" to synthesize all the views of the config file or the name of each output filename */
	std::vector<std::string> outfilenames;

	/**
	Name of the output masked files
	*/
	std::vector<std::string> outmaskedfilenames;


	/**
	Threshold for valid pixels
	*/
	float validity_threshold = 5000.f;

	/** What is the extension of the files to use if not specified in configuration file */
	std::string extension = "png";

	/** The element bit depth of raw input and output texture streams */
	int bit_depth_color = 8;

	/** The element bit depth of raw input and output depth streams */
	int bit_depth_depth = 16;

	/** Size of input image*/
	cv::Size size = cv::Size(1920, 1080);

	/** Size of output image (multiply by rescale to get the working size)*/
	cv::Size virtual_size = cv::Size(0, 0);

	/** Blending method (see BlendedView) */
	BlendingMethod blending_method = BLENDING_SIMPLE;

	/** Low frequency blending factor in BlendedViewMultiSpec */
	float blending_low_freq_factor = 1.0f;

	/** High frequency blending factor in BlendedViewMultiSpec */
	float blending_high_freq_factor = 4.0f;

	/** Blending factor in BlendedViewSimple */
	float blending_factor = 1.0f;

	/** Input projection type */
    ProjectionType input_projection_type   = PROJECTION_PERSPECTIVE;

	/** Output projection type */
    ProjectionType virtual_projection_type = PROJECTION_PERSPECTIVE;

	/**size of the cameras sensor, in the same unit as focal length*/
	float sensor_size = 1920.0f;

	/** First frame to process (zero-based) */
	int start_frame = 0;

	/** Number of frames to process */
	int number_of_frames = 1;

    /** filename of the pose trace file*/
    std::string name_pose_trace;

	/** true if using pose trace*/
    bool use_pose_trace;

	/**Pose*/
    std::vector<pose_traces::Pose> pose_trace;
};

#endif