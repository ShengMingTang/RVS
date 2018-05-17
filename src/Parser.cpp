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

#include "Parser.hpp"
#include <iostream>
#include <fstream>



int seek_string(std::string ParameterFile, int n, std::vector<std::string>& str, std::string key, std::string err_msg) {
	std::ifstream file(ParameterFile);
	if (file.is_open() == 0) {
		printf("Can't open %s\n", ParameterFile.c_str());
		abort();
	}
	std::string id;
	while (!file.eof() && file >> id) {
		if (id == key) {
			for (int i = 0; i < n; ++i) {
				std::string c;
				file >> c;
				str.push_back(c);
			}
			file.close();
			return 1;
		}
	}
	printf("%s: %s  not found in the parameter file.\n", key.c_str(), err_msg.c_str());
	file.close();
	return 0;
}

int seek_int(std::string ParameterFile, int& n, std::string key, std::string err_msg) {
	std::ifstream file(ParameterFile);

	if (file.is_open() == 0) {
		printf("Can't open %s\n", ParameterFile.c_str());
		abort();
	}
	std::string id;
	while (!file.eof() && file >> id) {
		if (id == key) {
			file >> n;
			file.close();
			return 1;
		}
	}
	printf("%s: %s  not found in the parameter file.\n", key.c_str(), err_msg.c_str());
	file.close();
	return 0;
}
int seek_float(std::string ParameterFile, float& f, std::string key, std::string err_msg) {
	std::ifstream file(ParameterFile);

	if (file.is_open() == 0) {
		printf("Can't open %s\n", ParameterFile.c_str());
		abort();
	}
	std::string id;
	while (!file.eof() && file >> id) {
		if (id == key) {
			file >> f;
			file.close();
			return 1;
		}
	}
	printf("%s: %s  not found in the parameter file.\n", key.c_str(), err_msg.c_str());
	file.close();
	return 0;
}

bool seek_znear_zfar(std::string ParameterFile, float& z, std::string DepthFile, std::string key, std::string err_msg) {
	std::ifstream file(ParameterFile);
	if (file.is_open() == 0) {
		printf("Can't open %s\n", ParameterFile.c_str());
		return 0;
	}
	std::string id;
	while (!file.eof() && file >> id) {
		if (id.find(DepthFile) != std::string::npos) {
			while (!file.eof() && file >> id) {
				if (id == key) {
					file >> z;
					file.close();
					return true;
				}
			}
		}
	}
	printf("%s was not found: using default values\n", err_msg.c_str());
	file.close();
	return false;
}

int find_cam(std::string CameraParameterFile, cv::Mat & r, cv::Vec3f & t, cv::Mat & camMat, std::string cameraId) {
	std::ifstream file(CameraParameterFile);

	if (file.is_open() == 0) {
		printf("Can't open %s\n", CameraParameterFile.c_str());
		abort();
	}
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
			file.close();
			return 1;
		}
	}
	printf("Camera %s not found\n", cameraId.c_str());
	file.close();
	return 0;
}

//read cameras names, positions and rotation
void read_cameras_paramaters(std::string filename, std::vector<std::string>& camnames, std::vector<Parameters>& params, float& sensor_size) {
	//read all cameras
	if (camnames.size() == 0 || camnames[0] == "ALL")
	{
		camnames = {};
		std::ifstream file(filename);
		if (file.is_open() == 0) {
			printf("Can't open %s\n", filename.c_str());
			abort();
		}
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
			params.push_back(Parameters(r, t, cam_mat, sensor_size));
		}
		file.close();
		return;
	}
	//read only specified cameras
	for (int i = 0; i < static_cast<int>(camnames.size()); ++i) {
		cv::Mat r;
		cv::Vec3f t;
		cv::Mat cam_mat;
		if (find_cam(filename, r, t, cam_mat, camnames[i])) {
			params.push_back(Parameters(r, t, cam_mat, sensor_size));
		}
	}
}
//string corresponding to camera name, camera matrix and camera world matrix
std::string camera_parameter(cv::Mat R, cv::Vec3f t, cv::Mat camMat, std::string cam_name) {
	//t *= 1000.0;
	std::string camparams;
	//camparams = String(name) + "\n1094.8866328999999951 0 934.63286315000004834\n0 1095.9840302000000065 543.2474342599999772\n0 0 1\n0\n0\n";
	camparams = cam_name + "\n" +
		std::to_string(camMat.at<float>(0, 0)) + " " + std::to_string(camMat.at<float>(0, 1)) + " " + std::to_string(camMat.at<float>(0, 2)) + "\n" +
		std::to_string(camMat.at<float>(1, 0)) + " " + std::to_string(camMat.at<float>(1, 1)) + " " + std::to_string(camMat.at<float>(1, 2)) + "\n" +
		std::to_string(camMat.at<float>(2, 0)) + " " + std::to_string(camMat.at<float>(2, 1)) + " " + std::to_string(camMat.at<float>(2, 2)) + "\n" +
		"0\n0\n";
	camparams +=
		std::to_string(R.at<float>(0, 0)) + " " + std::to_string(R.at<float>(0, 1)) + " " + std::to_string(R.at<float>(0, 2)) + " " + std::to_string(t[0]) + "\n" +
		std::to_string(R.at<float>(1, 0)) + " " + std::to_string(R.at<float>(1, 1)) + " " + std::to_string(R.at<float>(1, 2)) + " " + std::to_string(t[1]) + "\n" +
		std::to_string(R.at<float>(2, 0)) + " " + std::to_string(R.at<float>(2, 1)) + " " + std::to_string(R.at<float>(2, 2)) + " " + std::to_string(t[2]) + "\n\n";
	return std::string(camparams);
}

