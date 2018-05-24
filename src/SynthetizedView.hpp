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

This source file has been added by Koninklijke Philips N.V. for the purpose of
of the 3DoF+ Investigation.
Modifications copyright © 2018 Koninklijke Philips N.V.

Extraction of a generalized unproject -> translate/rotate -> project flow

Author  : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

#pragma once
#include <opencv2/core.hpp>

#include "Config.hpp"
#include "View.hpp"
#include "Projector.hpp"
#include "Unprojector.hpp"

/**
* Virtual view computed with DIBR, blending, inpainting or any other means
* */
class VirtualView : public View
{
public:
	/**
	\brief Default constructor
	*/
	VirtualView();

	/**
	\brief Constructor that directly initialized color, depth and quality maps
	*/
	VirtualView(cv::Mat3f color, cv::Mat1f depth, cv::Mat1f quality);

	/**
	\brief Destructor
	*/
	~VirtualView();

	/**
	\brief Quality per pixel
	@return An image of the same size than the view, indicating the quality of each pixel.
	Pixel of higher quality will be prioritized during blending.
	*/
	cv::Mat1f get_quality() const { return quality; }

protected:
	// The quality map is a side-effect of rasterize
	cv::Mat1f quality;
};

/**
 * View computed with DIBR
 * */
class SynthetizedView : public VirtualView
{
public:
	/**
	\brief Default constructor
	*/
	SynthetizedView();

	/**
	\brief Destructor
	*/
	virtual ~SynthetizedView();

	// Set unprojector (input view to world)
	void setUnprojector(Unprojector const *object) { this->unprojector = object; }

	// Set projector (world to virtual view)
	void setProjector(Projector const *object) { this->projector = object;  }

	/**
	\brief Computes this view from the input View
	*/
	void compute(View& img);

	/**
	\brief Quality per pixel
	@return An image of the same size than the view, indicating the quality of each pixel. 
	Pixel of higher quality will be prioritized during blending.
	*/
	cv::Mat1f get_quality() const { return quality; }
	
protected:
	// Rasterize the warped image, resulting in updates of color, depth and quality maps
	virtual void transform(cv::Mat3f input_color, cv::Mat2f input_positions, cv::Mat1f input_depth, cv::Size output_size) = 0;

private:
	// Unprojector converts input view to world coordinates
	Unprojector const *unprojector = nullptr;

	// Projector converts world to virtual view coordinates
	Projector const *projector = nullptr;
};

/**
 * The algorithm to compute the view divides the input view in triangles, to avoid non disocclusion holes and get a first inpainting
 * The quality of each pixel is given by the shape of the triangle it belongs to and the depth of the pixel
 * */
class SynthetizedViewTriangle : public SynthetizedView {
public:
	SynthetizedViewTriangle();

protected:
	// Rasterize the warped image, resulting in updates of color, depth and quality maps
	virtual void transform(cv::Mat3f input_color, cv::Mat2f input_positions, cv::Mat1f input_depth, cv::Size output_size);
};

/**
* The algorithm to compute the view maps every pixel to its new positions. The pixels are considered as squares.
* The quality of each pixel is given by the depth of the pixel
* */
class SynthetizedViewSquare :public SynthetizedView {
public:
	SynthetizedViewSquare();

protected:
	// Rasterize the warped image, resulting in updates of color, depth and quality maps
	virtual void transform(cv::Mat3f input_color, cv::Mat2f input_positions, cv::Mat1f input_depth, cv::Size output_size);
};
