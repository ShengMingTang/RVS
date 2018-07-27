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

#include "Parser.hpp"
#include <iostream>
#include <fstream>
#include <regex>
#include "PoseTraces.hpp"

namespace svs
{
	void check_is_open(std::ifstream& stream, char const *kind, std::string const& ParameterFile)
	{
		if (stream.is_open() == 0) {
			std::ostringstream text;
			text << "Failed to open " << kind << " file \"" << ParameterFile << "\".";
			throw std::runtime_error(text.str());
		}
	}

	bool seek_string(std::string ParameterFile, int n, std::vector<std::string>& str, std::string key, std::string err_msg) {
		std::ifstream file(ParameterFile);
		check_is_open(file, "parameter", ParameterFile);
		std::string id;
		while (!file.eof() && file >> id) {
			if (id == key) {
				for (int i = 0; i < n; ++i) {
					std::string c;
					file >> c;
					str.push_back(c);
				}
				return true;
			}
		}
		printf("%s: %s  not found in the parameter file.\n", key.c_str(), err_msg.c_str());
		return false;
	}

	bool seek_int(std::string ParameterFile, int& n, std::string key, std::string err_msg) {
		std::ifstream file(ParameterFile);
		check_is_open(file, "parameter", ParameterFile);
		std::string id;
		while (!file.eof() && file >> id) {
			if (id == key) {
				file >> n;
				return true;
			}
		}
		printf("%s: %s  not found in the parameter file.\n", key.c_str(), err_msg.c_str());
		return false;
	}

	bool seek_float(std::string ParameterFile, float& f, std::string key, std::string err_msg) {
		std::ifstream file(ParameterFile);
		check_is_open(file, "parameter", ParameterFile);
		std::string id;
		while (!file.eof() && file >> id) {
			if (id == key) {
				file >> f;
				return true;
			}
		}
		printf("%s: %s  not found in the parameter file.\n", key.c_str(), err_msg.c_str());
		return false;
	}

	bool seek_znear_zfar(std::string ParameterFile, float& z, std::string DepthFile, std::string key, std::string err_msg) {
		std::ifstream file(ParameterFile);
		check_is_open(file, "parameter", ParameterFile);
		std::string id;
		while (!file.eof() && file >> id) {
			if (id.find(DepthFile) != std::string::npos) {
				while (!file.eof() && file >> id) {
					if (id == key) {
						file >> z;
						return true;
					}
				}
			}
		}
		printf("%s was not found: using default values\n", err_msg.c_str());
		return false;
	}

	bool find_cam(std::string CameraParameterFile, cv::Mat & r, cv::Vec3f & t, cv::Mat & camMat, std::string cameraId) {
		std::ifstream file(CameraParameterFile);
		check_is_open(file, "camera parameter", CameraParameterFile);
		std::string id;
		while (!file.eof() && file >> id) {
			if (id == cameraId) {
				float f11, f12, f13, f21, f22, f23, f31, f32, f33, k1, k2;
				float r11, r12, r13, r21, r22, r23, r31, r32, r33;
				float tx, ty, tz;
				file >> f11 >> f12 >> f13 >> f21 >> f22 >> f23 >> f31 >> f32 >> f33 >> k1 >> k2 >> r11 >> r12 >> r13 >> tx >> r21 >> r22 >> r23 >> ty >> r31 >> r32 >> r33 >> tz;
				camMat = (cv::Mat_<float>(3, 3) << f11, f12, f13, f21, f22, f23, f31, f32, f33);
				r = (cv::Mat_<float>(3, 3) << r11, r12, r13, r21, r22, r23, r31, r32, r33);
				t = cv::Vec3f(tx, ty, tz);
				return true;
			}
		}
		printf("Camera %s not found\n", cameraId.c_str());
		return false;
	}

