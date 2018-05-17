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

#pragma once

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "helpers.hpp"

/**
Class representing a image and its depth map
*/
class View
{
public:
	/**
	\brief Constructor
	*/
	View() {};
	/**
	\brief Constructor 
	@param file_color Filename of the color image
	@param file_depth Filename of the depth map
	@param parameters Camera parameters
	@param size Image size
	*/
	View(std::string file_color, std::string file_depth, Parameters parameters, cv::Size size);
	/**
	\brief Constructor
	@param color Color image
	@param depth Depth map
	@param mask_depth Mask indicating empty values on the depth (for example in a Kinect depth map)
	@param size Image size
	*/
	View(cv::Mat color, cv::Mat depth, cv::Mat mask_depth, cv::Size size);
	/**
	\brief Write the image in the file filename_color
	*/
	void write();
	/**
	\brief Show the image
	*/
	void show() {cv::Mat resized; cv::resize(color, resized, cv::Size(1920, 1080)); cv::imshow(filename_color, resized); cv::waitKey(0); };
	/**
	\brief Load the View from the files filename_color and filename_depth
	*/
	void load();
	/**
	\brief Unload the View (free the images)
	*/
	void unload();
	/**
	\brief Preprocess the 0 values of the depth map
	*/
	void preprocess_depth();
	virtual cv::Mat& get_color() { return color; };
	virtual cv::Mat& get_depth() { return depth; };
	cv::Mat& get_mask_depth() { return mask_depth; };
	cv::Size get_size() { return size; }
	Parameters& get_parameters() { return parameter; };
	void set_z(float z_n, float z_f) { this->z_near = z_n, this->z_far = z_f; };
	void set_color(cv::Mat color_img) { this->color = color_img; };
	
protected:

	void convert_to_float();
	void convert_to_char();
	cv::Mat color;
	cv::Mat depth;
	/**
	\brief Indicates true where the depth is missing in the file 
	(for example in the case of a depth acquired with Kinect). 
	*/
	cv::Mat mask_depth;
	std::string filename_color;
	std::string filename_depth;
	bool is_loaded = false;
	float z_near = 500.0f;
	float z_far = 2000.0f;
	cv::Size size;
	Parameters parameter;
};

