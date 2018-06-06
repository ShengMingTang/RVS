/* ------------------------------------------------------------------------------ -

Copyright © 2018 Koninklijke Philips N.V.

Authors : Bart Sonneveldt, Bart Kroon
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

#include "PoseTraces.hpp"


#include <fstream>
#include <iostream>


using namespace pose_traces;


void Pose::FromCsv( std::string rowCsv )
{
    auto vec = detail::StringToVec<float>( rowCsv, ',' );

    auto numExpectedForRotationAndTranslation     = 12u;
    auto numExpectedForTranslationAndEulerAngles  = 6u;
    auto numExpectedForTimeStampAndModelMatrix    = 17u;

    if( vec.size() == numExpectedForRotationAndTranslation )
    {
        rotation     = cv::Matx33f( &vec[0] );
        translation  = cv::Vec3f(vec[9], vec[10], vec[11] );
    }
    else if( vec.size() == numExpectedForTimeStampAndModelMatrix )
    {
        // vec[0] = timestamp
        rotation     = cv::Matx33f( vec[1], vec[2], vec[3],   vec[5], vec[6], vec[7],   vec[9], vec[10], vec[11] );
        translation  = cv::Vec3f(vec[4], vec[8], vec[12] );
    }
    else if( vec.size() == numExpectedForTranslationAndEulerAngles )
    {
        cv::Vec3f eulerDegrees = cv::Vec3f(vec[3], vec[4], vec[5]);
        cv::Vec3f euler        = cv::Vec3f(vec[3], vec[4], vec[5]) * float( CV_PI / 180.0 );
        
        // local coordinates
        rotation          = detail::EulerAnglesToRotationMatrix(euler);
        translation       = cv::Vec3f( vec[0], vec[1], vec[2] );

#if 0
        // convert to global
        Pose poseInv      = Invert();
        rotation          = poseInv.rotation;
        translation       = poseInv.translation;
#endif
    }
    else
        throw std::runtime_error( "Error: expected 6 or 12 or 17 comma separated values \n" + rowCsv );
}

std::string Pose::ToCsv(bool eulerAngles) const
{
    std::ostringstream oss;
    
    if( eulerAngles )
    {
#if 0
        auto poseInv = Invert();
        auto euler = detail::RotationMatrixToEulerAngles(poseInv.rotation);
        auto T = poseInv.translation;
#endif   
        auto euler = detail::RotationMatrixToEulerAngles(rotation);
        auto eulerDegrees = euler * float( 180.0 / CV_PI );
        
        auto T = translation;
        auto E = eulerDegrees;
        
        oss << T[0] << ", " << T[1] << ", " << T[2] << ", " << E[0] << ", " << E[1] << ", " << E[2];
    }
    else
    {
        auto R = rotation.reshape<1,9>();
        for(int j=0; j < 9; ++j)
            oss << R(0,j) << ", ";
    
        auto T = translation;
        oss << T[0] << ", " << T[1] << ", " << T[2];
    }

    return oss.str();

}



Pose Pose::Invert() const
{
    Pose poseInv;
    poseInv.rotation    = rotation.t();
    poseInv.translation = poseInv.rotation * -translation;

    return poseInv;
}

std::vector<Pose> pose_traces::ReadPoseTrace( std::string fileNameCsv )
{
    auto lines = detail::GetTextLines(fileNameCsv);

    auto N = lines.size();

    std::vector<Pose> poseTrace;

    for( int i = 1; i<N; ++i)   // skip first line
    {
        auto line = detail::Trim( lines[i] );
        if( !line.empty() )
        {
            poseTrace.emplace_back();
            poseTrace.back().FromCsv( line );
        }
    }

    return poseTrace;
}

void pose_traces::WritePoseTrace( std::string fileNameCsv, const std::vector<Pose>& poseTrace, bool useEulerAngles )
{
    std::ofstream os( fileNameCsv.c_str(), std::ofstream::out );
    if( !os.is_open() )
        throw std::runtime_error( "Error: write to: "+  fileNameCsv ) ;

    if( useEulerAngles)
        os << "X,Y,Z,Yaw,Pitch,Roll" << std::endl;
    else
        os << std::endl;    // skip information line


    for( const auto& pose : poseTrace )
        os << pose.ToCsv(useEulerAngles) << std::endl;

    os.close();

}


cv::Vec3f detail::RotationMatrixToEulerAngles( const cv::Matx33f& R )
{
    float yaw = 0.f, pitch = 0.f, roll = 0.f;

    float pi2 = float( CV_PI/2 );

    if( AlmostZero( R(0, 0) ) && AlmostZero( R(1, 0) )  )  // Gimbal lock check
    {
        yaw = atan2(R(1, 2), R(0, 2));

        if(R(2, 0) < 0.f)
            pitch = pi2;
        else
            pitch = -pi2;
        roll = 0.f;		
    }
    else
    {
        yaw = atan2(R(1, 0), R(0, 0));

        if( AlmostZero( R(0, 0) ) )
            pitch = atan2( -R(2, 0), R(1, 0) / sin(yaw) );
        else
            pitch = atan2( -R(2, 0), R(0, 0) / cos(yaw) );

        roll = atan2(R(2, 1), R(2, 2));
    }

    return cv::Vec3f( yaw, pitch, roll);

}





// Calculates rotation matrix given Euler angles.
cv::Matx33f detail::EulerAnglesToRotationMatrix(const cv::Vec3f &euler)
{
    float z = euler[0];
    float y = euler[1];
    float x = euler[2];

    auto Rx = RotationMatrixFromRotationAroundX(x);
    auto Ry = RotationMatrixFromRotationAroundY(y);
    auto Rz = RotationMatrixFromRotationAroundZ(z);

    cv::Matx33f R = Rz * Ry * Rx;

    return R;

}


std::vector<std::string>  detail::GetTextLines( const std::string& fileName )
{
    std::vector<std::string> lines;

    std::ifstream is( fileName.c_str() );
    std::string str;
    if( is.is_open() )
    {
        for(; !is.eof(); )
        {
            getline( is, str );
            lines.push_back( str );
        }
        is.close();
    }
    else
        throw std::runtime_error( "Error: failed to open " + fileName);

    return lines;
}

std::string detail::Trim(std::string line)
{
    while (line.length()>0 && isspace(line[0]))
        line = line.substr(1, line.length()-1);
    while (line.length()>0 && isspace(line[line.length()-1]))
        line = line.substr(0, line.length()-1);
    return line;
}
