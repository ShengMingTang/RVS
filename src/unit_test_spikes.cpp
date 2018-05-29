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
#include "transform.hpp"


#include "opencv2/opencv.hpp"

#include <iostream>

using namespace std;
using namespace cv;


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
    cv::Mat depth = read_depth(nameDepth, size, bit_depth_depth, z_near, z_far );

    depth.convertTo( depth, -1, 1.0 / 2000);

    cv::imshow( "img",   ScaleDown(im, 0.5)    );
    cv::imshow( "depth", ScaleDown(depth, 0.5) );
    cv::waitKey(0);

}

#if 0
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

#endif


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



struct ErpReader
{
    std::string nameImgT   = "./classroom/img%04d.png";
    std::string nameDepthT = "./classroom/depth%04d.exr";

    void read( int num )
    {
        image          = cv::imread( cv::format(nameImgT.c_str(), num) );
        imRadius3f     = cv::imread( cv::format(nameDepthT.c_str(), num), cv::IMREAD_UNCHANGED );
        
        CV_Assert( !image.empty() && !imRadius3f.empty() );
        
        image.convertTo( image3f, image3f.type() );
        imRadius       = SelectChannel( imRadius3f, 0 );
        imRadius.convertTo( imRadius8u, imRadius8u.type(), -255.0/ 10.0, 255.0 );
        size = imRadius.size();
    }

    cv::Size  size; 
    cv::Mat3b image; 
    cv::Mat3f image3f;
    cv::Mat3f imRadius3f;
    cv::Mat1f imRadius;
    cv::Mat1b imRadius8u;
};




FUNC( Spike_ErpViewSynthesis )
{
    ErpReader erpz1;
    erpz1.read(1);

    ErpReader erpz2;
    erpz2.read(2);

    auto size = erpz1.size;

    cv::imshow( "img_2",   ScaleDown(erpz2.image, 0.25)    );
    cv::imshow( "imRadius8u_2", ScaleDown(erpz2.imRadius8u, 0.25) );
    cv::waitKey(100);


    erp::Unprojector unprojector;
    unprojector.create(size);
    auto verticesXyz = unprojector.unproject(erpz1.imRadius);

    const auto translation = cv::Vec3f(0.f, 0.05f, 0.f );
    cv::Mat3f verticesXyzNew = verticesXyz + translation;

    rescale = 1.f;
    cv::Size sizeOut = size;

    erp::Projector projector;
    cv::Mat1f imRadiusNew;
    cv::Mat2f imUVnew     = projector.project( verticesXyzNew, imRadiusNew );

    bool wrapHorizontal = true;

    cv::Mat1f depth, quality;
    cv::Mat3f imResult3f = transform_trianglesMethod(erpz1.image3f, imRadiusNew, imUVnew, sizeOut, depth, quality, wrapHorizontal);


    cv::Mat3b imResult;
    imResult3f.convertTo(imResult, imResult.type() );

    cv::imshow( "imResult",   ScaleDown(imResult, 0.25)    );

    cv::Mat3b imDiff;
    cv::absdiff(imResult, erpz2.image, imDiff );
    cv::imshow( "imDiff",   ScaleDown(imDiff, 0.25)    );

    cv::Mat3b imDiffRef;
    cv::absdiff(erpz1.image, erpz2.image, imDiffRef );
    cv::imshow( "imDiffRef",   ScaleDown(imDiffRef, 0.25)    );


    cv::waitKey(0);


}


struct ErpReaderRaw
{
    std::string nameImg   = "./ClassRoomVideo/v0_4096_2048_420_10b.yuv";
    std::string nameDepth = "./ClassRoomVideo/v0_4096_2048_0_8_1000_0_420_10b.yuv";
    
    cv::Size size = cv::Size(4096,2048);
    int bit_depth = 10;
    float z_near = 0.8;
    float z_far  = 1e6;

