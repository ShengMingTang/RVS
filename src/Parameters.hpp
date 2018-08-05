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

#ifndef _PARAMETERS_HPP_
#define _PARAMETERS_HPP_

#include "JsonParser.hpp"
#include <opencv2/core.hpp>

/**
@file Parameters.hpp
\brief Definition of extrinsic and intrinsic camera parameters as well as video parameters
*/

/**\brief Projection type

\see Projector*/
enum class ProjectionType {
	perspective = 0,
	equirectangular = 1
};

/** Camera and video parameters */
class Parameters {
public:
	/** Camera parameters
	@param parameters The camera and video parameters of this camera	
	*/
	static Parameters readFrom(json::Node parameters);

	/** The projection type */
	ProjectionType getProjectionType() const;

	/** Extrinsic parameter of rotation (Euler angles, degrees) */
	cv::Vec3f getRotation() const;

	/** Set rotation (Euler angles, degrees) */
	void setRotation(cv::Vec3f);

	/** Get rotation as a matrix */
	cv::Matx33f getRotationMatrix() const;

	/** Extrinsic parameter of translation */
	cv::Vec3f getPosition() const;

	/** Set a new translation */
	void setPosition(cv::Vec3f);

	/** Depth range

	perspective: [znear, zfar]
	equirectangular: [Rmin, Rmax] */
	cv::Vec2f getDepthRange() const;

	/** Padded image size (before cropping) */
	cv::Size getPaddedSize() const;

	/** Image size after cropping */
	cv::Size getSize() const;

	/** Image parameter of crop region:

	perspective: principle point relates to uncropped region
	equirectangular: angular ranges relate to cropped region */
	cv::Rect getCropRegion() const;

	/** Texture bit depth */
	int getColorBitDepth() const;

	/** Depth map bit depth */
	int getDepthBitDepth() const;

	/** Horizontal angular range (degrees) */
	cv::Vec2f getHorRange() const;

	/** Vertical angular range (degrees) */
	cv::Vec2f getVerRange() const;

	/** Is full horizontal angular range? */
	bool isFullHorRange() const;

	/** Intrinsic parameter of focal length (perspective) */
	cv::Vec2f getFocal() const;

	/** Intrinsic parameter of principle point (perspective)
	
	The value returned is already adjusted to refer to the cropped region. */
	cv::Vec2f getPrinciplePoint() const;

	/** Print a description */
	void printTo(std::ostream& stream) const;

private:
	Parameters();

	void setProjectionFrom(json::Node root);
	void setPositionFrom(json::Node root);
	void setRotationFrom(json::Node root);
	void setDepthRangeFrom(json::Node root);
	void setResolutionFrom(json::Node root);
	void setBitDepthColorFrom(json::Node root);
	void setBitDepthDepthFrom(json::Node root);
	void setHorRangeFrom(json::Node root);
	void setVerRangeFrom(json::Node root);
	void setCropRegionFrom(json::Node root);
	void setFocalFrom(json::Node root);
	void setPrinciplePointFrom(json::Node root);

	/** Validate some fields of the JSON format that RVS is not effectively using */
	static void validateUnused(json::Node root);

	ProjectionType m_projectionType;
	cv::Vec3f m_position;
	cv::Vec3f m_rotation;
	cv::Vec2f m_depthRange;
	cv::Size m_resolution;
	int m_bitDepthColor;
	int m_bitDepthDepth;
	cv::Vec2f m_horRange;
	cv::Vec2f m_verRange;
	bool m_isFullHorRange;
	cv::Rect m_cropRegion;
	cv::Vec2f m_focal;
	cv::Vec2f m_principlePoint;
};

#endif