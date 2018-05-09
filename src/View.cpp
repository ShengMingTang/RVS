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

#include "View.hpp"
#include "Parser.hpp"
#include "inpainting.hpp"
#include "image_loading.hpp"

#include <iostream>

View::View(std::string file_color, std::string file_depth, Parameters parameters, cv::Size size)
{
	this->filename_color = file_color;
	this->filename_depth = file_depth;
	this->parameter = parameters;
	this->size = size;
}

View::View(cv::Mat color, cv::Mat depth, cv::Mat mask_depth, cv::Size size)
{
	this->color = color;
	this->depth = depth;
	this->mask_depth = mask_depth;
	this->size = size;
}

void View::write()
{
	if (color.depth() == CV_32F) {
		cv::Mat col = 255.0f*color;
		cv::imwrite(filename_color, col);
	}
	else
		cv::imwrite(filename_color, color);

}

void View::load()
{
	is_loaded = true;
	//load color image
	color = read_color(filename_color, size);  
	convert_to_float();
	// Load depth images
	depth = read_depth(filename_depth, size, z_near, z_far);
}

void View::unload()
{
	is_loaded = false;
	color.release();
	depth.release();
	mask_depth.release();

}

void View::preprocess_depth()
{
	depth.convertTo(depth, CV_32F);
	mask_depth = (depth == 0.0f);
}

void View::convert_to_float()
{
	color.convertTo(color, CV_32FC3);
	color /= 255.0;
}

void View::convert_to_char()
{
	color *= 255.0;
	color.convertTo(color, CV_8UC3);
}
