/* ------------------------------------------------------------------------------ -

Copyright � 2018 Koninklijke Philips N.V.

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

#include "Pipeline.hpp"
#include "image_loading.hpp"

#include "Parser.hpp"
#include "SynthetizedView.hpp"

#include "opencv2/opencv.hpp"

#include <iostream>

using namespace std;


cv::Mat ScaleDown(cv::Mat im, double scale)
{
    cv::Mat imScaled;
    cv::resize(im, imScaled, cv::Size(), scale, scale, cv::INTER_AREA);
    return imScaled;
}




FUNC(Spike_Unicorn_Example)
{
    Pipeline p("./config_files/example_config_file.cfg");
    p.execute();
}


FUNC(Spike_ReadImageAndDepth)
{
    const std::string nameImg   = "./Plane_B'/Plane_B'_Texture/Kinect_z0300y0307x0370.yuv";
    const std::string nameDepth = "./Plane_B'/Plane_B'_Depth/Kinect_z0300y0307x0370.yuv";
    const int bit_depth_color = 8;
    const int bit_depth_depth = 16;

    cv::Size size = cv::Size(1920,1080);
    color_space = COLORSPACE_RGB;
    cv::Mat im = read_color(nameImg, size, bit_depth_color);
    cv::Mat1b mask_depth = cv::Mat1b::ones(size) * 255;

    float z_near  = 500;
    float z_far   = 2000;
    cv::Mat depth = read_depth(nameDepth, size, bit_depth_depth, z_near, z_far, mask_depth );

    depth.convertTo( depth, -1, 1.0 / 2000);

    cv::imshow( "img",   ScaleDown(im, 0.5)    );
    cv::imshow( "depth", ScaleDown(depth, 0.5) );
    cv::waitKey(0);

}


FUNC(Spike_ViewSynthSingle)
{
    const int bit_depth_color = 8;
    const int bit_depth_depth = 16;
    
    cout << endl;

    Parser parser("./config_files/example_config_file.cfg");

    auto config = parser.get_config();

    std::vector<View> input_images;

    int numInputs = int( config.params_real.size() );
    cout << "numInputs " << numInputs << endl;

    for( int idx = 0; idx <numInputs; ++idx)
    {
        auto nameImg   = config.texture_names[idx];
        auto nameDepth = config.depth_names[idx];
        auto& params    = config.params_real[idx];

        cout << nameImg << endl;
        cout << nameDepth << endl;
        //cout << params.camera_matrix << endl;

        View image = View(nameImg, nameDepth, params, config.size, bit_depth_color, bit_depth_depth);
        //if (config.znear.size() > 0) image.set_z(config.znear[idx], config.zfar[idx]);

        image.load();

        SynthetizedViewTriangle renderer( config.params_virtual[0], image.get_size() );

        renderer.compute(image);
        cv::Mat imOut = renderer.get_color();
        cv::Mat imDepth = renderer.get_depth();
        imDepth.convertTo( imDepth, -1, 1.0 / 2000);

        cv::imshow( cv::format("img%d",idx),   ScaleDown(imOut, 0.25)    );
        cv::imshow( cv::format("depth%d",idx), ScaleDown(imDepth, 0.25)    );

        cv::waitKey(1);
        cout << endl;
    }

    cv::waitKey(0);
}

cv::Mat imreadThrow( const std::string& filename, int flags = cv::IMREAD_COLOR )
{
    auto im = cv::imread(filename, flags );
    if( im.empty() )
        throw std::runtime_error( "Can't read " + filename );
    return im;
}

template< class T, int cn>
auto  SelectChannel(const cv::Mat_<cv::Vec<T,cn>>& imN, int ch) -> cv::Mat_<cv::Vec<T,1>>
{
    cv::Mat_<cv::Vec<T,1>> im1( imN.size() ) ;
    auto vnIt = imN.begin();
    for (auto& v : im1)
    {
        const auto& vn = *vnIt;
        v = vn[ch];
        vnIt++;
    }
    return im1;
}

template <class T>
inline const T& Clip(const T& val, const T& minVal, const T& maxVal)
{
    return std::min(maxVal, std::max(minVal, val));
}

uchar RadiusTo8u(float radius, float minimumCodingDistance , float maximumCodingDistance)
{

    float radiusBounded = Clip( radius, minimumCodingDistance, maximumCodingDistance);
    auto disparity = minimumCodingDistance / radiusBounded;
    uchar depth = cv::saturate_cast<uchar>(255.0 * disparity);

    return depth;
}

cv::Mat1b EncodeRadiusTo8u( cv::Mat1f imRadius, float minimumCodingDistance = 0.25f, float maximumCodingDistance = 40.f )
{
    cv::Mat1b depthEnc( imRadius.size() );

    auto radius = imRadius.begin();
    for( auto& depth : depthEnc )
        depth = ::RadiusTo8u(*radius++, minimumCodingDistance, maximumCodingDistance );

    return depthEnc;
}


#include "transform.hpp"

void colorize_triangle(const cv::Mat & img, const cv::Mat & depth, const cv::Mat& depth_prologation_mask, const cv::Mat & new_pos, cv::Mat& res, cv::Mat& depth_inv, cv::Mat& new_depth_prologation_mask, cv::Mat& triangle_shape, cv::Vec2f a, cv::Vec2f b, cv::Vec2f c);

struct ErpViewSynth
{

    cv::Mat new_pos;

cv::Mat translateBigger_trianglesMethod_Erp(const cv::Mat& img, const cv::Mat& depth, const cv::Mat& depth_prologation_mask, const cv::Mat& R, const Translation & t, const cv::Mat & old_cam_mat, const cv::Mat & new_cam_mat, float sensor, cv::Mat& depth_inv, cv::Mat& new_depth_prologation_mask, cv::Mat& triangle_shape) 
{
    cv::Size s((int)(rescale*(float)depth.size().width), (int)(rescale*(float)depth.size().height));

    depth_inv = cv::Mat::zeros(s, CV_32F);
    new_depth_prologation_mask = cv::Mat::ones(s, depth_prologation_mask.type());
    triangle_shape = cv::Mat::zeros(s, CV_32F);
    cv::Mat res = cv::Mat::zeros(s, CV_32FC3);

    sensor;
    new_cam_mat;
    old_cam_mat;
    t;
    R;



    //compute new position
    //cv::Mat new_pos; //= translation_map(depth, t, old_cam_mat, new_cam_mat, sensor, 0.5);
    //new_pos = rotation_map(R, new_pos, new_cam_mat, sensor);

    //compute triangulation
    for (int x = 0; x < img.cols - 1; ++x)
        for (int y = 0; y < img.rows - 1; ++y) {
            if (depth.at<float>(y, x + 1) > 0.0 && depth.at<float>(y + 1, x) > 0.0 && new_pos.at<cv::Vec2f>(y, x + 1)[0] > 0.0 && new_pos.at<cv::Vec2f>(y + 1, x)[0] > 0.0) {
                if (depth.at<float>(y, x) > 0.0 && new_pos.at<cv::Vec2f>(y, x)[0] > 0.0)
                    colorize_triangle(img, depth, depth_prologation_mask, new_pos, res, depth_inv, new_depth_prologation_mask, triangle_shape, cv::Vec2f((float)x, (float)y), cv::Vec2f((float)x + 1.0f, (float)y), cv::Vec2f((float)x, (float)y + 1.0f));
                if (depth.at<float>(y + 1, x + 1) > 0.0 && new_pos.at<cv::Vec2f>(y + 1, x + 1)[0] > 0.0)
                    colorize_triangle(img, depth, depth_prologation_mask, new_pos, res, depth_inv, new_depth_prologation_mask, triangle_shape, cv::Vec2f((float)x + 1.0f, (float)y + 1.0f), cv::Vec2f((float)x, (float)y + 1.0f), cv::Vec2f((float)x + 1.0f, (float)y));
            }
        }
    return res;
}


};


FUNC( Spike_ReadImageAndDepthErp )
{
    const std::string nameImg   = "./classroom/img0001.png";
    const std::string nameDepth = "./classroom/depth0001.exr";

    cv::Mat3b im             = ::imreadThrow( nameImg );

    cv::Mat3f imRadius3f     = ::imreadThrow( nameDepth, cv::IMREAD_UNCHANGED);
    cv::Mat1f imRadius       = ::SelectChannel( imRadius3f, 0 );

    auto depth               = ::EncodeRadiusTo8u( imRadius );
    cv::imshow( "img",   ScaleDown(im, 0.5)    );
    cv::imshow( "depth", ScaleDown(depth, 0.5) );
    cv::waitKey(0);

    erp::BackProjector backProjector;
    auto verticesXyz = backProjector.CalculateVertices(imRadius);

    const auto translation = cv::Vec3f(0.1f, 0.f, 0.f );
    //cv::Mat3f imXYZt = imXYZ + translation;

    cv::Mat3b verticesXyzNew = verticesXyz + translation;

    erp::Projector projector;
    cv::Mat2f imUVnew = projector.ProjectToImageCoordinatesUV( verticesXyzNew );
    cv::Mat1f imRadiusNew = projector.imRadius;

    //ErpViewSynth viewSynth;
    


    // find new coordinates




}