	//read cameras names, positions and rotation
	void read_cameras_parameters(std::string filename, std::vector<std::string>& camnames, std::vector<Parameters>& params, float& sensor_size) {
		//read all cameras
		if (camnames.size() == 0 || camnames[0] == "ALL")
		{
			camnames = {};
			std::ifstream file(filename);
			check_is_open(file, "camera parameters", filename);
			std::string id;
			while (!file.eof() && file >> id) {
				float f11, f12, f13, f21, f22, f23, f31, f32, f33, k1, k2;
				float r11, r12, r13, r21, r22, r23, r31, r32, r33;
				float tx, ty, tz;
				file >> f11 >> f12 >> f13 >> f21 >> f22 >> f23 >> f31 >> f32 >> f33 >> k1 >> k2 >> r11 >> r12 >> r13 >> tx >> r21 >> r22 >> r23 >> ty >> r31 >> r32 >> r33 >> tz;
				cv::Mat cam_mat = (cv::Mat_<float>(3, 3) << f11, f12, f13, f21, f22, f23, f31, f32, f33);
				cv::Mat r = (cv::Mat_<float>(3, 3) << r11, r12, r13, r21, r22, r23, r31, r32, r33);
				cv::Vec3f t = cv::Vec3f(tx, ty, tz);
				camnames.push_back(id);
				params.push_back(Parameters(r, t, cam_mat, sensor_size, CoordinateSystem::VSRS));
			}
			return;
		}
		//read only specified cameras
		for (int i = 0; i < static_cast<int>(camnames.size()); ++i) {
			cv::Mat r;
			cv::Vec3f t;
			cv::Mat cam_mat;
			if (find_cam(filename, r, t, cam_mat, camnames[i])) {
				params.push_back(Parameters(r, t, cam_mat, sensor_size, CoordinateSystem::VSRS));
			}
		}
	}
}

namespace json
{
	using svs::check_is_open;

	bool is_file(std::string filename)
	{
		return filename.substr(filename.size() - 5, 5) == ".json";
	}

	std::vector<std::string> read_cam_ids(std::string const& text)
	{
		std::regex name_regex("\"Name\"\\s*:\\s*\"(.+)\"");
		std::vector<std::string> result;
		for (auto name = std::sregex_iterator(std::begin(text), std::end(text), name_regex);
			      name != std::sregex_iterator();
			    ++name) { 
			auto match = *name;
			result.push_back(match[1].str());
		}
		return result;
	}

	std::vector<cv::Vec3f> read_positions(std::string const& text)
	{
		std::regex name_regex("\"Position\"\\s*:\\s*\\[(.+),(.+),(.+)\\]");
		std::vector<cv::Vec3f> result;
		for (auto name = std::sregex_iterator(std::begin(text), std::end(text), name_regex);
			name != std::sregex_iterator();
			++name) {
			auto match = *name;
			result.emplace_back(
				stof(match[1].str()),
				stof(match[2].str()),
				stof(match[3].str()));
		}
		return result;
	}

	std::vector<cv::Vec3f> read_rotations(std::string const& text)
	{
		std::regex name_regex("\"Rotation\"\\s*:\\s*\\[(.+),(.+),(.+)\\]");
		std::vector<cv::Vec3f> result;
		for (auto name = std::sregex_iterator(std::begin(text), std::end(text), name_regex);
			name != std::sregex_iterator();
			++name) {
			auto match = *name;
			result.emplace_back(
				stof(match[1].str()),
				stof(match[2].str()),
				stof(match[3].str()));
		}
		return result;
	}

	std::vector<float> read_radius_far(std::string const& text)
	{
		std::regex name_regex("\"Rmax\"\\s*:(.+),");
		std::vector<float> result;
		for (auto name = std::sregex_iterator(std::begin(text), std::end(text), name_regex);
			name != std::sregex_iterator();
			++name) {
			auto match = *name;
			auto Rmax = stof(match[1].str());
			if (Rmax == 1000.f) // according to CfTM
				Rmax = std::numeric_limits<float>::infinity();
			result.push_back(Rmax);
		}
		return result;
	}

