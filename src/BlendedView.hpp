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

#include "SynthetizedView.hpp"

#include <opencv2/core.hpp>


/**
Blending class: blends synthetised views one by one as they are generated.
*/
class BlendedView : public View
{
public:
	virtual ~BlendedView();

	/**
	\brief Blends with a new virtual view.	
	*/
	virtual void blend(View const& view) = 0;
};

/**
Simple blending: this algorithm blends the images with the formula per pixel: \f$color=(\sum_i quality_i^a*color_i)/(\sum_i quality_i)\f$ or \f$color=color_{argmax(quality_i)}\f$ if a<0
*/
class BlendedViewSimple : public BlendedView {
public:
	/**
	\brief Initialise an empty blending
	*/
	BlendedViewSimple(float blending_exp);
	
	/**
	\brief Destructor
	*/
	~BlendedViewSimple();

	void blend(View const& view);

private:
	bool is_empty;

	/** The value of a in the formula \f$color=(\sum_i quality_i^a*color_i)/(\sum_i quality_i)\f$ or \f$color=color_{argmax(quality_i)}\f$ if a<0 */
	float blending_exp;
};

/**
Multispectral blending: this algorithm divides the input images in a low frequency image and a high frequency image, to blend them with different coefficients.
*/
class BlendedViewMultiSpec : public BlendedView {
public:
	/**
	\brief Initialise an empty blending
	*/
	BlendedViewMultiSpec(float exp_low_freq, float exp_high_freq);
	/**
	\brief Destructor
	*/
	~BlendedViewMultiSpec();
	
	void blend(View const& view);

private:
	BlendedViewSimple low_freq;
	BlendedViewSimple high_freq;
};
