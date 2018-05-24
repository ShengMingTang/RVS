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

#include "opencv2/core/core.hpp"

namespace erp
{

inline float calculate_phi( float hPos, int imageWidth )
{
    float phi = static_cast<float>( CV_2PI * ( 0.5 - hPos / imageWidth ) );
    return phi;
}

inline float calculate_theta( float vPos, int imageHeight )
{
    float theta = static_cast<float>( CV_PI * ( 0.5f - vPos / imageHeight ) );
    return theta;
}

inline float calculate_horizontal_image_coordinate( float phi, int imageWidth )
{
    float hPos = static_cast<float>( imageWidth * ( 0.5 - phi / CV_2PI) );
    return hPos;
}

inline float calculate_vertical_image_coordinate( float theta, int imageHeight )
{
    float vPos = static_cast<float>( imageHeight * ( 0.5 - theta / CV_PI) );
    return vPos;
}


cv::Vec3f calculate_euclidian_coordinates( const cv::Vec2f& phiTheta );

cv::Vec2f calculate_sperical_coordinates( const cv::Vec3f& xyz_norm );


struct BackProjector
{
    cv::Mat3f verticesXYZ;
    cv::Mat3f verticesXYZNormalized;
    cv::Mat2f phiTheta;

    cv::Mat3f calculate_vertices( cv::Mat1f radiusMap);
    void      calculate_normalized_euclidian_coordinates(cv::Size size);

};


struct Projector
{
    cv::Mat2f imUV;
    cv::Mat1f imRadius;
    cv::Mat2f imPhiTheta;

    cv::Mat2f project_to_image_coordinates_uv( cv::Mat3f vecticesXYZ, float rescale);

};
    



} // namespace

