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

    cv::Size size = cv::Size(1920,1080);
    color_space = COLORSPACE_RGB;
    cv::Mat im = read_color(nameImg, size);


    float z_near  = 500;
    float z_far   = 2000;
    cv::Mat depth = read_depth(nameDepth, size, z_near, z_far );

    depth.convertTo( depth, -1, 1.0 / 2000);

    cv::imshow( "img",   ScaleDown(im, 0.5)    );
    cv::imshow( "depth", ScaleDown(depth, 0.5) );
    cv::waitKey(0);

}


FUNC(Spike_ViewSynthSingle)
{
    cout << endl;

    Parser parser("./config_files/example_config_file.cfg");

    auto config = parser.get_config();

    //config.params_virtual[0]

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

        View image = View(nameImg, nameDepth, params, config.size);
        //if (config.znear.size() > 0) image.set_z(config.znear[idx], config.zfar[idx]);

        image.load();
        image.preprocess_depth();

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