	std::vector<float> read_radius_near(std::string const& text)
	{
		std::regex name_regex("\"Rmin\"\\s*:(.+),");
		std::vector<float> result;
		for (auto name = std::sregex_iterator(std::begin(text), std::end(text), name_regex);
			name != std::sregex_iterator();
			++name) {
			auto match = *name;
			result.push_back(stof(match[1].str()));
		}
		return result;
	}

	void read_cameras_parameters(std::string filename, std::vector<std::string>& cam_ids, std::vector<Parameters>& params, float& sensor_size,
		std::vector<float> *zfar, std::vector<float> *znear)
	{
		// Read entire file
		std::ifstream stream(filename);
		check_is_open(stream, "JSON metadata file", filename);
		std::stringstream buffer;
		buffer << stream.rdbuf();
		auto text = buffer.str();

		// Read relevant fields
		auto all_cam_ids = read_cam_ids(text);
		auto all_positions = read_positions(text);
		auto all_rotations = read_rotations(text);
		auto all_radius_far = read_radius_far(text);
		auto all_radius_near = read_radius_near(text);

		CV_Assert(all_cam_ids.size() == all_positions.size());
		CV_Assert(all_cam_ids.size() == all_rotations.size());
		CV_Assert(!zfar || all_cam_ids.size() == all_radius_far.size());
		CV_Assert(!znear || all_cam_ids.size() == all_radius_near.size());

		if (cam_ids.empty() || cam_ids.front() == "ALL")
			cam_ids = all_cam_ids;

		for (auto cam_id : cam_ids) {
			auto iter = std::find(std::begin(all_cam_ids), std::end(all_cam_ids), cam_id);
			if (iter == all_cam_ids.end())
				throw std::runtime_error("Camera ID does not occur in camera metadata file");

			auto index = iter - std::begin(all_cam_ids);
			auto position = all_positions[index];
			auto rotation = all_rotations[index];

			std::clog << cam_id << ": position=" << position << ", rotation=" << rotation << std::endl;

			auto R = pose_traces::detail::EulerAnglesToRotationMatrix(rotation * CV_PI / 180.f);
			auto t = position;

			// 90deg FOV for pose trace
			auto cam_mat = cv::Matx33f(
				2.f*sensor_size, 0.f,             sensor_size/2, 
				0.f,             2.f*sensor_size, sensor_size/4,
				0.f,             0.f,             1.f);

			params.push_back(Parameters(R, t, cam_mat, sensor_size, CoordinateSystem::MPEG_I_OMAF));
			if (zfar) zfar->push_back(all_radius_far[index]);
			if (znear) znear->push_back(all_radius_near[index]);
		}
	}	
}

void read_cameras_parameters(std::string filename, std::vector<std::string>& camnames, std::vector<Parameters>& params, float& sensor_size,
	std::vector<float> *zfar, std::vector<float> *znear)
{
	if (json::is_file(filename))
		json::read_cameras_parameters(filename, camnames, params, sensor_size, zfar, znear);
	else
		svs::read_cameras_parameters(filename, camnames, params, sensor_size);
}

Parser::Parser(const std::string & filename)
	: filename_parameter_file(filename)
{
	if (is_SVS_file(filename))
	{
		read_SVS_config_file();
		read_ZValues();
	}
	else
	{
		read_vsrs_config_file();
		config.use_pose_trace = false;
	}

	// Intelligent default for backwards compatibility
	if (config.virtual_size == cv::Size(0, 0)) {
		config.virtual_size = config.size;
	}

	//get input cameras parameters
	read_cameras_parameters(config.camerasParameters_in, config.InputCameraNames, config.params_real, config.sensor_size, &config.zfar, &config.znear);

	//get virtual cameras parameters to render
	read_cameras_parameters(config.virtualCamerasParameters_in, config.VirtualCameraNames, config.params_virtual, config.sensor_size, nullptr, nullptr);

	
	generate_output_filenames();
}

Parser::~Parser()
{
}

