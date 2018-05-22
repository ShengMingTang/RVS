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

This header file has been modified by Koninklijke Philips N.V. for the purpose of
of the 3DoF+ Investigation.
Modifications copyright © 2018 Koninklijke Philips N.V.

Support for n-bit raw texture and depth streams.

Author  : Bart Kroon
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

#include <string>
#include "opencv2/core.hpp"

/**
@file image_loading.hpp
\brief The file containing the image loading functions
*/

int cvdepth_from_bit_depth(int bit_depth);
unsigned max_level(int bit_depth);

/**
Reads a color image (RGB or YUV). Returns a CV_8UC3 image 
@param filename name of the image file
@param size size of the loaded file
@param bit_depth bit depth of raw stream
*/
cv::Mat3f read_color(std::string filename, cv::Size size, int bit_depth);

/**
Reads a depth image: a exr depth file or a YUV disparity file. Returns a float image
@param filename name of the image file
@param size size of the loaded file (for YUV)
@param bit_depth bit depth of raw stream
@param z_near to convert YUV disparity to depth
@param n_far to convert YUV disparity to depth 
*/
cv::Mat1f read_depth(std::string filename, cv::Size size, int bit_depth, float z_near, float z_far, cv::Mat1b& mask_depth);