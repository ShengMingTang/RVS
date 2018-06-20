/* ------------------------------------------------------------------------------ -

Copyright © 2018 Koninklijke Philips N.V.

Authors : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

SVS 3DoF+
For the purpose of the 3DoF+ Investigation, SVS is extended to match with the
description of the Reference View Synthesizer(RVS) of the 3DoF+ part of the CTC.
This includes support for unprojecting and projecting ERP images, reading and
writing of 10 - bit YUV 4:2 : 0 texture and depth, parsing of JSON files according to
the 3DoF + CfTM, and unit / integration tests.

Permission is hereby granted, free of charge, to the members of the Moving Picture
Experts Group(MPEG) obtaining a copy of this software and associated documentation
files(the "Software"), to use the Software exclusively within the framework of the
MPEG - I(immersive) and MPEG - I Visual activities, for the sole purpose of
developing the MPEG - I standard.This permission explicitly excludes the rights
to publish, distribute, sublicense, sell, embed into a product or a service and / or
otherwise commercially exploit copies of the Software without the written consent
of the owner(Koninklijke Philips N.V.).

This permission is provided subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies, substantial portions or derivative works of the Software.

------------------------------------------------------------------------------ - */

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
cv::Vec2f calculate_sperical_coordinates( const cv::Vec3f& xyz_norm );

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
	\see calculate_horizontal_image_coordinate(), calculate_vertical_image_coordinate(), calculate_sperical_coordinates()
	*/
    cv::Mat2f project( cv::Mat3f vecticesXYZ, cv::Mat1f& imRadius, WrappingMethod& wrapping_method) const override;
};

} // namespace
