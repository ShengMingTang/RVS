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

#include "BlendedView.hpp"
#include "blending.hpp"
#include "Timer.hpp"

#include "iostream"

BlendedView::~BlendedView() {}

BlendedViewMultiSpec::BlendedViewMultiSpec(float exp_low_freq, float exp_high_freq)
	: low_freq(exp_low_freq)
	, high_freq(exp_high_freq)
{}

BlendedViewMultiSpec::~BlendedViewMultiSpec() {}

void BlendedViewMultiSpec::blend(View const& view)
{
	// Split color frequencies in low and high band
	cv::Mat3f low_color;
	cv::Mat3f high_color;
	auto mask = cv::Mat1b(view.get_quality() > 0.f);
	split_frequencies(view.get_color(), low_color, high_color, mask);

	// Repack as views
	auto low_view = View(low_color, view.get_depth(), view.get_quality());
	auto high_view = View(high_color, view.get_depth(), view.get_quality());

	// Blend low and high frequency separately
	low_freq.blend(low_view);
	high_freq.blend(high_view);

	// Combine result
	assign(low_freq.get_color() + high_freq.get_color(), cv::Mat1f(), low_freq.get_quality());
}

BlendedViewSimple::BlendedViewSimple(float blending_exp)
	: is_empty(true)
	, blending_exp(blending_exp)
{}

BlendedViewSimple::~BlendedViewSimple() {}

void BlendedViewSimple::blend(View const& view)
{ 
	if (is_empty) {
		is_empty = false;
		assign(view.get_color(), view.get_depth(), view.get_quality());
	}
	else {
		std::vector<cv::Mat> colors = { get_color(), view.get_color() };
		std::vector<cv::Mat> qualities = { get_quality(), view.get_quality() };
		std::vector<cv::Mat> depth_masks = { get_depth_mask(), view.get_depth_mask() };

		cv::Mat quality;
		cv::Mat depth_prolongation_mask; // don't care
		cv::Mat inpaint_mask; // don't care
		
		auto color = blend_img(
			colors, qualities, depth_masks,
			empty_rgb_color,
			/*out*/ quality,
			/*out*/ depth_prolongation_mask,
			/*out*/ inpaint_mask,
			blending_exp);

		assign(color, cv::Mat1f(), quality);
	}
}
