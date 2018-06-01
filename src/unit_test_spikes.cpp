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

#include "yaffut.hpp"
#include "EquirectangularProjection.hpp"
#include "PerspectiveProjector.hpp"
#include "PoseTraces.hpp"
#include "Pipeline.hpp"
#include "image_loading.hpp"
#include "Parser.hpp"
#include "SynthetizedView.hpp"
#include "transform.hpp"


#include "opencv2/opencv.hpp"

#include <iostream>

using namespace std;
using namespace cv;


namespace 
{
    // for testing if two angles are the same. resolves modulo 2pi
    double DistanceOnUnitCircle( float a, float b)
    {
        return cv::norm( cv::Vec2f( std::sin(a), std::cos(a) ) - cv::Vec2f(std::sin(b), std::cos(b) ) );
    }

    Vec3d DistanceOnUnitCircle( cv::Vec3f a, cv::Vec3f b)
    {
        return cv::Vec3d(
            DistanceOnUnitCircle(a[0], b[0] ),
            DistanceOnUnitCircle(a[1], b[1] ),
            DistanceOnUnitCircle(a[2], b[2] ) );
        
    }


} // namespace

cv::Mat ScaleDown(cv::Mat im, double scale)
{
    cv::Mat imScaled;
    cv::resize(im, imScaled, cv::Size(), scale, scale, cv::INTER_AREA);
    return imScaled;
}




FUNC(Spike_ReadImageAndDepth)
{
    const std::string nameImg   = "./Plane_B'/Plane_B'_Texture/Kinect_z0300y0307x0370.yuv";
    const std::string nameDepth = "./Plane_B'/Plane_B'_Depth/Kinect_z0300y0307x0370.yuv";
    const int bit_depth_color = 8;
    const int bit_depth_depth = 16;

    cv::Size size = cv::Size(1920,1080);
    color_space = COLORSPACE_RGB;
    cv::Mat im = read_color(nameImg, size, bit_depth_color, 0);
    cv::Mat1b mask_depth = cv::Mat1b::ones(size) * 255;

    float z_near  = 500;
    float z_far   = 2000;
    cv::Mat depth = read_depth(nameDepth, size, bit_depth_depth, z_near, z_far, 0);

    depth.convertTo( depth, -1, 1.0 / 2000);

    cv::imshow( "img",   ScaleDown(im, 0.5)    );
    cv::imshow( "depth", ScaleDown(depth, 0.5) );
    cv::waitKey(0);

}



struct ErpReaderRaw
{
    std::string nameImgT   = "./ClassRoomVideo/v%d_4096_2048_420_10b.yuv";
    std::string nameDepthT = "./ClassRoomVideo/v%d_4096_2048_0_8_1000_0_420_10b.yuv";
    
    cv::Size size = cv::Size(4096,2048);
    int bit_depth = 10;
    float z_near = 0.8f;
    float z_far  = 1e6f;

    void read( int num )
    {
        image3f  = read_color(cv::format(nameImgT.c_str(), num), size, bit_depth, 0);
        imRadius = read_depth(cv::format(nameDepthT.c_str(), num), size, bit_depth, z_near, z_far, 0);

        CV_Assert( !image3f.empty() && !imRadius.empty() );

        image3f.convertTo( image, image.type(), 255.0 );
        imRadius.convertTo( imRadius8u, imRadius8u.type(), -255.0/ 10.0, 255.0 );
        size = imRadius.size();
    }

    cv::Mat3b image; 
    cv::Mat3f image3f;
    cv::Mat1f imRadius;
    cv::Mat1b imRadius8u;
};



FUNC( Spike_ViewSynthesisFromErpRaw )
{
    ::ErpReaderRaw erpzV0;
    erpzV0.read(0);
    
    auto size = erpzV0.size;

    ::ErpReaderRaw erpzV1;
    erpzV1.read(1);

    cv::Mat3b imBGR;
    cv::cvtColor(erpzV0.image, imBGR, cv::COLOR_YUV2BGR  );
    cv::imshow( "im",    ScaleDown(imBGR, 0.25)    );
    cv::imshow( "depth", ScaleDown(erpzV0.imRadius8u, 0.25)    );
    cv::waitKey(200);


	Parameters const parameters;
    erp::Unprojector unprojector(parameters, size);
    unprojector.create(size);
    auto verticesXyz = unprojector.unproject(erpzV0.imRadius);

    //const auto translation = cv::Vec3f(-0.0519615f, 0.03f, 0.f );
    const auto translation = cv::Vec3f(0.0519615f, -0.03f, 0.f );
    cv::Mat3f verticesXyzNew = verticesXyz + translation;

    rescale = 1.f;
    cv::Size sizeOut = size;

    erp::Projector projector(parameters, sizeOut);
    cv::Mat1f imRadiusNew;
    WrappingMethod wrapping_method;
    cv::Mat2f imUVnew     = projector.project( verticesXyzNew, imRadiusNew, wrapping_method );
    CHECK(wrapping_method == WrappingMethod::HORIZONTAL);

    bool wrapHorizontal = true;

    cv::Mat1f depth, quality;
    cv::Mat3f imResult3f = transform_trianglesMethod(erpzV0.image3f, imRadiusNew, imUVnew, sizeOut, depth, quality, wrapHorizontal);


    cv::Mat3b imResult;
    imResult3f.convertTo(imResult, imResult.type(), 255.0 );
    cv::Mat3b imResultBGR;
    cv::cvtColor(imResult, imResultBGR, cv::COLOR_YUV2BGR  );
    cv::imshow( "imResultBGR",   ScaleDown(imResultBGR, 0.25)    );


    cv::Mat3b imBGR_V1;
    cv::cvtColor(erpzV1.image, imBGR_V1, cv::COLOR_YUV2BGR  );


    cv::Mat3b imDiff;
    cv::absdiff(imResultBGR, imBGR_V1, imDiff );
    cv::imshow( "imDiff",   ScaleDown(imDiff, 0.25)    );

    cv::Mat3b imDiffRef;
    cv::absdiff(imBGR, imBGR_V1, imDiffRef );
    cv::imshow( "imDiffRef",   ScaleDown(imDiffRef, 0.25)    );


    cv::waitKey(0);

}

