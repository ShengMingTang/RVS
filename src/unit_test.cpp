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

#define YAFFUT_MAIN
#include "yaffut.hpp"

#include "EquirectangularProjection.hpp"

#include "opencv2/opencv.hpp"

#include <iostream>

using namespace std;


double DistanceOnUnitCircle( float a, float b)
{
    return cv::norm( cv::Vec2f( std::sin(a), std::cos(a) ) - cv::Vec2f(std::sin(b), std::cos(b) ) );
}


FUNC( TestERP_CoordinateTransform )
{
    const double eps = 1e-6;
    const int N = 10;
    
    // exclude poles
    for( int i =1; i< N; ++i )      
        for(int j = 0; j<N; ++j )
        {
            float vPos = i + 0.5f;
            float hPos = j + 0.5f;

            float theta = erp::CalcTheta( vPos, N);
            float phi   = erp::CalcPhi( hPos, N);

            auto sphericalExpected = cv::Vec2f(phi, theta);

            auto xyzNorm           = erp::CalcEuclidanCoordinates( sphericalExpected );
            auto sphericalActual   = erp::CalcSphereCoordinates( xyzNorm);

            auto err0 = DistanceOnUnitCircle( sphericalExpected[0] , sphericalActual[0] );
            auto err1 = DistanceOnUnitCircle( sphericalExpected[1] , sphericalActual[1] );

            if( err0 > eps || err1 > eps )
            {
                cout << i << " " << j << endl;
                cout << sphericalExpected << endl;
                cout << sphericalActual << endl;
                cout << endl;
                CHECK(false);
            }
        }

}

FUNC( TestERP_BackProject)
{
    const double eps = 1e-7;
    
    erp::BackProjector erpMesh;

    cv::Size size(30,30);
    cv::Mat1f radiusMap = cv::Mat1f::ones(size);

    auto vertices = erpMesh.CalculateVertices(radiusMap);

    const double radiusExpected = 1.0;

    for( auto v : vertices )
        ALMOST( radiusExpected,  cv::norm(v) , eps );

}



FUNC( TestERP_Project_Sanity)
{
    double eps = 1e-7;

    erp::BackProjector backProjector;

    cv::Size size(5, 5);
    cv::Mat1f imRadius = cv::Mat1f::ones(size);

    auto imXYZ = backProjector.CalculateVertices(imRadius);

    //const auto translation = cv::Vec3f(0.1f, 0.f, 0.f );
    //cv::Mat3f imXYZt = imXYZ + translation;

    float radiusExpected = 2.f;
    cv::Mat3f imXYZnew = imXYZ * radiusExpected;

    erp::Projector projector;
    cv::Mat2f imUV = projector.ProjectToImageCoordinatesUV( imXYZnew);

    eps *= size.area();
    double errorRadius = cv::sum( cv::abs( projector.imRadius  - radiusExpected ) ).val[0];
    ALMOST( 0.0, errorRadius, eps );

    auto errorPhiTheta = cv::sum( cv::abs( backProjector.phiTheta - projector.imPhiTheta ) );
    ALMOST( 0.0, errorPhiTheta[0], eps );
    ALMOST( 0.0, errorPhiTheta[1], eps );

}


