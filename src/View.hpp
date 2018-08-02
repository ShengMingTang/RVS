/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2010-2018, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
Original authors:

Universite Libre de Bruxelles, Brussels, Belgium:
  Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be  Daniele Bonatto, Daniele.Bonatto@ulb.ac.be  Arnaud Schenkel, arnaud.schenkel@ulb.ac.be

Koninklijke Philips N.V., Eindhoven, The Netherlands:
  Bart Kroon, bart.kroon@philips.com
  Bart Sonneveldt, bart.sonneveldt@philips.com
*/

#ifndef _VIEW_HPP_
#define _VIEW_HPP_

#include "Parameters.hpp"

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

	cv::Mat3f m_color;
	cv::Mat1f m_depth;
	cv::Mat1f m_quality;
	cv::Mat1f m_validity;
};

/**
Class representing a loaded image and depth map
*/
class InputView : public View
{
public:
	/** Constructor.
	
	Loads an input view and its depth map.

	@param frame Frame to load.
	@param parameters Camera and video parameters
	*/
	InputView(std::string const& filepath_color, std::string const& filepath_depth, int frame, Parameters const& parameters);
};

#endif