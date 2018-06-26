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

Extraction of a generalized unproject -> translate/rotate -> project flow

Author  : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

#pragma once

#include <opencv2/core.hpp>

/**
@file transform.hpp
\brief The file containing the warping functions
*/

/**
\brief Translate and rotate the camera in any 3D direction according to the input position map

The image is divided into triangles which vertices are the centers of the pixels. Those triangle that are wrapped according to the input input position, and rasterized.

The output is the new color image, the new corresponding depth map, and a quality map indicating the quality of each pixel.

@param input_color Input color map
@param input_depth Input depth map; valid values are > 0 (invalid may be NaN or <= 0)
@param input_positions Warped coordinate map (result of unproject -> rotate/translate -> project)
@param output_size Output size of the color image
@param[out] depth Output depth map
@param[out] quality Quality metric to drive blending; involves depth and shape of warped triangles (elongated and big = low quality)
@param horizontalWrap
@return Output color map
*/
cv::Mat3f transform_trianglesMethod(cv::Mat3f input_color, cv::Mat1f input_depth, cv::Mat2f input_positions, cv::Size output_size, cv::Mat1f& depth, cv::Mat1f& quality, bool horizontalWrap);
