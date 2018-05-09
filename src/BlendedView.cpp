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

#include "BlendedView.hpp"
#include "blending.hpp"
#include "Timer.hpp"

#include "iostream"

BlendedViewMultiSpec::BlendedViewMultiSpec()
{
	low_freq = BlendedViewSimple();
	high_freq = BlendedViewSimple();
}


void BlendedViewMultiSpec::blend(SynthetizedView & view)
{
	this->size = view.get_size();
	//split the view into low and high frequency
	cv::Mat low, high;
	cv::Mat mask = view.get_quality() > 0;
	split_frequencies(view.get_color(), low, high, mask);
	SynthetizedView *v_low = view.copy();
	SynthetizedView *v_high = view.copy();
	v_low->set_color(low);
	v_high->set_color(high);
	//blend low and high frequency separately
	low_freq.blend(*v_low);
	high_freq.blend(*v_high);
}


BlendedView::BlendedView()
{
}

BlendedViewSimple::BlendedViewSimple()
{
	is_empty = true;
}


void BlendedViewSimple::blend(SynthetizedView & view)
{ 
	if (is_empty) {
		this->size = view.get_size();
		this->color = view.get_color();
		this->quality = view.get_quality();
		this->depth_prolongation = view.get_mask_depth();
		this->to_inpaint = (quality == 0.0);
		is_empty = false;
		return;
	}
	std::vector<cv::Mat> imgs = { this->color, view.get_color() };
	std::vector<cv::Mat> qualities = { this->quality, view.get_quality() };
	std::vector<cv::Mat> depth_prolongations = { this->depth_prolongation, view.get_mask_depth() };
	cv::Mat col, qual, dp;
	col = blend_img(imgs, qualities, depth_prolongations, empty_rgb_color, qual, dp, to_inpaint, blending_exp);
	color = col;
	quality = qual;
	depth_prolongation = dp;
}
