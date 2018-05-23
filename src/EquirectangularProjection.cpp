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

#include "EquirectangularProjection.hpp"



cv::Vec3f erp::CalcEuclidanCoordinates( const cv::Vec2f& phiTheta )
{
    const float& phi   = phiTheta[0];
    const float& theta = phiTheta[1];

    return cv::Vec3f(  std::cos(phi) * std::cos(theta),
                       std::sin(phi)*std::cos(theta),
                       std::sin(theta) );
}

cv::Vec2f erp::CalcSphereCoordinates( const cv::Vec3f& xyz_norm )
{
    const float& x = xyz_norm[0];
    const float& y = xyz_norm[1];
    const float& z = xyz_norm[2];

    float phi   = std::atan2(y,x);
    float theta = std::asin( z );

    return cv::Vec2f(phi, theta);
}



void erp::MeshEquirectangular::CalcNormalizedEuclidianCoordinates(cv::Size size)
{
    if( verticesXYZNormalized.size() == size )
        return;

    phiTheta.create(size);

    int H = size.height;
    int W = size.width;

    verticesXYZNormalized.create(size);
    for (int i = 0; i < H; ++i)
    {
        float vPos  = 0.5f + i;
        float theta = CV_PI * ( 0.5f - vPos / H );

        for (int j = 0; j < W; ++j)
        {
            float hPos = 0.5f + j;
            float phi  = CV_2PI * ( 0.5 - hPos / W );

            phiTheta(i,j) = cv::Vec2f(phi, theta);
            
            verticesXYZNormalized(i, j) = erp::CalcEuclidanCoordinates( phiTheta(i,j) );
        }
    }


}




cv::Mat3f erp::MeshEquirectangular::CalculateVertices( cv::Mat1f radiusMap)
{
    cv::Size size = radiusMap.size();

    CalcNormalizedEuclidianCoordinates(size);

    verticesXYZ.create(size);

    for( int i=0; i < verticesXYZ.rows; ++i )
        for( int j =0; j < verticesXYZ.cols; ++j )
            verticesXYZ(i,j) = radiusMap(i,j) * verticesXYZNormalized(i,j);


    return verticesXYZ;
}

