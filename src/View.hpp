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
@file View.hpp
*/

/**
Class representing an image, optionally a depth map, and optionally a quality map
*/
class View
{
public:
	View() = default;

	/** Initialize all maps at once */
	View(cv::Mat3f, cv::Mat1f, cv::Mat1f, cv::Mat1f);

	~View();

	/** Assign all maps at once */
	void assign(cv::Mat3f, cv::Mat1f, cv::Mat1f, cv::Mat1f);

	/** @return the texture */
	virtual cv::Mat3f get_color() const;

	/** @return the depth map (same size as texture) */
	cv::Mat1f get_depth() const;

	/** @return the quality map (same size as texture) */
	cv::Mat1f get_quality() const;

	/** @return the validity map (same size as texture) */
	cv::Mat1f get_validity() const;

	/** @return the size of the texture and depth map */
	cv::Size get_size() const;

	/** @return a mask with all valid depth values */
	cv::Mat1b get_depth_mask() const;

	/** @return a mask for inpainting */
	cv::Mat1b get_inpaint_mask() const;

	/** @return a mask for invalid masking */
	cv::Mat1b get_validity_mask(float threshold) const;

	virtual float get_max_depth() const { return 1.0; };
	virtual float get_min_depth() const { return 0.0; };

private:
	void validate() const;

	cv::Mat3f _color;
	cv::Mat1f _depth;
	cv::Mat1f _quality;
	cv::Mat1f _validity;
};

/**
Class representing a loaded image and depth map
*/
class InputView : public View
{
public:
	/** Empty input view */
	InputView();

	/** 
	Constructor.
	Loads an input view and its depth map.
	@param filepath_color Color file to be loaded. The format can be png/jpeg or yuv
	@param filepath_depth Depth map to be loaded. The format has to be yuv.
	@param size Size of the image to be loaded.
	@param bit_depth_color Depth of raw texture stream, if input files are in yuv format
	@param bit_depth_depth Depth of raw depth stream, if input files are in yuv format
	@param z_near Lowest depth value
	@param z_far Highest depth value
	@param frame Frame to load.
	*/
	InputView(
		std::string filepath_color, 
		std::string filepath_depth, 
		cv::Size size, 
		int bit_depth_color, 
		int bit_depth_depth,
		float z_near,
		float z_far,
		int frame);

	void load();
	void unload();
	float get_max_depth() const { return z_far; };
	float get_min_depth() const { return z_near; };

private:
	std::string filepath_color;
	std::string filepath_depth;
	cv::Size size;
	int bit_depth_color;
	int bit_depth_depth;
	float z_near;
	float z_far;
	int frame;
};
