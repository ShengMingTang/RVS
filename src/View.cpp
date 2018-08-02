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

#include "View.hpp"
#include "inpainting.hpp"
#include "image_loading.hpp"

#include <fstream>
#include <iostream>

#define DUMP_MEMORY_USE false

#if DUMP_MEMORY_USE
namespace
{
	std::size_t memory_use = 0;

	std::string const keys[] = { "VmRSS:", "RssFile:", "Threads:" };

	void logProcessStatus()
	{
		std::ifstream stream("/proc/self/status");
		std::string line;

		while (std::getline(stream, line)) {
			for (auto key : keys) {
				if (key == line.substr(0, key.size())) {
					std::cout << line << '\n';
					break;
				}
			}
		}
	}
}
#endif

// Initialize all maps at once
View::View(cv::Mat3f color, cv::Mat1f depth, cv::Mat1f quality, cv::Mat1f validity)
{
	assign(color, depth, quality, validity);
}

View::~View()
{
#if DUMP_MEMORY_USE
	memory_use -= _color.size().area() * _color.elemSize();
	memory_use -= _depth.size().area() * _depth.elemSize();
	memory_use -= _validity.size().area() * _validity.elemSize();
	std::cout << __FUNCTION__ << ": " << std::ldexp(double(memory_use), -30) << " GB\n";
	logProcessStatus(); 
#endif
}

// Initialize all maps at once
void View::assign(cv::Mat3f color, cv::Mat1f depth, cv::Mat1f quality, cv::Mat1f validity)
{
#if DUMP_MEMORY_USE
	memory_use -= _color.size().area() * _color.elemSize();
	memory_use -= _depth.size().area() * _depth.elemSize();
	memory_use -= _validity.size().area() * _validity.elemSize();
#endif

	m_color = color;
	m_depth = depth;
	m_quality = quality;
	m_validity = validity;
	validate();

#if DUMP_MEMORY_USE
	memory_use += _color.size().area() * _color.elemSize();
	memory_use += _depth.size().area() * _depth.elemSize();
	memory_use += _validity.size().area() * _validity.elemSize();
	std::cout << __FUNCTION__ << ": " << std::ldexp(double(memory_use), -30) << " GB\n";
	logProcessStatus();
#endif
}

// Return the texture
cv::Mat3f View::get_color() const
{
	validate();
	CV_Assert(!m_color.empty());
	return m_color;
}

// Return the depth map (same size as texture)
cv::Mat1f View::get_depth() const 
{
	validate();
	CV_Assert(!m_depth.empty());
	return m_depth;
}

// Return the quality map (same size as texture)
cv::Mat1f View::get_quality() const
{
	validate();
	CV_Assert(!m_quality.empty());
	return m_quality;
}

// Return the validity map (same size as texture)
cv::Mat1f View::get_validity() const
{
	validate();
	CV_Assert(!m_validity.empty());
	return m_validity;
}

// Return the size of the texture and depth map
cv::Size View::get_size() const
{
	validate();
	return m_color.size();
}

// Return a mask with all valid depth values
cv::Mat1b View::get_depth_mask() const
{
	return get_depth() > 0.f; // excludes NaN's
}

// Calculate a mask for inpainting
cv::Mat1b View::get_inpaint_mask() const
{
	auto inpaint_mask = cv::Mat1b(get_size(), 255);
	inpaint_mask.setTo(0, get_quality() > 0.f); // excludes NaN's
	return inpaint_mask;
}

// Calculate a mask with valid pixels for masked output
cv::Mat1b View::get_validity_mask(float threshold) const
{
	auto validity_mask = cv::Mat1b(get_size(), 255);
	validity_mask.setTo(0, get_validity() > threshold); // excludes NaN's
	return validity_mask;
}

void View::validate() const
{
	auto size = m_color.size();
	CV_Assert(m_depth.empty() || m_depth.size() == size);
	CV_Assert(m_quality.empty() || m_quality.size() == size);
	CV_Assert(m_validity.size() == m_quality.size());
}

// Load a color image and depth map
InputView::InputView(std::string const& filepath_color, std::string const& filepath_depth, int frame, Parameters const& parameters)
{
	// Load images
	auto color = read_color(filepath_color, parameters.getPaddedSize(), parameters.getColorBitDepth(), frame);
	auto depth = read_depth(filepath_depth, parameters.getPaddedSize(), parameters.getDepthBitDepth(), parameters.getDepthRange()[0], parameters.getDepthRange()[1], frame);

	// Crop padded images
	if (parameters.getSize() != parameters.getPaddedSize()) {
		color = color(parameters.getCropRegion()).clone();
		depth = depth(parameters.getCropRegion()).clone();
	}

	assign(color, depth, cv::Mat1f(), cv::Mat1f());
}
