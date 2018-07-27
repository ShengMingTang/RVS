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

#include "PerspectiveProjector.hpp"

#include <limits>
#include <iostream>
auto const NaN = std::numeric_limits<float>::quiet_NaN();

PerspectiveProjector::PerspectiveProjector(Parameters const& parameters, cv::Size size)
	: Projector(size)
	, m_parameters(parameters)
{
}

cv::Mat2f PerspectiveProjector::project(cv::Mat3f world_pos, /*out*/ cv::Mat1f& depth, /*out*/ WrappingMethod& wrapping_method) const
{
	if (world_pos.cols != m_parameters.get_sensor())
		throw std::runtime_error("Situation where sensor size is different from input view width is currently not supported");

	auto M = m_parameters.get_camera_matrix();
	auto fx = M(0, 0);
	auto fy = M(1, 1);
	auto px = M(0, 2);
	auto py = M(1, 2);

	cv::Mat2f image_pos(world_pos.size(), cv::Vec2f::all(NaN));
	depth = cv::Mat1f(world_pos.size(), NaN);

	for (int i = 0; i != world_pos.rows; ++i) {
		for (int j = 0; j != world_pos.cols; ++j) {
			auto xyz = world_pos(i, j);

			// OMAF Referential: x forward, y left, z up
			// Image plane: x right, y down

			if (xyz[0] > 0.f) {
				auto uv = cv::Vec2f(
					-fx * xyz[1] / xyz[0] + px,
					-fy * xyz[2] / xyz[0] + py);

				image_pos(i, j) = uv;

				if (uv[0] >= 0.f && uv[1] >= 0 && uv[0] <= image_pos.cols && uv[1] <= image_pos.rows)
					depth(i, j) = xyz[0];
			}
		}
	}

	wrapping_method = WrappingMethod::NONE;
	return image_pos;
}

cv::Matx33f const & PerspectiveProjector::get_camera_matrix() const
{
	return m_parameters.get_camera_matrix();
}