void Parser::generate_output_filenames() {
	if (config.outfilenames.size() != config.VirtualCameraNames.size() && config.outmaskedfilenames.empty()) { // ALL?
		config.outfilenames = {};
		for (int i = 0; i < static_cast<int>(config.VirtualCameraNames.size()); ++i) {
			config.outfilenames.push_back(config.folder_out + config.VirtualCameraNames[i] + "." + config.extension);
		}
	}
	else {
		for (int i = 0; i < static_cast<int>(config.outfilenames.size()); ++i) {
			config.outfilenames[i] = config.folder_out + config.outfilenames[i];
		}
	}
}

bool Parser::is_SVS_file(const std::string & filename) const
{
	int n = 0;
	svs::seek_int(filename, n, "SVSFile", "This is a VSRS file.");
	return !!n;
}

void Parser::read_vsrs_config_file() {
	using namespace svs;

	// Input camera file containing all the parameters for each camera
	std::vector<std::string> InputCameraParameterFile;
	// camera parameter file for the output cameras
	std::vector<std::string> VirtualCameraParameterFile;
	// Name of the output folder
	std::vector<std::string> folders_out;
	
	//seek input camera file
	seek_string(filename_parameter_file, 1, InputCameraParameterFile, "CameraParameterFile", "Camera parameter file");
	//seek left/right camera
	seek_string(filename_parameter_file, 1, config.texture_names, "LeftViewImageName", "Input RGB left file name");
	seek_string(filename_parameter_file, 1, config.texture_names, "RightViewImageName", "Input RGB right file name");
	seek_string(filename_parameter_file, 1, config.texture_names, "Left2ViewImageName", "Input RGB left file name");
	seek_string(filename_parameter_file, 1, config.texture_names, "Right2ViewImageName", "Input RGB right file name");
	//seek l/r Depth images
	seek_string(filename_parameter_file, 1, config.depth_names, "LeftDepthMapName", "Input depth left file name");
	seek_string(filename_parameter_file, 1, config.depth_names, "RightDepthMapName", "Input depth right file name");
	seek_string(filename_parameter_file, 1, config.depth_names, "Left2DepthMapName", "Input depth left file name");
	seek_string(filename_parameter_file, 1, config.depth_names, "Right2DepthMapName", "Input depth right file name");
	//seek l/r input cameras names
	seek_string(filename_parameter_file, 1, config.InputCameraNames, "LeftCameraName", "Input left camera name");
	seek_string(filename_parameter_file, 1, config.InputCameraNames, "RightCameraName", "Input right camera name");
	seek_string(filename_parameter_file, 1, config.InputCameraNames, "Left2CameraName", "Input left camera name");
	seek_string(filename_parameter_file, 1, config.InputCameraNames, "Right2CameraName", "Input right camera name");

	//seek virtual camera file
	seek_string(filename_parameter_file, 1, VirtualCameraParameterFile, "CameraParameterFile", "Camera parameter file");
	//seek virtual cameras names
	seek_string(filename_parameter_file, 1, config.VirtualCameraNames, "VirtualCameraName", "Output cameras names");
	//seek folder for output
	if (seek_string(filename_parameter_file, 1, folders_out, "OuputDir", "Output directory"))
		config.folder_out = folders_out[0];
	//seek filesnames for output
	seek_string(filename_parameter_file, 1, config.outfilenames, "OutputVirtualViewImageName", "Output file names");
	seek_string(filename_parameter_file, 1, config.outmaskedfilenames, "MaskedVirtualViewImageName", "Masked output file names");

	if (seek_float(filename_parameter_file, rescale, "Precision", "Precision") == 0)
		rescale = 4.0f;

	//seek w,h (default = 1920x1080)
	int w, h;
	if (seek_int(filename_parameter_file, w, "SourceWidth", "") && seek_int(filename_parameter_file, h, "SourceHeight", ""))
		config.size = cv::Size(w, h);

	//seek zn, zf, l/r
	float z_near_left, z_far_left, z_near_right, z_far_right;
	seek_float(filename_parameter_file, z_near_left, "LeftNearestDepthValue", "");
	seek_float(filename_parameter_file, z_far_left, "LeftFarthestDepthValue", "");
	seek_float(filename_parameter_file, z_near_right, "RightNearestDepthValue", "");
	seek_float(filename_parameter_file, z_far_right, "RightFarthestDepthValue", "");
	config.zfar.push_back(z_far_left);
	config.zfar.push_back(z_far_right);
	config.znear.push_back(z_near_left);
	config.znear.push_back(z_near_right);
	//seek zn, zf, l/r
	float z_near_left2, z_far_left2, z_near_right2, z_far_right2;
	seek_float(filename_parameter_file, z_near_left2, "Left2NearestDepthValue", "");
	seek_float(filename_parameter_file, z_far_left2, "Left2FarthestDepthValue", "");
	seek_float(filename_parameter_file, z_near_right2, "Right2NearestDepthValue", "");
	seek_float(filename_parameter_file, z_far_right2, "Right2FarthestDepthValue", "");
	config.zfar.push_back(z_far_left2);
	config.zfar.push_back(z_far_right2);
	config.znear.push_back(z_near_left2);
	config.znear.push_back(z_near_right2);

	print_results(InputCameraParameterFile, config.texture_names, config.depth_names, VirtualCameraParameterFile, 2, 1);
	
	config.camerasParameters_in = InputCameraParameterFile[0];
	config.virtualCamerasParameters_in = VirtualCameraParameterFile[0];
}


