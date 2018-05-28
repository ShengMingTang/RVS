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

Refactoring within the context of extracting a generalized
unproject -> translate/rotate -> project flow

Author  : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

#pragma once
#include <opencv2/core.hpp>


/**
Class representing an image, optionally a depth map, and optionally a quality map
*/
class View
{
public:
	View() = default;

	// Initialize all maps at once
	View(cv::Mat3f, cv::Mat1f, cv::Mat1f);

	// Assign all maps at once
	void assign(cv::Mat3f, cv::Mat1f, cv::Mat1f);

	// Return the texture
	cv::Mat3f get_color() const;

	// Return the depth map (same size as texture)
	cv::Mat1f get_depth() const;

	// Return the quality map (same size as texture)
	cv::Mat1f get_quality() const;

	// Return the size of the texture and depth map
	cv::Size get_size() const;

	// Return a mask with all valid depth values
	cv::Mat1b get_depth_mask() const;

	// Return a mask for inpainting
	cv::Mat1b get_inpaint_mask() const;

private:
	void validate() const;

	cv::Mat3f _color;
	cv::Mat1f _depth;
	cv::Mat1f _quality;
};

/**
Class representing a loaded image and depth map
*/
class InputView : public View
{
public:
	InputView();

	InputView(
		std::string filepath_color, 
		std::string filepath_depth, 
		cv::Size size, 
		int bit_depth_color, 
		int bit_depth_depth,
		float z_near,
		float z_far);
};
