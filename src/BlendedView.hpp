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

#pragma once

#include "SynthetizedView.hpp"

#include "opencv2/core.hpp"


/**
Blending class: blends synthetised views one by one as they are generated.
*/
class BlendedView : public View
{
public:
	/**
	\brief Initialise an empty blending
	*/
	BlendedView();
	/**
	\brief Destructor for BlendedView
	*/
	virtual ~BlendedView(){};
	/**
	\brief Blends with a new synthetised view.	
	*/
	virtual void blend(SynthetizedView& view) = 0;
	/**
	@return an image indicating the pixels without any value (disoclusion/empty values on every image)
	*/
	virtual cv::Mat1b get_inpaint_mask() const = 0;
	/**
	@return the final size the image has to be written
	*/

private:


protected:
};


/**
Simple blending: this algorithm blends the images with the formula per pixel: \f$color=(\sum_i quality_i^a*color_i)/(\sum_i quality_i)\f$ or \f$color=color_{argmax(quality_i)}\f$ if a<0
*/
class BlendedViewSimple :public BlendedView {
public:
	/**
	\brief Initialise an empty blending
	*/
	BlendedViewSimple();
	/**
	\brief Destructor
	*/
	~BlendedViewSimple(){};
	void blend(SynthetizedView& view);
	/**
	Set the value of a in the formula \f$ color=(\sum_i quality_i^a*color_i)/(\sum_i quality_i)\f$ or \f$color=color_{argmax(quality_i)} \f$ if a<0 
	*/
	void set_blending_exp(float exp) { blending_exp = exp; };
	cv::Mat1b get_inpaint_mask() const { return to_inpaint; };
private:
	/** The value of a in the formula \f$color=(\sum_i quality_i^a*color_i)/(\sum_i quality_i)\f$ or \f$color=color_{argmax(quality_i)}\f$ if a<0 */
	float blending_exp;
	bool is_empty;
	cv::Mat quality;
	cv::Mat to_inpaint;
	cv::Mat depth_prolongation;
};

/**
Multispectral blending: this algorithm divides the input images in a low frequency image and a high frequency image, to blend them with different coefficients.
*/
class BlendedViewMultiSpec : public BlendedView{
public:
	/**
	\brief Initialise an empty blending
	*/
	BlendedViewMultiSpec();
	/**
	\brief Destructor
	*/
	~BlendedViewMultiSpec(){};
	void blend(SynthetizedView& view);
	/**
	\brief Set the value of a for low frequency and highfrequency images in the formula \f$color=(\sum_i quality_i^a*color_i)/(\sum_i quality_i)\f$ or \f$color=color_{argmax(quality_i)}\f$ if a<0
	*/
	void set_blending_exp(float exp_low_freq, float exp_high_freq) { low_freq.set_blending_exp(exp_low_freq); high_freq.set_blending_exp(exp_high_freq); };
	cv::Mat3f get_color() const { return low_freq.get_color() + high_freq.get_color();  };
	cv::Mat1b get_inpaint_mask() const { return low_freq.get_inpaint_mask(); };
private:
	BlendedViewSimple low_freq;
	BlendedViewSimple high_freq;
};