void Parser::read_SVS_config_file() {
	using namespace svs;

	// Input camera file containing all the parameters for each camera
	std::vector<std::string> InputCameraParameterFile;
	std::vector<std::string> InputZValuesFile;

	// number of RGB cameras
	int number_input_cameras;

	// number of D cameras
	int number_output_cameras;

	// camera parameter file for the output cameras
	std::vector<std::string> VirtualCameraParameterFile;

	// Name of the output folder
	std::vector<std::string> folders_out;

	//seek input camera file
	seek_string(filename_parameter_file, 1, InputCameraParameterFile, "InputCameraParameterFile", "Input camera parameter file");
	//seek input camera file
	if (seek_string(filename_parameter_file, 1, InputZValuesFile, "ZValues", "Input z values file"))
		config.zvalues = InputZValuesFile[0];
	//seek number of input cameras
	seek_int(filename_parameter_file, number_input_cameras, "InputCameraNumber", "Number of input cameras");
	//seek n of input RGB images
	seek_string(filename_parameter_file, number_input_cameras, config.texture_names, "ViewImagesNames", "Input RGB files names");
	//seek n of input Depth images
	seek_string(filename_parameter_file, number_input_cameras, config.depth_names, "DepthMapsNames", "Input Depth files names");
	//seek n of input cameras names
	seek_string(filename_parameter_file, number_input_cameras, config.InputCameraNames, "CamerasNames", "Input cameras names");

	//seek virtual camera file
	seek_string(filename_parameter_file, 1, VirtualCameraParameterFile, "VirtualCameraParamaterFile", "virtual camera parameter file");
	//seek number of virtual cameras
	seek_int(filename_parameter_file, number_output_cameras, "VirtualCameraNumber", "Number of virtual cameras");
	//seek m of virtual cameras names
	seek_string(filename_parameter_file, number_output_cameras, config.VirtualCameraNames, "VirtualCamerasNames", "Input cameras names");
	//seek folder for output
	if (seek_string(filename_parameter_file, 1, folders_out, "OuputDir", "Output directory"))
		config.folder_out = folders_out[0];
	//seek filesnames for output
	seek_string(filename_parameter_file, number_output_cameras, config.outfilenames, "OutputFiles", "Output file names");
	seek_string(filename_parameter_file, number_output_cameras, config.outmaskedfilenames, "MaskedOutputFiles", "Masked output file names");
	if (seek_float(filename_parameter_file, config.validity_threshold, "ValidityTheshold", "Validity threshold for masked output") == 0)
		config.validity_threshold = 5000.f;

	//seek w,h (default = 1920x1080)
	int w, h;
	if (seek_int(filename_parameter_file, w, "Width", "") && seek_int(filename_parameter_file, h, "Height", ""))
		config.size = cv::Size(w, h);
	//seek w,h (default = 1920x1080)
	if (seek_int(filename_parameter_file, w, "VirtualWidth", "") && seek_int(filename_parameter_file, h, "VirtualHeight", ""))
		config.virtual_size = cv::Size(w, h);
	//seek extension (default = .png)
	std::vector<std::string> exts;
	if (seek_string(filename_parameter_file, 1, exts, "Extension", ""))
		config.extension = exts[0];
	// seek bitdepth (default = 8 for texture and 16 for depth)
	seek_int(filename_parameter_file, config.bit_depth_color, "BitDepthColor", "Element bit depth of raw texture streams");
	seek_int(filename_parameter_file, config.bit_depth_depth, "BitDepthDepth", "Element bit depth of raw depth streams");
	//seek Rescale factor for super resolution
	if (seek_float(filename_parameter_file, rescale, "Precision", "Precision") == 0)
		rescale = 4.0f;

	//seek working color space
	std::vector<std::string> cs;
	if (seek_string(filename_parameter_file, 1, cs, "ColorSpace", "Working Color Space"))
	{
		if(cs[0] == "RGB") color_space = COLORSPACE_RGB;
		else if (cs[0] == "YUV") color_space = COLORSPACE_YUV;
		else throw std::runtime_error("ColorSpace");
	}
	//view synthesis method
	std::vector<std::string> vs;
	if (seek_string(filename_parameter_file, 1, vs, "ViewSynthesisMethod", "View Synthesis Method"))
	{
		if (vs[0] == "Triangles") vs_method = SYNTHESIS_TRIANGLE;
		else throw std::runtime_error("ViewSynthesisMethod");
	}
	//seek blending method
	std::vector<std::string> bl;
	if (seek_string(filename_parameter_file, 1, bl, "BlendingMethod", "Blending Method"))
	{
		if (bl[0] == "Simple") config.blending_method = BLENDING_SIMPLE;
		else if (bl[0] == "MultiSpectral") config.blending_method = BLENDING_MULTISPEC;
		else throw std::runtime_error("BlendingMethod");
	}
	//seek blending factors
	if (seek_float(filename_parameter_file, config.blending_low_freq_factor, "BlendingLowFreqFactor", "Blending Low Freq Factor") == 0)
		config.blending_low_freq_factor = 1.0f;
	if (seek_float(filename_parameter_file, config.blending_high_freq_factor, "BlendingHighFreqFactor", "Blending High Freq Factor") == 0)
		config.blending_high_freq_factor = 4.0f;
	if (seek_float(filename_parameter_file, config.blending_factor, "BlendingFactor", "Blending Factor") == 0)
		config.blending_factor = 1.0f;
	//seek sensor size (default: image width)
	if (seek_float(filename_parameter_file, config.sensor_size, "SensorSize", "Sensor size") == 0)
		config.sensor_size = static_cast<float>(w);

    //seek projection types
    std::vector<std::string> inputProjectionType;
    if (seek_string(filename_parameter_file, 1, inputProjectionType, "InputProjectionType", "Input Projection Type"))
    {
        if (inputProjectionType[0] == "Perspective") config.input_projection_type = PROJECTION_PERSPECTIVE;
        else if (inputProjectionType[0] == "Equirectangular") config.input_projection_type = PROJECTION_EQUIRECTANGULAR;
        else throw std::runtime_error("InputProjectionType");
    }
    std::vector<std::string> virtualProjectionType;
    if (seek_string(filename_parameter_file, 1, virtualProjectionType, "VirtualProjectionType", "Virtual Projection Type"))
    {
        if (virtualProjectionType[0] == "Perspective") config.virtual_projection_type = PROJECTION_PERSPECTIVE;
        else if (virtualProjectionType[0] == "Equirectangular") config.virtual_projection_type = PROJECTION_EQUIRECTANGULAR;
        else throw std::runtime_error("VirtualProjectionType");
    }

	// Frame range
	if (seek_int(filename_parameter_file, config.start_frame, "StartFrame", "First frame (zero-based)") == 0) {
		config.start_frame = 0;
	}
	if (seek_int(filename_parameter_file, config.number_of_frames, "NumberOfFrames", "Number of frames to process") == 0) {
		config.number_of_frames = 1;
	}

    //Read pose trace file
    std::vector<std::string> name_pose_trace_file;
    if (seek_string(filename_parameter_file, 1, name_pose_trace_file, "VirtualPoseTraceName", "Name of pose trace file")) {
        config.pose_trace = pose_traces::ReadPoseTrace(name_pose_trace_file[0] );

        if( static_cast<unsigned>(config.number_of_frames) > config.pose_trace.size() )
            throw std::runtime_error("Error: Number of frames to process is larger then number of entries in pose trace file");

        std::cout << std::endl << "using pose trace with " << config.pose_trace.size() << " entries" << std::endl;
        config.use_pose_trace = true;
    }
    else
        config.use_pose_trace = false;

	print_results(InputCameraParameterFile, config.texture_names, config.depth_names, VirtualCameraParameterFile, number_input_cameras, number_output_cameras);

	config.camerasParameters_in = InputCameraParameterFile[0];
	config.virtualCamerasParameters_in = VirtualCameraParameterFile[0];

}

