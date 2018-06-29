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

#include "Config.hpp"
#include "View.hpp"
#include "Projector.hpp"
#include "SpaceTransformer.hpp"


/**
@file SynthetizedView.hpp
*/

/**
 * \brief View computed with DIBR

 The View obtained from one input reference view
 * */
class SynthetizedView : public View
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

	/** 
	\brief Set SpaceTransformer (input view to world)
	*/
	void setSpaceTransformer(SpaceTransformer const * object) { this->space_transformer = object; };

	/**
	\brief Compute this view from the input View

	The pipeline executes the following steps:
		- Unproject the pixels
		- Compute the transformation
		- Project the pixels locations in image coordinates
		- Rasterize the image with oversampling
	*/
	void compute(View& img);
	
    

protected:
	/**
	\brief Rasterize the warped image, resulting in updates of color, depth and quality maps 
	*/
	virtual void transform(cv::Mat3f input_color, cv::Mat2f input_positions, cv::Mat1f input_depth, 
        cv::Size output_size, WrappingMethod wrapping_method) = 0;

private:
	SpaceTransformer const *space_transformer = nullptr;
};

/**
  \brief The algorithm to compute the view divides the input view in triangles, to avoid non disocclusion holes and get a first inpainting

  This algorithm consists in warping the inputs using directly the disparity rather than performing back 
  and forth 3D re-projection of the pixels, which is sensitive to rounding operations. The translation 
  and rotation between the input and the target cameras are applied to get the result. The translation 
  is obtained by moving each pixel according to its disparity and the rotation is obtained with a homography.

  For a camera movement in the image plane, for each pixel \f$p=(p_x,p_y)\f$, its translation \f$t=(t_x,t_y)\f$ 
  in the image is given by the disparity \f$t=\frac{fT}{d}\f$ where \f$T=(T_x, T_y, T_z)\f$ is the translation 
  vector of the camera, d is the depth at \f$(p_x,p_y)\f$ and f is the focal length. For a movement along the Z 
  axis of the camera, it is \f$t'=\frac{T_z(p+t)}{f\left(d-T_z\right)}\f$

  Three degrees of freedom of translation are obtained by combining the above equations. The rotation is obtained 
  with a simple homography. After the translation, each pixel p is displaced following the rotation of the camera 
  to get the final result \f$p'=(p_x',p_y',1)^T=\frac{1}{R_3\cdot(p_x,p_y,1)^T}R(p_x,p_y,1)^T\f$ where R is the 
  rotation matrix and $R_3$ its last row. With those equations, a map is obtained, indicating the new position 
  of each pixel in the new warped image.

  The input view is divided into triangles with the pixels centers as vertices. The triangles are deformed using 
  the translation and rotation equations before being filled with interpolated colors. The colors are obtained 
  by the tri-linear interpolation between each three vertices of the triangle. Discontinuities between objects 
  creating disocclusions and tangential surfaces may lead to triangles with very elongated shapes, which will 
  not be kept in the final result, as they get eliminated during the blending phase.

  In order to improve the final quality of the synthesized views, during the rasterization of the warped triangles, 
  the synthesized view is upscaled. Hence, the displacement of each pixel according to its depth is more precise 
  and the resulting image, once downscaled to the same size as the input, is sharper. 

  The quality of each pixel is given by the shape of the triangle it belongs to and the depth of the pixel (see BlendedView)
  */
class SynthetizedViewTriangle : public SynthetizedView {
public:
	/**
	\brief Constructor
	*/
	SynthetizedViewTriangle();

protected:
	/** 
	\brief Rasterize the warped image, resulting in updates of color, depth and quality maps
	@param input_color Input color image
	@param input_positions New positions in the image of each pixel
	@param input_depth Input depth map
	@param output_size Size of the output image
	@param wrapping_method Output warping method (perpective or equirectangular, see Projector)
	\see transform_trianglesMethod()
	*/
	virtual void transform(cv::Mat3f input_color, cv::Mat2f input_positions, cv::Mat1f input_depth, 
        cv::Size output_size, WrappingMethod wrapping_method);
};
