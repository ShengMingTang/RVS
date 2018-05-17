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

#include "helpers.hpp"
#include "Config.hpp"

#include "opencv2/core.hpp"
/**
@file transform.hpp
\brief The file containing the warping functions
*/

/**
Translates camera in any 3D direction, the result is a bigger image, to keep more information for a future rotation.
@param img View to translate
@param depth Corresponding depth
@param depth_prologation_mask Corresponding depth_mask (where depth has values)
@param R Optionnal rotation in first view coordinate system to apply at the same time
@param t Translation in first view coordinate system
@param old_cam_mat Input view camera matrix
@param new_cam_mat Output view camera matrix
@param sensor Sensor size
@param new_depth_prologation_mask Output depth mask
@param with_rotation Also applies a rotation
*/
cv::Mat translateBigger_squaresMethod(const cv::Mat& img, const cv::Mat& depth, const cv::Mat& depth_prologation_mask, const cv::Mat& R, const Translation & t, const cv::Mat & old_cam_mat, const cv::Mat & new_cam_mat, float sensor, cv::Mat& depth_inv, cv::Mat& new_depth_prologation_mask, bool with_rotation);

/**
Translates camera in any 3D direction, the result is a bigger image, to keep more information for a future rotation.
@param img View to translate
@param depth Corresponding depth
@param depth_prologation_mask Corresponding depth_mask (where depth has values)
@param R R optionnal rotation in first view coordinate system to apply at the same time
@param t Translation in first view coordinate system
@param old_cam_mat Input view camera matrix
@param new_cam_mat Output view camera matrix
@param sensor Sensor size
@param new_depth_prologation_mask Output depth mask
@param triangle_shape Quality of the warped triangles (elongated and big = low quality)
@param with_rotation Also applies a rotation
*/
cv::Mat translateBigger_trianglesMethod(const cv::Mat& img, const cv::Mat& depth, const cv::Mat& depth_prologation_mask, const cv::Mat& R, const Translation & t, const cv::Mat & old_cam_mat, const cv::Mat & new_cam_mat, float sensor, cv::Mat& depth_inv, cv::Mat& new_depth_prologation_mask, cv::Mat& triangle_shape, bool with_rotation);

/**
creates the new depth map regarding the translation
*/
void translateZ_disp(cv::Mat& d, float z); 

/**
creates the new depth map regarding the rotation
*/
cv::Mat rotate_disp(const cv::Mat& d, cv::Mat new_cam_mat, cv::Mat R); 