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



cv::Vec3f erp::calculate_euclidian_coordinates( const cv::Vec2f& phiTheta )
{
    const float& phi   = phiTheta[0];
    const float& theta = phiTheta[1];

    return cv::Vec3f(  std::cos(phi) * std::cos(theta),
                       std::sin(phi)*std::cos(theta),
                       std::sin(theta) );
}

cv::Vec2f erp::calculate_sperical_coordinates( const cv::Vec3f& xyz_norm )
{
    const float& x = xyz_norm[0];
    const float& y = xyz_norm[1];
    const float& z = xyz_norm[2];

    float phi   = std::atan2(y,x);
    float theta = std::asin( z );

    return cv::Vec2f(phi, theta);
}




void erp::Unprojector::create(cv::Size size)
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
        float theta = erp::calculate_theta(vPos, H );

        for (int j = 0; j < W; ++j)
        {
            float hPos = 0.5f + j;
            float phi  = erp::calculate_phi( hPos, W);

            phiTheta(i,j) = cv::Vec2f(phi, theta);
            
            verticesXYZNormalized(i, j) = erp::calculate_euclidian_coordinates( phiTheta(i,j) );
        }
    }


}




cv::Mat3f erp::Unprojector::unproject( cv::Mat1f radiusMap) const 
{
    cv::Size size = radiusMap.size();

    CV_Assert( size == verticesXYZNormalized.size() );

    cv::Mat3f verticesXYZ(size);

    for( int i=0; i < verticesXYZ.rows; ++i )
        for( int j =0; j < verticesXYZ.cols; ++j )
            verticesXYZ(i,j) = radiusMap(i,j) * verticesXYZNormalized(i,j);

    return verticesXYZ;
}

cv::Mat3f erp::Unprojector::unproject( cv::Mat2f image_pos, cv::Mat1f radiusMap) const 
{
    image_pos; // ignore image pos, assume regular grid
    return unproject(radiusMap);
}


cv::Mat2f erp::Projector::project( cv::Mat3f vecticesXYZ, cv::Mat1f& imRadius ) const
{
    auto size = vecticesXYZ.size();

    cv::Mat2f imUV(size);
    imRadius.create(size);

    for(int i = 0; i < size.height; ++i )
        for( int j=0; j < size.width; ++j )
        {
            cv::Vec3f xyz      = vecticesXYZ(i,j);
            float radius       = static_cast<float>( cv::norm(xyz) );
            imRadius(i,j)      = radius;

            cv::Vec3f xyzNorm  = xyz / radius;
            cv::Vec2f phiTheta = erp::calculate_sperical_coordinates(xyzNorm);

            imUV(i,j)[0] = rescale * erp::calculate_horizontal_image_coordinate(phiTheta[0], size.width );
            imUV(i,j)[1] = rescale * erp::calculate_vertical_image_coordinate(phiTheta[1], size.height  );
        }

    return imUV;
}

