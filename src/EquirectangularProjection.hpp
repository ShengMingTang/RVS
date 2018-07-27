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

#pragma once
#include "Projector.hpp"
#include "Unprojector.hpp"
#include "Config.hpp"

#include <opencv2/core.hpp>

/**
@file EquirectangularProjection.hpp
*/

namespace erp
{
/**\brief Compute the angle phi (spherical coordinates) of the pixel
@param hPos Horizontal position in the image
@param imageWidth Image width
@return phi=pi*(0.5-hPos/imageWidth)*/
inline float calculate_phi( float hPos, int imageWidth )
{
    float phi = static_cast<float>( CV_2PI * ( 0.5 - hPos / imageWidth ) );
    return phi;
}

/**\brief Compute the angle theta (spherical coordinates) of the pixel
@param vPos Vertical position in the image space
@param imageHeight Image Height
@return theta=pi*(0.5-vPos/imageHeight)*/
inline float calculate_theta( float vPos, int imageHeight )
{
    float theta = static_cast<float>( CV_PI * ( 0.5f - vPos / imageHeight ) );
    return theta;
}

/**\brief Compute the horizontal image coordinates from spherical coordinates
@param phi Angle in spherical coordinates
@param imageWidth Image width
@return hPos=pi*(imageWidth*(0.5-phi/pi))*/
inline float calculate_horizontal_image_coordinate( float phi, int imageWidth )
{
    float hPos = static_cast<float>( imageWidth * ( 0.5 - phi / CV_2PI) );
    return hPos;
}

/**\brief Compute the vertical image coordinates from spherical coordinates
@param theta Angle in spherical coordinates
@param imageHeight Image Height
@return vPos=pi*(imageHeight*(0.5-theta/pi))*/
inline float calculate_vertical_image_coordinate( float theta, int imageHeight )
{
    float vPos = static_cast<float>( imageHeight * ( 0.5 - theta / CV_PI) );
    return vPos;
}

/**\brief Compute the euclidian coordinates from spherical coordinates
@param phiTheta Angles in sperical coordinates
@return xyz Normalized vector in euclidian coordinates
*/
cv::Vec3f calculate_euclidian_coordinates( const cv::Vec2f& phiTheta );

/**\brief Compute the spherical coordinates from normalized euclidian coordinates
@param xyz_norm Normalized vector in euclidian coordinates 
@return phi theta, the sperical coordinates
*/
cv::Vec2f calculate_spherical_coordinates( const cv::Vec3f& xyz_norm );

/**\brief Equirectangular Unprojector*/
class Unprojector : public ::Unprojector
{
public:
	/**\brief Constructor
	@param parameters Parameters of the View
	@param size Size of the View 
	*/
    Unprojector(Parameters const& parameters, const cv::Size& size);

	/**\brief Create the image of spherical coordinates and normalized euclidian coordinates
	@param size Size of the View
	*/
    void create(cv::Size size);

	/**\brief Project in 3D the normalized euclidian coordinates thanks to the radius map
	@param image_pos Not implemented yet
	@param radiusMap Equirectangular Radius Map
	@return Map of the pixels in euclidian coordinates
	\see calculate_euclidian_coordinates(), calculate_phi(), calculate_theta()
	*/
    cv::Mat3f unproject( cv::Mat2f image_pos, cv::Mat1f radiusMap) const override;

	/**\brief Project in 3D the normalized euclidian coordinates thanks to the radius map
	@param radiusMap Equirectangular Radius Map
	@return Map of the pixels in euclidian coordinates
	\see calculate_euclidian_coordinates(), calculate_phi(), calculate_theta()
	*/
    cv::Mat3f unproject( cv::Mat1f radiusMap) const;

	/**\brief Normalized euclidian coordinates of the pixels*/
    cv::Mat3f verticesXYZNormalized;
	
	/**\brief Spherical coordinates of the pixels*/
    cv::Mat2f phiTheta;
};

/**\brief Equirectangular Projector*/
class Projector : public ::Projector
{
public:
	/**\brief Constructor
	@param parameters Parameters of the View
	@param size Size of the View
	*/
    Projector(Parameters const& parameters, cv::Size size);

	/**\brief Project from 3D to images coordinates and outputs a radius map
	@param verticesXYZ 3D coordinates of the pixels
	@param imRadius Output equirectangular radius map
	@param wrapping_method
	@return Map of the pixels in image coordinates
	\see calculate_horizontal_image_coordinate(), calculate_vertical_image_coordinate(), calculate_spherical_coordinates()
	*/
    cv::Mat2f project( cv::Mat3f vecticesXYZ, cv::Mat1f& imRadius, WrappingMethod& wrapping_method) const override;
};

} // namespace
