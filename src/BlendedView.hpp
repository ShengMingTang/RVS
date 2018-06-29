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

#include "SynthetizedView.hpp"

#include <opencv2/core.hpp>

/**
@file BlendedView.hpp
*/

/**
\brief Blending class: blends synthetised views one by one as they are generated.

The blending is done by comparing a per-pixel quality, determined by synthesized depth 
maps and the shape of the triangle a pixel lies in: a pixel of good quality has a low 
depth (e.g. in the foreground) and belongs to a triangle with a regular shape (e.g. 
does not lie in a disocclusion).

Taking the pixel with the maximal quality would give a sharper result, while taking 
the weighted mean is more resistant to errors in the depth maps, color differences 
between the input views and, in the case of navigation leads to smoother navigation, 
with less flickering (BlendedViewSimple). High and low frequencies can be separated with 
a mean blur with a kernel of size of 10% of the image size, and the most adapted 
blending can be applied to each frequency (BlendedViewMultiSpec). 
*/
class BlendedView : public View
{
public:
	virtual ~BlendedView();

	/**
	\brief Blends this view with a new view.	
	*/
	virtual void blend(View const& view) = 0;
#if WITH_OPENGL
	/**
	\brief Transfert textures from OpenGL to OpenCV Matrices (color, validity)
	*/
	void assignFromGL2CV(cv::Size size);
#endif
};

/**
\brief Simple blending

The final color \f$c\f$
of a pixel is:

\f$c=\frac{\sum\limits_{i=0}^nw_i c_i}{\sum\limits_{i=0}^nw_i}\f$ with \f$w_i=\left(\frac{q_i}{d_i}\right)^\alpha\f$ if \f$\alpha\geq 0\f$

or \f$c=argmax_{c_i}\left(w_i\right)\f$ if \f$\alpha<0\f$

where \f$n\f$ is the number of input views, \f$c_i\f$ the color of the pixel in view \f$i\f$, \f$q_i\f$ the quality of the triangle 
the pixel lies in (see transform_trianglesMethod()), 
\f$\alpha\f$ the blending_exp parameter and \f$d_i\f$ the depth at this pixel for the synthesized view \f$i\f$. 

We have 
\f$
q_i=2\mathcal{A}/b^2\f$ if the triangle keeps the same orientation during warping, 0 otherwise. where \f$\mathcal{A}\f$ is the area 
of the triangle, \f$b\f$ is the length of the second longest side of the triangle. 
*/
class BlendedViewSimple : public BlendedView {
public:
	/**
	\brief Initialise an empty blending

	\param blending_exp \f$a\f$ in the formula \f$color=(\sum_i quality_i^a*color_i)/(\sum_i quality_i)\f$ or \f$color=color_{argmax(quality_i)}\f$ if \f$a<0\f$.
	*/
	BlendedViewSimple(float blending_exp);
	
	/**
	\brief Destructor
	*/
	~BlendedViewSimple();

	/**
	\brief Adds a new view to the blended image.

	The resulting image has the per-pixel value: \f$color=(\sum_i quality_i^a*color_i)/(\sum_i quality_i)\f$ or \f$color=color_{argmax(quality_i)}\f$ if \f$a<0\f$, where \f$a\f$ is the blending exponent.
	@param view View to add to the blended image.
	*/
	void blend(View const& view);

private:
	bool is_empty;

	/** The value of a in the formula \f$color=(\sum_i quality_i^a*color_i)/(\sum_i quality_i)\f$ or \f$color=color_{argmax(quality_i)}\f$ if \f$a<0\f$ */
	float blending_exp;

	// Depth mask
	cv::Mat1b _depth_mask;
};

/**
\brief Multispectral blending: this algorithm divides the input images in a low frequency image and a high frequency image, to blend them with different coefficients (exp_high_freq and exp_low_freq) with BlendedViewSimple algorithm.

The final color \f$c\f$
of a pixel is hence:
\f$c=c_{high}+c_{low}\f$

where \f$c_{high}\f$, \f$c_{low}\f$ are the resulting colors of high and low frequencies simple blending. 
*/
class BlendedViewMultiSpec : public BlendedView {
public:
	/**
	\brief Initialise an empty blending
	@param exp_low_freq Blending factor for the low frequencies
	@param exp_high_freq Blending factor for the high frequencies
	*/
	BlendedViewMultiSpec(float exp_low_freq, float exp_high_freq);
	/**
	\brief Destructor
	*/
	~BlendedViewMultiSpec();
	
	/**
	\brief Adds a new view to the blended image.

	The new view is divided in high and low frequencies with a mean blur kernel of size \f$\frac{1}{20}\f$ of the image size (see split_frequencies()). 
	Each frequency is blended separately as a BlendedViewSimple.  
	@param view View to add to the blended image.
	*/
	void blend(View const& view);

private:
	BlendedViewSimple low_freq;
	BlendedViewSimple high_freq;
};
