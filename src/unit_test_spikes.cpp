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

FUNC( SpikeCreateCenterViewsForPoseTraces )
{

    auto configs = { 
        "./PoseTraces/config_files/TechnicoloMuseum-Center_360.cfg",
        "./PoseTraces/config_files/TechnicoloMuseum-Center_360_depth.cfg"  /*,
        "./PoseTraces/config_files/TechnicolorHijack-Center_360.cfg",
        "./PoseTraces/config_files/TechnicolorHijack-Center_360_depth.cfg" */   };
    
    for ( auto cfg : configs )
    {
        Pipeline p(cfg);
        p.execute();
    }
}

FUNC( SpikeCreatePerspective )
{


    std::string cfg = "./PoseTraces/config_files/TechnicoloMuseum-Center_360_perspective.cfg";

    Pipeline p(cfg);
    p.execute();
}

FUNC( SpikeCreatePoseTracesViews )
{

    
    std::string cfg = "./PoseTraces/config_files/TechnicoloMuseum-Center_360_posetrace.cfg";

    Pipeline p(cfg);
    p.execute();
}


FUNC( SpikeConvertPoseTraceToEuler )
{
    using namespace pose_traces;

    cout << endl;

    auto names = {  "poses_classroom_0.csv",
                    "poses_classroom_1.csv",
                    "poses_classroom_2.csv",
                    "poses_hijack_0.csv",
                    "poses_hijack_1.csv",
                    "poses_hijack_2.csv",
                    "poses_museum_0.csv",
                    "poses_museum_1.csv",
                    "poses_museum_2.csv"      };

    for( string name : names )
    {
        std::string fname      = "./PoseTraces/config_files/" + name ;
        std::string fnameEuler = "./PoseTraces/config_files/euler/" + name ;

        auto poseTrace = ReadPoseTrace( fname );

        // make poses relative with respect to first frame
        auto R0 = poseTrace[0].rotation;
        auto t0 = poseTrace[0].translation;
        for( auto& pose : poseTrace )
        {
            pose.rotation = R0.t() * pose.rotation;
            pose.translation = pose.translation - t0;
        }
    
        // re-sample poses trace 1/3
        std::vector<Pose> poseTrace2;
        for( auto i=0u; i< poseTrace.size(); ++i)
            if( i % 3u == 0 )
            {
                poseTrace2.emplace_back();
                poseTrace2.back().rotation = poseTrace[i].rotation;
                poseTrace2.back().translation = poseTrace[i].translation;
            }

        WritePoseTrace( fnameEuler, poseTrace2, true );
    }

}

FUNC( SpikeValidateProjectionFromPoseTrace )
{
    std::string csv0 = "0.0777,   -0.1429, 1.3872,  -61.4676,   40.3759, 0.0000  ";
    pose_traces::Pose pose;
    pose.FromCsv(csv0);
    cout << endl;
    cout << pose.rotation << endl;
    cout << endl;
    cout << pose.translation << endl;
    
    std::string nameCameraParams = "C:/Users/NLV16453/projects/ulb_svs/config_files/TechnicolorMuseum/B3-sv-camparams.txt";

    std::vector<string> camnames = { "v0" };
    std::vector<Parameters> params;
    float sensorSize;
    read_cameras_paramaters(nameCameraParams, camnames, params, sensorSize);
    
    cout << endl;
    cout << params.size() << endl;
    cout << params[0].get_rotation() <<  endl;
    cout << endl;
    cout << params[0].get_translation() << endl;

    cout << endl;
    cout << sensorSize << endl;

    cout << "rotation error" << endl;
    cout << cv::norm( pose.rotation.t() * params[0].get_rotation(), cv::Matx33f::eye() ) << endl; // R.inv * R == eye 
    cout << "translation error " << endl;
    cout << cv::norm( pose.translation - params[0].get_translation() )  << endl;


}
