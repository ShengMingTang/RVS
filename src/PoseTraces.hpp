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

#pragma once

#include <opencv2/core.hpp>

#include <vector>
#include <string>
#include <stdexcept>

namespace pose_traces
{

struct Pose
{
    void FromCsv( std::string rowCsv );
    std::string ToCsv(bool eulerAngles ) const;
    Pose Invert() const;

    cv::Matx33f rotation;
    cv::Vec3f   translation;
};

std::vector<Pose> ReadPoseTrace( std::string fileNameCsv );

void WritePoseTrace( std::string fileNameCsv, const std::vector<Pose>& poseTrace, bool useEulerAngles );


namespace detail
{
    
    inline cv::Matx33f RotationMatrixFromRotationAroundX( float rx )
    {
        return cv::Matx33f( 1.f, 0.f, 0.f,
            0.f, cos(rx), -sin(rx),
            0.f, sin(rx),  cos(rx)   );
    }

    inline cv::Matx33f RotationMatrixFromRotationAroundY( float ry )
    {
        return cv::Matx33f(  cos(ry), 0.f, sin(ry),
            0.f, 1.f, 0.f,
            -sin(ry), 0.f, cos(ry)  );
    }

    inline cv::Matx33f RotationMatrixFromRotationAroundZ( float rz )
    {
        return cv::Matx33f(  cos(rz), -sin(rz), 0.f,
            sin(rz),  cos(rz), 0.f,
            0.f, 0.f, 1.f  );
    }

    inline bool AlmostZero(float val, float eps = 1e-7)
    {
        return std::abs(val) < eps;
    }
    
    // euler = { yaw, pitch, roll }
    // yaw   = [-pi  , pi]
    // pitch = <-pi/2, pi/2>
    // roll  = [-pi  , pi]    

    cv::Vec3f RotationMatrixToEulerAngles( const cv::Matx33f& R );

    cv::Matx33f EulerAnglesToRotationMatrix(const cv::Vec3f &euler);
    
    std::vector<std::string>  GetTextLines( const std::string& fileName );

    std::string Trim(std::string line);

    template<class T>
    std::vector<T> StringToVec(const std::string& str, char ch)
    {
        std::vector<T> vec;
        std::istringstream lineStream( str );
        std::string s;

        while (std::getline( lineStream, s, ch) )
        {
            s = Trim(s);
            if ( !s.empty() )
            {
                T value;
                std::istringstream valueStream(s);
                valueStream >> value;
                if( valueStream.fail() )
                    throw std::runtime_error( "Error: parsing " + str );

                vec.push_back(value);
            }
        }

        return vec;
    }



} // namespace


} // namespace