FUNC( Spike_ViewSynthesisErpToPerspective )
{
    ::ErpReaderRaw erpzV0;
    erpzV0.read(0);

    auto size = erpzV0.size;

    auto on_image = [size](const cv::Vec2f& uv)->bool
    {
        return uv[0] >= 0.f && uv[0] <= float(size.width) && uv[1] >= 0.f && uv[1] <= float(size.height);
    };



    cv::Mat3b imBGR;
    cv::cvtColor(erpzV0.image, imBGR, cv::COLOR_YUV2BGR  );
    cv::imshow( "im",    ScaleDown(imBGR, 0.25)    );
    cv::imshow( "depth", ScaleDown(erpzV0.imRadius8u, 0.25)    );
    cv::waitKey(200);


    erp::Unprojector unprojector;
    unprojector.create(size);
    auto verticesXyz = unprojector.unproject(erpzV0.imRadius);

    cv::Mat3f verticesXyzNew = verticesXyz.clone();

    //for( auto& v : verticesXyzNew )    v[0] = -v[0];

    rescale = 1.f;
    cv::Size sizeOut = size;

    float w   = 4096.f;
    float h   = 2048.f;
    float f   = w/2;
    float p_x = w/2;
    float p_y = h/2;
    
    auto const camera_matrix = cv::Matx33f(
        f , 0.f, p_x,
        0.f,  f , p_y,
        0.f, 0.f, 1.f);
    
    Parameters parameters( cv::Matx33f::eye(), cv::Vec3f::all(0), camera_matrix, w, CoordinateSystem::MPEG_I_OMAF );

    PerspectiveProjector projector(parameters);

    cv::Mat1f depthNew;
    WrappingMethod wrappingMethod;
    cv::Mat2f imUVnew = projector.project( verticesXyzNew, depthNew, wrappingMethod );
    

    cv::Mat1b validMask = depthNew > 0.f;

    cv::imshow( "validMask",   ScaleDown(validMask, 0.25)  );
    cv::waitKey(100);

    //auto roi = cv::Rect(2000,1000,5,5);
    //cout << verticesXyzNew(roi) << endl << endl;
    //cout << imUVnew(roi)        << endl << endl;
    //cout << depthNew(roi)       << endl << endl;

    bool wrapHorizontal = false;
    cv::Mat1f depth, quality;
    cv::Mat3f imResult3f = transform_trianglesMethod(erpzV0.image3f, depthNew, imUVnew, sizeOut, depth, quality, wrapHorizontal);

    cv::Mat3b imResult;
    imResult3f.convertTo(imResult, imResult.type(), 255.0 );
    cv::Mat3b imResultBGR;
    cv::cvtColor(imResult, imResultBGR, cv::COLOR_YUV2BGR  );
    cv::imshow( "imResultBGR",   ScaleDown(imResultBGR, 0.25)    );


    cout << endl; 
    cout << "Done" << endl;
    
    cv::waitKey(0);

}


FUNC(Spike_ClassRoomVideo )
{
    //Pipeline p("./ClassroomVideo/ClassroomVideo-SVS-v7v8_to_i0.cfg");
    Pipeline p("./ClassroomVideo/ClassroomVideo-SVS-v0_to_p0.cfg");

    p.execute();
}





FUNC( SpikePoseTraces )
{
    using namespace pose_traces;
    
    cout << endl;

    std::string name      = "./PoseTraces/tracker_data.rt.csv";
    std::string nameEuler = "./PoseTraces/tracker_data.euler.csv";
    std::string nameEuler2 = "./PoseTraces/tracker_data.euler2.csv";

    //auto poseTrace = ReadPoseTrace( name );
    //WritePoseTrace( nameEuler, poseTrace, true );

    auto poseTrace2 = ReadPoseTrace( nameEuler );
    WritePoseTrace( nameEuler2, poseTrace2, true );

    //auto pose = poseTrace[1];
    //cout << pose.rotation << endl << endl;
    //cout << pose.translation << endl;
    //cout << pose.ToCsv(false) << endl;



    
    
    


}