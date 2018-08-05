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
  Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be
  Daniele Bonatto, Daniele.Bonatto@ulb.ac.be
  Arnaud Schenkel, arnaud.schenkel@ulb.ac.be

Koninklijke Philips N.V., Eindhoven, The Netherlands:
  Bart Kroon, bart.kroon@philips.com
  Bart Sonneveldt, bart.sonneveldt@philips.com
*/

#ifndef _PIPELINE_HPP_
#define _PIPELINE_HPP_

#include "View.hpp"
#include "Config.hpp"

class BlendedView;
class SynthesizedView;
class SpaceTransformer;

/**
@file Pipeline.hpp
\brief The file containing the pipeline functions
*/

/**
\brief The pipeline of view synthesis

The pipeline executes the following steps:
	- Parsing the configuration file (see Parser)
	- Loading the input reference views (see InputView, and load_images());
	- View synthesis of the target view once for each input reference view (see SynthesizedView);
	- Blending all the SynthesizedView together by assigning a per-pixel quality to each synthesized view (see BlendedView);
	- Inpainting to fill the remaining holes (see inpaint());
	- Writing the output (see write_color()).
*/
class Pipeline
{
public:
	/**
	\brief Constructor
	@param filepath Configuration file (JSON format)
	*/
	Pipeline(std::string const& filepath);

	/**
	\brief Execution of the view synthesis

	Execution consists in parsing, image loading, view computation (warping, blending, inpainting), view writing
	*/
	void execute();

private:	
	/**
	\brief Computes one frame of a virtual view

	Executes the view view computation (warping, blending, inpainting) and writing.
	@param inputFrame Input frame number of the frame to compute
	@param virtualFrame Virtual (output) frame number of the frame to compute
	@param virtualView Index of the virtual view to compute
	*/
	void computeView(int inputFrame, int virtualFrame, int virtualView);

	std::unique_ptr<BlendedView> createBlender();
	std::unique_ptr<SynthesizedView> createSynthesizer();
	std::unique_ptr<SpaceTransformer> createSpaceTransformer();

	/**
	\brief Dump maps for analysis purposes
	*/
	void dumpMaps(View const& input_image, std::size_t input_idx, std::size_t virtual_idx, View const& synthesizer, View const& blender);

	/** Config of the view synthesis*/
	Config m_config;
};

#endif
