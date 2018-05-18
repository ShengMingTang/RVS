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

#include "SynthetizedView.hpp"
#include "transform.hpp"
#include "Timer.hpp"

#include <iostream>

SynthetizedView::SynthetizedView(cv::Mat color, cv::Mat depth_inverse, cv::Mat mask_depth, cv::Size size)
{
	this->color = color;
	this->depth_inverse = depth_inverse;
	this->mask_depth = mask_depth;
	this->size = size;
}

SynthetizedView::~SynthetizedView()
{
}

SynthetizedViewTriangle::SynthetizedViewTriangle(cv::Mat color, cv::Mat depth_inverse, cv::Mat depth_prolongation_mask, cv::Mat triangle_shape, cv::Size size):
	SynthetizedView(color, depth_inverse, depth_prolongation_mask, size)
{
	this->triangle_shape = triangle_shape;
}

void SynthetizedViewTriangle::compute(View& img)
{
	PROF_START("warping");

	Translation real_transl_camera_coord(img.get_parameters().rotation * (cv::Mat) (-img.get_parameters().translation + parameter.translation));
	cv::Mat rotation_camera_coord = parameter.rotation.t()*img.get_parameters().rotation;

    //apply translation and rotation 
	color = translateBigger_trianglesMethod(img.get_color(), img.get_depth(), img.get_mask_depth(), rotation_camera_coord, real_transl_camera_coord, img.get_parameters().camera_matrix, parameter.camera_matrix, rescale*parameter.sensor, depth_inverse, mask_depth, triangle_shape);

    //depth in the new coordinate system: translation+rotation
	translateZ_disp(depth_inverse, -real_transl_camera_coord[2]);
	cv::Mat depth_rotated = rotate_disp(depth_inverse, rescale*parameter.camera_matrix, rotation_camera_coord);

	depth_inverse = depth_rotated;
	triangle_shape *= 100.0f;
	
	PROF_END("warping");
}

SynthetizedViewTriangle * SynthetizedViewTriangle::copy() const
{
	return new SynthetizedViewTriangle(color, depth_inverse, mask_depth, triangle_shape, size);
}

void SynthetizedViewSquare::compute(View& img)
{
	PROF_START("warping");

	Translation real_transl_camera_coord(img.get_parameters().rotation * (cv::Mat) (-img.get_parameters().translation + parameter.translation));
	cv::Mat rotation_camera_coord = parameter.rotation.t()*img.get_parameters().rotation;

    //apply translation and rotation 
	color = translateBigger_squaresMethod(img.get_color(), img.get_depth(), img.get_mask_depth(), rotation_camera_coord, real_transl_camera_coord, img.get_parameters().camera_matrix, parameter.camera_matrix, rescale*parameter.sensor, depth_inverse, mask_depth);

    //depth in the new coordinate system: translation+rotation
	translateZ_disp(depth_inverse, -real_transl_camera_coord[2]);
	cv::Mat depth_rotated = rotate_disp(depth_inverse, rescale*parameter.camera_matrix, rotation_camera_coord);
	depth_inverse = depth_rotated;

	PROF_END("warping");
}

SynthetizedViewSquare * SynthetizedViewSquare::copy() const
{
	return new SynthetizedViewSquare(color, depth_inverse, mask_depth, size);
}

