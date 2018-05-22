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


namespace
{
    // Coordinates and indices are designed to sample into the disparity map
    // size == 4, wrap T: { 1/8, 3/8, 5/8, 7/8, 9/8 }      (size + 1), sample points { 1/8, 3/8, 5/8, 7/8, 1/8 }     , pixel indices { 0, 1, 2, 3, 0 }
    //          , wrap F: {  0 , 1/8, 3/8, 5/8, 7/8,  1  } (size + 2), sample points { 1/8, 1/8, 3/8, 5/8, 7/8, 7/8 }, pixel indices { 0, 0, 1, 2, 3, 3 }

    float gridPoint(int index, int size, bool wrap)
    {
        if (wrap) {
            return float((0.5 + index)/size);
        }
        else {
            return std::max(0.f, std::min(1.f, float((-0.5 + index)/size)));
        }
    }

    int gridIndex(int index, int size, bool wrap)
    {
        if (wrap) {
            return index == size ? 0 : index;
        }
        else {
            return std::max(0, std::min(size - 1, index - 1));
        }

    }
}


void MeshEquirectangular::CalculatePolarAngles(cv::Size size)
{
    if( polarAngles.size() == size )
        return;

    polarAngles.create(size);
    for (int i = 0; i != polarAngles.rows; ++i)
    {
        float v = gridPoint(i, size.height, wrap[1]);
        auto theta = float(CV_PI - CV_PI*v);

        for (int j = 0; j != polarAngles.cols; ++j)
        {
            float u = gridPoint(j, size.width, wrap[0]);
            auto phi = float(CV_2PI*u);

            polarAngles(i, j) = cv::Vec3f( std::sin(theta)*std::sin(phi),
                -std::cos(theta),
                -std::sin(theta)*std::cos(phi));
        }
    }
}




cv::Mat3f MeshEquirectangular::CalculateVertices( cv::Mat1f radiusMap)
{
    cv::Size size = radiusMap.size();

    CalculatePolarAngles(size);

    vertices.create(size);

    for (int i = 0; i < size.height; ++i)
    {
        int n = gridIndex(i, size.height, wrap[1]);

        for (int j = 0; j < size.width; ++j)
        {
            int m = gridIndex(j, size.width, wrap[0]);
            auto radius = radiusMap(n, m);
            vertices(i,j) = radius * polarAngles(i, j);
        }
    }

    return vertices;
}