void Parser::read_ZValues()
{
	using namespace svs;

	if (config.zvalues != "")
		for (int i = 0; i < static_cast<int>(config.InputCameraNames.size()); ++i) {
			float zf, zn;
			if (!seek_znear_zfar(config.zvalues, zf, config.depth_names[i], "FarthestDepthValue", "zfar"))
				zf = 2000.0f;
			if (!seek_znear_zfar(config.zvalues, zn, config.depth_names[i], "NearestDepthValue", "znear"))
				zn = 500.0f;
			config.zfar.push_back(zf);
			config.znear.push_back(zn);
		}
}

void Parser::print_results(
	const std::vector<std::string> & InputCameraParameterFile,
	const std::vector<std::string> & texture_names,
	const std::vector<std::string> & depth_names,
	const std::vector<std::string> & VirtualCameraParameterFile,
	const int number_RGB_cameras,
	const int number_D_cameras
	) const {
	printf("results\n");
	printf("Parameter file: %s\n", filename_parameter_file.c_str());
	for (int i = 0; i < static_cast<int>(InputCameraParameterFile.size()); ++i)
		printf("%s\n", InputCameraParameterFile[i].c_str());
	printf("%d\n", number_RGB_cameras);
	for (int i = 0; i < static_cast<int>(texture_names.size()); ++i)
		printf("%s\n", texture_names[i].c_str());
	for (int i = 0; i < static_cast<int>(depth_names.size()); ++i)
		printf("%s\n", depth_names[i].c_str());
	for (int i = 0; i < static_cast<int>(config.InputCameraNames.size()); ++i)
		printf("%s\n", config.InputCameraNames[i].c_str());
	for (int i = 0; i < static_cast<int>(VirtualCameraParameterFile.size()); ++i)
		printf("%s\n", VirtualCameraParameterFile[i].c_str());
	printf("%d\n", number_D_cameras);
	for (int i = 0; i < static_cast<int>(config.VirtualCameraNames.size()); ++i)
		printf("%s\n", config.VirtualCameraNames[i].c_str());
	printf("%s\n", config.folder_out.c_str());
	for (int i = 0; i < static_cast<int>(config.outfilenames.size()); ++i) {
		printf("%s\n", config.outfilenames[i].c_str());
	}
	for (int i = 0; i < static_cast<int>(config.outmaskedfilenames.size()); ++i) {
		printf("%s\n", config.outmaskedfilenames[i].c_str());
	}
	printf("%d %d\n", config.size.width, config.size.height);
	printf("%d %d\n", config.virtual_size.width, config.virtual_size.height);
	printf("%db color, %db depth\n", config.bit_depth_color, config.bit_depth_depth);
}
