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

#include "Parameters.hpp"
#include "PoseTraces.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

namespace
{
	cv::Matx33f rotationMatrixFromRotationAroundX(float rx)
	{
		return cv::Matx33f(
			     1.f,     0.f,      0.f,
			     0.f, cos(rx), -sin(rx),
			     0.f, sin(rx),  cos(rx));
	}

	cv::Matx33f rotationMatrixFromRotationAroundY(float ry)
	{
		return cv::Matx33f(
			 cos(ry),     0.f,  sin(ry),
			     0.f,     1.f,      0.f,
			-sin(ry),     0.f,  cos(ry));
	}

	cv::Matx33f rotationMatrixFromRotationAroundZ(float rz)
	{
		return cv::Matx33f(
			cos(rz), -sin(rz),      0.f,
			sin(rz),  cos(rz),      0.f,
			    0.f,      0.f,      1.f);
	}

	cv::Matx33f EulerAnglesToRotationMatrix(cv::Vec3f rotation)
	{
		return
			rotationMatrixFromRotationAroundZ(rotation[2]) *
			rotationMatrixFromRotationAroundY(rotation[1]) *
			rotationMatrixFromRotationAroundX(rotation[0]);
	}
}

Parameters::Parameters() {}

Parameters Parameters::readFrom(json::Node root)
{
	Parameters parameters;

	parameters.setProjectionFrom(root);
	parameters.setPositionFrom(root);
	parameters.setRotationFrom(root);
	parameters.setDepthRangeFrom(root);
	parameters.setResolutionFrom(root);
	parameters.setBitDepthColorFrom(root);
	parameters.setBitDepthDepthFrom(root);
	parameters.setHorRangeFrom(root);
	parameters.setVerRangeFrom(root);
	parameters.setCropRegionFrom(root);
	parameters.setFocalFrom(root);
	parameters.setPrinciplePointFrom(root);	

	return parameters;
}

ProjectionType Parameters::getProjectionType() const
{
	return m_projectionType;
}

cv::Vec3f Parameters::getRotation() const
{
	return m_rotation;
}

void Parameters::setRotation(cv::Vec3f rotation)
{
	m_rotation = rotation;
}

cv::Matx33f Parameters::getRotationMatrix() const
{
	auto const radperdeg = 0.01745329252f;
	return EulerAnglesToRotationMatrix(radperdeg * m_rotation);
}

cv::Vec3f Parameters::getPosition() const
{
	return m_position;
}

void Parameters::setPosition(cv::Vec3f)
{
}

cv::Vec2f Parameters::getDepthRange() const
{
	return m_depthRange;
}

cv::Size Parameters::getPaddedSize() const
{
	return m_resolution;
}

cv::Size Parameters::getSize() const
{
	return m_cropRegion.size();
}

cv::Rect Parameters::getCropRegion() const
{
	return m_cropRegion;
}

int Parameters::getColorBitDepth() const
{
	return m_bitDepthColor;
}

int Parameters::getDepthBitDepth() const
{
	return m_bitDepthDepth;
}

cv::Vec2f Parameters::getHorRange() const
{
	assert(m_projectionType == ProjectionType::equirectangular);
	return m_horRange;
}

cv::Vec2f Parameters::getVerRange() const
{
	assert(m_projectionType == ProjectionType::equirectangular);
	return m_verRange;
}

bool Parameters::isFullHorRange() const
{
	return m_isFullHorRange;
}

cv::Vec2f Parameters::getFocal() const
{
	assert(m_projectionType == ProjectionType::perspective);
	return m_focal;
}

cv::Vec2f Parameters::getPrinciplePoint() const
{
	assert(m_projectionType == ProjectionType::perspective);
	return m_principlePoint - cv::Vec2f(cv::Point2f(m_cropRegion.tl()));
}

void Parameters::setProjectionFrom(json::Node root)
{
	auto projection = root.require("Projection");

	if (projection.asString() == "Equirectangular") {
		m_projectionType = ProjectionType::equirectangular;
	}
	else if (projection.asString() == "Perspective") {
		m_projectionType = ProjectionType::perspective;
	}
	else {
		throw std::runtime_error("Unknown projection type");
	}
}

namespace
{
	template<int N> cv::Vec<float, N> asFloatVec(json::Node node)
	{
		if (node.size() != N) {
			throw std::runtime_error("JSON parser: Expected a vector of floats");
		}
		cv::Vec<float, N> v;
		for (int i = 0; i != N; ++i) {
			v[i] = node.at(i).asFloat();
		}
		return v;
	}

	template<int N> cv::Vec<int, N> asIntVec(json::Node node)
	{
		if (node.size() != N) {
			throw std::runtime_error("JSON parser: Expected a vector of floats");
		}
		cv::Vec<int, N> v;
		for (int i = 0; i != N; ++i) {
			v[i] = node.at(i).asInt();
		}
		return v;
	}
}

void Parameters::setPositionFrom(json::Node root)
{
	m_position = asFloatVec<3>(root.require("Position"));
}

void Parameters::setRotationFrom(json::Node root)
{
	m_rotation = asFloatVec<3>(root.require("Rotation"));
}

void Parameters::setDepthRangeFrom(json::Node root)
{
	m_depthRange = asFloatVec<2>(root.require("Depth_range"));
}

void Parameters::setResolutionFrom(json::Node root)
{
	m_resolution = cv::Size(asIntVec<2>(root.require("Resolution")));
}

void Parameters::setBitDepthColorFrom(json::Node root)
{
	m_bitDepthColor = root.require("BitDepthColor").asInt();
}

void Parameters::setBitDepthDepthFrom(json::Node root)
{
	m_bitDepthDepth = root.require("BitDepthDepth").asInt();
}

void Parameters::setHorRangeFrom(json::Node root)
{
	if (m_projectionType == ProjectionType::equirectangular) {
		m_horRange = asFloatVec<2>(root.require("Hor_range"));

		m_isFullHorRange = false;
		try {
			m_isFullHorRange = asIntVec<2>(root.require("Hor_range")) == cv::Vec2i(-180, 180);
		}
		catch (std::runtime_error&) {}
	}
}

void Parameters::setVerRangeFrom(json::Node root)
{
	if (m_projectionType == ProjectionType::equirectangular) {
		m_verRange = asFloatVec<2>(root.require("Ver_range"));
	}
}

void Parameters::setCropRegionFrom(json::Node root)
{
	auto node = root.optional("Crop_region");
	if (node) {
		auto values = asIntVec<4>(node);
		m_cropRegion = cv::Rect(values[0], values[1], values[2], values[3]);
	}
	else {
		m_cropRegion = cv::Rect(cv::Point(), m_resolution);
	}
}

void Parameters::setFocalFrom(json::Node root)
{
	if (m_projectionType == ProjectionType::perspective) {
		m_focal = asFloatVec<2>(root.require("Focal"));
	}
}

void Parameters::setPrinciplePointFrom(json::Node root)
{
	if (m_projectionType == ProjectionType::perspective) {
		m_principlePoint = asFloatVec<2>(root.require("Principle_point"));
	}
}
