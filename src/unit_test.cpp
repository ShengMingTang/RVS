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
#include "PerspectiveProjector.hpp"
#include "PoseTraces.hpp"

#include <opencv2/opencv.hpp>

#include <iostream>

using namespace std;

namespace testing
{
    // for testing if two angles are the same. resolves modulo 2pi
    double DistanceOnUnitCircle( float a, float b)
    {
        return cv::norm( cv::Vec2f( std::sin(a), std::cos(a) ) - cv::Vec2f(std::sin(b), std::cos(b) ) );
    }

    cv::Vec3d DistanceOnUnitCircle( cv::Vec3f a, cv::Vec3f b)
    {
        return cv::Vec3d(
            DistanceOnUnitCircle(a[0], b[0] ),
            DistanceOnUnitCircle(a[1], b[1] ),
            DistanceOnUnitCircle(a[2], b[2] ) );

    }


} // namespace


FUNC( TestERP_ConvertImageCoordinateToFromPhiTheta )
{
    const double eps = 1e-6;
    
    int N = 10;
    for( int j = 0; j < N; ++j )
    {
        float hPosExpected = j + 0.5f;
        float phi = erp::calculate_phi( hPosExpected, N);

        float hPosActual = erp::calculate_horizontal_image_coordinate(phi, N);
        ALMOST( hPosExpected, hPosActual, eps);
    }

    for( int i = 0; i < N; ++i )
    {
        float vPosExpected = i + 0.5f;
        float theta = erp::calculate_theta( vPosExpected, N);

        float vPosActual = erp::calculate_vertical_image_coordinate(theta, N);
        ALMOST( vPosExpected, vPosActual, eps);
    }


}