    void read( int num )
    {
        num;
        image3f  = read_color(nameImg, size, bit_depth);
        
        imRadius = read_depth(nameDepth, size, bit_depth, z_near, z_far);

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

FUNC( Spike_ErpViewSynthesisRaw )
{
    ErpReaderRaw erpz1;
    erpz1.read(0);

    
    cv::Mat3b imBGR;
    cv::cvtColor(erpz1.image, imBGR, cv::COLOR_YUV2BGR  );
    cv::imshow( "im",    ScaleDown(imBGR, 0.25)    );
    
    cv::imshow( "depth", ScaleDown(erpz1.imRadius8u, 0.25)    );


    cv::waitKey(0);


}

FUNC( Spike_WriteJSON )
{


    FileStorage fs("./Spike_WriteJSON.json", FileStorage::WRITE);
    fs << "frameCount" << 5;
    //time_t rawtime; time(&rawtime);
    //fs << "calibrationDate" << asctime(localtime(&rawtime));
    Mat cameraMatrix = (Mat_<double>(3,3) << 1000, 0, 320, 0, 1000, 240, 0, 0, 1);
    Mat distCoeffs = (Mat_<double>(5,1) << 0.1, 0.01, -0.001, 0, 0);
    fs << "cameraMatrix" << cameraMatrix << "distCoeffs" << distCoeffs;
    fs << "features" << "[";
    for( int i = 0; i < 3; i++ )
    {
        int x = rand() % 640;
        int y = rand() % 480;
        uchar lbp = rand() % 256;
        fs << "{:" << "x" << x << "y" << y << "lbp" << "[:";
        for( int j = 0; j < 8; j++ )
            fs << ((lbp >> j) & 1);
        fs << "]" << "}";
    }
    fs << "]";
    fs.release();

}

FUNC( Spike_ParseJSON )
{
    //std::string name   = "./ClassRoomVideo/ClassroomVideo.json";
    std::string name   = "./ClassRoomVideo/example.json";

    cv::FileStorage fs(name.c_str(), cv::FileStorage::READ);

    //string Content_name = static_cast<std::string>( fs["Content_name"] );

    //std::cout << endl;
    //std::cout << Content_name << endl;

}


FUNC(Spike_ULB_Unicorn_Triangles_Simple_Erp)
{
    Pipeline p("./config_files/Unicorn_Triangles_Simple_ToErp.cfg");
    p.execute();

}


#if 0
To read the previously written XML, YAML or JSON file, do the following:
-#  Open the file storage using FileStorage::FileStorage constructor or FileStorage::open method.
In the current implementation the whole file is parsed and the whole representation of file
storage is built in memory as a hierarchy of file nodes (see FileNode)
-#  Read the data you are interested in. Use FileStorage::operator [], FileNode::operator []
and/or FileNodeIterator.
-#  Close the storage using FileStorage::release.
Here is how to read the file created by the code sample above:
@code
FileStorage fs2("test.yml", FileStorage::READ);
// first method: use (type) operator on FileNode.
int frameCount = (int)fs2["frameCount"];
String date;
// second method: use FileNode::operator >>
fs2["calibrationDate"] >> date;
Mat cameraMatrix2, distCoeffs2;
fs2["cameraMatrix"] >> cameraMatrix2;
fs2["distCoeffs"] >> distCoeffs2;
cout << "frameCount: " << frameCount << endl
<< "calibration date: " << date << endl
<< "camera matrix: " << cameraMatrix2 << endl
<< "distortion coeffs: " << distCoeffs2 << endl;
FileNode features = fs2["features"];
FileNodeIterator it = features.begin(), it_end = features.end();
int idx = 0;
std::vector<uchar> lbpval;
// iterate through a sequence using FileNodeIterator
for( ; it != it_end; ++it, idx++ )
{
    cout << "feature #" << idx << ": ";
    cout << "x=" << (int)(*it)["x"] << ", y=" << (int)(*it)["y"] << ", lbp: (";
    // you can also easily read numerical arrays using FileNode >> std::vector operator.
    (*it)["lbp"] >> lbpval;
    for( int i = 0; i < (int)lbpval.size(); i++ )
        cout << " " << (int)lbpval[i];
    cout << ")" << endl;
}
fs2.release();
@endcode
Format specification    {#format_spec}
--------------------
`([count]{u|c|w|s|i|f|d})`... where the characters correspond to fundamental C++ types:
-   `u` 8-bit unsigned number
-   `c` 8-bit signed number
-   `w` 16-bit unsigned number
-   `s` 16-bit signed number
-   `i` 32-bit signed number
-   `f` single precision floating-point number
-   `d` double precision floating-point number
-   `r` pointer, 32 lower bits of which are written as a signed integer. The type can be used to
store structures with links between the elements.
`count` is the optional counter of values of a given type. For example, `2if` means that each array
element is a structure of 2 integers, followed by a single-precision floating-point number. The
equivalent notations of the above specification are `iif`, `2i1f` and so forth. Other examples: `u`
means that the array consists of bytes, and `2d` means the array consists of pairs of doubles.
@see @ref filestorage.cpp
*/

#endif
