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

#define YAFFUT_MAIN
#include "yaffut.hpp"

#include "EquirectangularProjection.hpp"
#include "PerspectiveProjector.hpp"
#include "PoseTraces.hpp"
#include "JsonParser.hpp"

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
        CHECK(std::abs(hPosExpected - hPosActual) < eps);
    }

    for( int i = 0; i < N; ++i )
    {
        float vPosExpected = i + 0.5f;
        float theta = erp::calculate_theta( vPosExpected, N);

        float vPosActual = erp::calculate_vertical_image_coordinate(theta, N);
		CHECK(std::abs(vPosExpected - vPosActual) < eps);
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
            auto sphericalActual   = erp::calculate_spherical_coordinates( xyzNorm);

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

	std::istringstream text(R"({
			"Name": "v0",
			"Projection": "Equirectangular",
			"Position": [0, 0, 0],
			"Rotation": [0, 0, 0],
			"Depthmap": 1,
			"Background": 0,
			"Depth_range": [0, 1],
			"Resolution": [30, 30],
			"BitDepthColor": 13,
			"BitDepthDepth": 15,
			"ColorSpace": "YUV420",
			"DepthColorSpace": "YUV420",
			"Hor_range": [-180, 180],
			"Ver_range": [-90, 90]
	})");

	auto root = json::Node::readFrom(text);
	auto parameters = Parameters::readFrom(root);

    erp::Unprojector unprojector(parameters);
    
    cv::Mat1f radiusMap = cv::Mat1f::ones(parameters.getSize());

    auto vertices = unprojector.unproject(radiusMap);

    const double radiusExpected = 1.0;

	for (auto v : vertices) {
		CHECK(std::abs(radiusExpected - cv::norm(v)) < eps);
	}
}



FUNC( TestERP_Project)
{
    double eps = 1e-7;
    g_rescale = 1.f;

	std::istringstream text(R"({
			"Name": "v0",
			"Projection": "Equirectangular",
			"Position": [0, 0, 0],
			"Rotation": [0, 0, 0],
			"Depthmap": 1,
			"Background": 0,
			"Depth_range": [0, 1],
			"Resolution": [5, 5],
			"BitDepthColor": 13,
			"BitDepthDepth": 15,
			"ColorSpace": "YUV420",
			"DepthColorSpace": "YUV420",
			"Hor_range": [-180, 180],
			"Ver_range": [-90, 90]
	})");

	auto root = json::Node::readFrom(text);
	auto parameters = Parameters::readFrom(root);

    erp::Unprojector unprojector(parameters);
    
    cv::Mat1f imRadius = cv::Mat1f::ones(parameters.getSize());

    auto imXYZ = unprojector.unproject(imRadius);

    float radiusExpected = 2.f;
    
    cv::Mat3f imXYZnew = imXYZ * radiusExpected;

	erp::Projector projector(parameters);
    cv::Mat1f imRadiusActual;
	WrappingMethod wrapping_method;
    cv::Mat2f imUV = projector.project( imXYZnew, imRadiusActual, wrapping_method);
	CHECK(wrapping_method == WrappingMethod::none);


    eps *= parameters.getSize().area();
    double errorRadius = cv::sum( cv::abs( imRadiusActual  - radiusExpected ) ).val[0];
    
    CHECK(errorRadius < eps);
}

FUNC(Test_PerspectiveProjector)
{
	std::istringstream text(R"({
			"Name"				: "v0",
			"Projection"		: "Perspective",
			"Position"			: [5, 3, 9],
			"Rotation"			: [10, 5, -3],
			"Depthmap"			: 1,
			"Background"		: 0,
			"Depth_range"		: [0, 1],
			"Resolution"		: [4, 2],
			"Focal"				: [6, 6],
			"Principle_point"	: [1, 1],
			"BitDepthColor"		: 10,
			"BitDepthDepth"		: 10,
			"ColorSpace"		: "YUV420",
			"DepthColorSpace"	: "YUV420"
	})");

	auto root = json::Node::readFrom(text);
	auto parameters = Parameters::readFrom(root);

	// Construct projector
	PerspectiveProjector projector(parameters);
	
	// Example world positions
	// OMAF Referential: x forward, y left, z up
	cv::Mat3f world_pos(1, 4);
	world_pos(0, 0) = cv::Vec3f(42.f, 0.f, 0.f); // straight ahead, far away
	world_pos(0, 1) = cv::Vec3f(6.f, 0.f, 0.f); // principle point on sensor
	world_pos(0, 2) = cv::Vec3f(6.f, 1.f, 1.f); // top-left corner of sensor
	world_pos(0, 3) = cv::Vec3f(6.f, -3.f, -1.f); // botom-right corner of sensor

	// Reference image positions and depth values
	cv::Mat2f reference_image_pos(1, 4);
	reference_image_pos(0, 0) = cv::Vec2f(1.f, 1.f); // p
	reference_image_pos(0, 1) = cv::Vec2f(1.f, 1.f); // p
	reference_image_pos(0, 2) = cv::Vec2f(0.f, 0.f);
	reference_image_pos(0, 3) = cv::Vec2f(4.f, 2.f); // (w, h)
	cv::Mat1f reference_depth(1, 4);
	reference_depth(0, 0) = 42.f;
	reference_depth(0, 1) = 6.f;
	reference_depth(0, 2) = 6.f;
	reference_depth(0, 3) = 6.f;

	// Project
	cv::Mat1f actual_depth;
	WrappingMethod wrapping_method;
	auto actual_image_pos = projector.project(world_pos, actual_depth, wrapping_method);
	CHECK(wrapping_method == WrappingMethod::none);

	// Check projections results against reference
	auto image_pos_error = norm(reference_image_pos, actual_image_pos);
	auto depth_error = norm(reference_depth, actual_depth);

	CHECK(image_pos_error < 1e-12);
	CHECK(depth_error < 1e-12);
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

FUNC(TestJsonParser)
{
	std::istringstream stream(R"(
{
	"Content_name": "ClassroomVideo",
	"BoundingBox_center" : [0.0, 42.0, -1e3],
	"Fps" : 30,
	"cameras" :
	[
		{
			"Name": "v0",
			"Background" : 0
		},
		{
		}
	]
})");
	
	auto root = json::Node::readFrom(stream);
	EQUAL(root.optional("Content_name").asString(), "ClassroomVideo");	
	auto center = root.optional("BoundingBox_center");
	EQUAL(center.size(), 3);
	EQUAL(center.at(0).asDouble(), 0.0);
	EQUAL(center.at(1).asDouble(), 42.0);
	EQUAL(center.at(2).asDouble(), -1e3);
	EQUAL(root.optional("Fps").asDouble(), 30.0);
	auto cameras = root.optional("cameras");
	EQUAL(cameras.size(), 2);
	auto cam0 = cameras.at(0);
	EQUAL(cam0.optional("Name").asString(), "v0");
	EQUAL(cam0.optional("Background").asDouble(), 0.0);
	auto cam1 = cameras.at(1);
}