FUNC( TestERP_CoordinateTransform )
{
    const double eps = 1e-6;
    const int N = 10;
    
    // exclude poles: 0,N
    for( int i =1; i< N; ++i )      
        for(int j = 0; j<N; ++j )
        {
            float vPos = i + 0.5f;
            float hPos = j + 0.5f;

            float theta = erp::calculate_theta( vPos, N);
            float phi   = erp::calculate_phi( hPos, N);

            auto sphericalExpected = cv::Vec2f(phi, theta);

            auto xyzNorm           = erp::calculate_euclidian_coordinates( sphericalExpected );
            auto sphericalActual   = erp::calculate_sperical_coordinates( xyzNorm);

            auto err0 = testing::DistanceOnUnitCircle( sphericalExpected[0] , sphericalActual[0] );
            auto err1 = testing::DistanceOnUnitCircle( sphericalExpected[1] , sphericalActual[1] );

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
    cv::Size size(30,30);
    
    erp::Unprojector unprojector;
    unprojector.create(size);
    
    cv::Mat1f radiusMap = cv::Mat1f::ones(size);

    auto vertices = unprojector.unproject(radiusMap);

    const double radiusExpected = 1.0;

    for( auto v : vertices )
        ALMOST( radiusExpected,  cv::norm(v) , eps );

}



FUNC( TestERP_Project)
{
    double eps = 1e-7;
    //const float rescale = 1.f;
    rescale = 1.f;

    const cv::Size size(5, 5);
    
    erp::Unprojector unprojector;
    unprojector.create(size);
    
    cv::Mat1f imRadius = cv::Mat1f::ones(size);

    auto imXYZ = unprojector.unproject(imRadius);

    float radiusExpected = 2.f;
    
    cv::Mat3f imXYZnew = imXYZ * radiusExpected;

    erp::Projector projector;
    cv::Mat1f imRadiusActual;
	WrappingMethod wrapping_method;
    cv::Mat2f imUV = projector.project( imXYZnew, imRadiusActual, wrapping_method);
	CHECK(wrapping_method == WrappingMethod::NONE);


    eps *= size.area();
    double errorRadius = cv::sum( cv::abs( imRadiusActual  - radiusExpected ) ).val[0];
    
    ALMOST( 0.0, errorRadius, eps );

    //auto errorPhiTheta = cv::sum( cv::abs( unprojector.phiTheta - projector.imPhiTheta ) );
    //ALMOST( 0.0, errorPhiTheta[0], eps );
    //ALMOST( 0.0, errorPhiTheta[1], eps );

}

FUNC(Test_PerspectiveProjector)
{
	// Aribitrary extrinsics
	auto const R = cv::Matx33f::randn(3, 3);
	auto const t = cv::Vec3f(5.f, 3.f, 9.f);

	// Example values
	auto const w = 4.f;
	auto const h = 2.f;
	auto const f = 6.f;
	auto const p_x = 1.f; // assymmetric
	auto const p_y = 1.f;
	auto const far_away = 42.f;

	// Intrinsics
	auto const M = cv::Matx33f(
		 f , 0.f, p_x,
		0.f,  f , p_y,
		0.f, 0.f, 1.f);

	// Intrinsic and extrinsic camera parameters
	auto const parameters = Parameters(R, t, M, w, CoordinateSystem::MPEG_I_OMAF);

	// Construct projector
	PerspectiveProjector projector(parameters);
	
	// Check that extrinsics can be obtained
	EQUAL(R, projector.get_rotation());
	EQUAL(t, projector.get_translation());

	// Example world positions
	// OMAF Referential: x forward, y left, z up
	cv::Mat3f world_pos(1, 4);
	world_pos(0, 0) = cv::Vec3f(far_away, 0.f, 0.f); // straight ahead, far away
	world_pos(0, 1) = cv::Vec3f(f, 0.f, 0.f); // principle point
	world_pos(0, 2) = cv::Vec3f(f, p_x, p_y); // top-left corner of sensor
	world_pos(0, 3) = cv::Vec3f(f, p_x - w, p_y - h); // botom-right corner of sensor

	// Reference image positions and depth values
	cv::Mat2f reference_image_pos(1, 4);
	reference_image_pos(0, 0) = cv::Vec2f(p_x, p_y);
	reference_image_pos(0, 1) = cv::Vec2f(p_x, p_y);
	reference_image_pos(0, 2) = cv::Vec2f(0.f, 0.f);
	reference_image_pos(0, 3) = cv::Vec2f(w, h);
	cv::Mat1f reference_depth(1, 4);
	reference_depth(0, 0) = far_away;
	reference_depth(0, 1) = f;
	reference_depth(0, 2) = f;
	reference_depth(0, 3) = f;

	// Project
	cv::Mat1f actual_depth;
	WrappingMethod wrapping_method;
	auto actual_image_pos = projector.project(world_pos, actual_depth, wrapping_method);
	CHECK(wrapping_method == WrappingMethod::NONE);

	// Check projections results against reference
	auto image_pos_error = norm(reference_image_pos, actual_image_pos);
	auto depth_error = norm(reference_depth, actual_depth);

	ALMOST(image_pos_error, 0, 1e-12);
	ALMOST(depth_error, 0, 1e-12);
}



FUNC( TestRotationMatrixToFromEulerAngles )
{
    using namespace pose_traces::detail;
    
    const double eps = 1e-7;

    int N = 16;
    
    for( int i0 = 0; i0 <= N; ++i0 )                // closed interval
        for( int i1 = 1; i1 <  N; ++i1 )            // open interval
            for( int i2 = 0; i2 <= N; ++i2 )        // closed interval
            {

                float yaw   = float( i0 * CV_2PI / N - CV_PI  );
                float pitch = float( i1 * CV_PI  / N - CV_PI/2);
                float roll  = float( i2 * CV_2PI / N - CV_PI  );

                auto eulerExpected  = cv::Vec3f(yaw, pitch, roll );
                auto R              = EulerAnglesToRotationMatrix( eulerExpected );
                auto eulerActual    = RotationMatrixToEulerAngles(R);

                double err = cv::norm( testing::DistanceOnUnitCircle(eulerExpected, eulerActual) );
                if( err > eps )
                {
                    cout << i0 << " "<< i1 << " "<< i2 << " " << err << endl ;
                    CHECK(false);
                }

            }


}