Parser::Parser(const std::string & filename)
	: filename_parameter_file(filename)
{
	//this->config.size.width = size.width;
	//this->config.size.height = size.height;
	
	if (is_SVS_file(filename))
	{
		read_SVS_config_file();
		read_ZValues();
	}
	else
	{
		read_vsrs_config_file();
	}


	//get input cameras parameters
	read_cameras_paramaters(config.camerasParameters_in, config.InputCameraNames, config.params_real, config.sensor_size);

	//get virtual cameras parameters to render
	read_cameras_paramaters(config.virtualCamerasParameters_in, config.VirtualCameraNames, config.params_virtual, config.sensor_size);

	
	generate_output_filenames();
}

Parser::~Parser()
{
}




void Parser::generate_output_filenames() {
	if (config.outfilenames.size() != config.VirtualCameraNames.size()) { // ALL?
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
	//for (int i = 0; i < config.VirtualCameraNames.size(); ++i) {
	//	if (config.outfilenames[i].find("yuv") != std::string::npos)
	//		config.outfilenames[i] = config.outfilenames[i].substr(0, config.outfilenames[i].size() - 3) + config.extension;
	//}

}

bool Parser::is_SVS_file(const std::string & filename) const
{
	int n = 0;
	seek_int(filename, n, "SVSFile", "This is a VSRS file.");
	return !!n;
}

void Parser::read_vsrs_config_file() {
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
	//seek l/r Depth images
	seek_string(filename_parameter_file, 1, config.depth_names, "LeftDepthMapName", "Input depth left file name");
	seek_string(filename_parameter_file, 1, config.depth_names, "RightDepthMapName", "Input depth right file name");
	//seek l/r input cameras names
	seek_string(filename_parameter_file, 1, config.InputCameraNames, "LeftCameraName", "Input left camera name");
	seek_string(filename_parameter_file, 1, config.InputCameraNames, "RightCameraName", "Input right camera name");

	//seek virtual camera file
	seek_string(filename_parameter_file, 1, VirtualCameraParameterFile, "CameraParameterFile", "Camera parameter file");
	//seek virtual cameras names
	seek_string(filename_parameter_file, 1, config.VirtualCameraNames, "VirtualCameraName", "Output cameras names");
	//seek folder for output
	if (seek_string(filename_parameter_file, 1, folders_out, "OuputDir", "Output directory"))
		config.folder_out = folders_out[0];
	//seek filesnames for output
	seek_string(filename_parameter_file, 1, config.outfilenames, "OutputVirtualViewImageName", "Output file names");

	//seek inpainting (default = 0)
	seek_int(filename_parameter_file, config.inpainting, "IvsrsInpaint", "");
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

	print_results(InputCameraParameterFile, config.texture_names, config.depth_names, VirtualCameraParameterFile, 2, 1);
	
	config.camerasParameters_in = InputCameraParameterFile[0];
	config.virtualCamerasParameters_in = VirtualCameraParameterFile[0];
}


void Parser::read_SVS_config_file() {

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

	//seek inpainting (default = 0)
	seek_int(filename_parameter_file, config.inpainting, "Inpaint", "");
	//seek w,h (default = 1920x1080)
	int w, h;
	if (seek_int(filename_parameter_file, w, "Width", "") && seek_int(filename_parameter_file, h, "Height", ""))
		config.size = cv::Size(w, h);
	//seek extension (default = .png)
	std::vector<std::string> exts;
	if (seek_string(filename_parameter_file, 1, exts, "Extension", ""))
		config.extension = exts[0];
	//seek image_bigger_ratio (useful for translation computation). default =2
	if (seek_float(filename_parameter_file, image_bigger_ratio, "TranslationRatio", "Translation ratio") == 0)
		image_bigger_ratio = 4.0;
	//seek Rescale factor for super resolution
	if (seek_float(filename_parameter_file, rescale, "Precision", "Precision") == 0)
		rescale = 4.0f;

	//seek working color space
	std::vector<std::string> cs;
	if (seek_string(filename_parameter_file, 1, cs, "ColorSpace", "Working Color Space") > 0)
	{
		if(cs[0] == "RGB") color_space = COLORSPACE_RGB;
		else if (cs[0] == "YUV") color_space = COLORSPACE_YUV;
	}
	//view synthesis method
	std::vector<std::string> vs;
	if (seek_string(filename_parameter_file, 1, vs, "ViewSynthesisMethod", "View Synthesis Method") > 0)
	{
		if (vs[0] == "Triangles") vs_method = SYNTHESIS_TRIANGLE;
		else if (vs[0] == "Squares") vs_method = SYNTHESIS_SQUARE;
		else if (vs[0] == "VSRS") vs_method = SYNTHESIS_VSRS;
	}
	//seek blending method
	std::vector<std::string> bl;
	if (seek_string(filename_parameter_file, 1, bl, "BlendingMethod", "Blending Method") > 0)
	{
		if (bl[0] == "Simple") config.blending_method = BLENDING_SIMPLE;
		else if (bl[0] == "MultiSpectral") config.blending_method = BLENDING_MULTISPEC;
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

	
	print_results(InputCameraParameterFile, config.texture_names, config.depth_names, VirtualCameraParameterFile, number_input_cameras, number_output_cameras);

	config.camerasParameters_in = InputCameraParameterFile[0];
	config.virtualCamerasParameters_in = VirtualCameraParameterFile[0];

}

void Parser::read_ZValues()
{
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
	printf("%d\n", config.size.width);
	printf("%d\n", config.size.height);
}
