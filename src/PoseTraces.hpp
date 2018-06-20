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

/**
@file PoseTraces.hpp
\brief The file caintaining the tool to handle rotations and translations
*/

namespace pose_traces
{

/**\brief Describe a translation and a rotation*/
struct Pose
{
	/**\brief Create a pose from a CSV file
	@param rowCsv CSV line to read
	\see ReadPoseTrace()*/
    void FromCsv( std::string rowCsv );

	/**\brief Export a pose to CSV file format
	@param eulerAngles If true write the rotation in Euler angles
	@return A string containing the CVS description of the Pose*/
    std::string ToCsv(bool eulerAngles ) const;
	
	/**\brief Invert
	
		- Transpose the rotation
		- Translation:-R*translation*/
    Pose Invert() const;

	/**\brief Rotation matrix*/
    cv::Matx33f rotation;
	
	/**\brief Translation vector*/
    cv::Vec3f   translation;
};

/**\brief Read the poses in a CSV file
@param fileNameCsv CSV file to read
@return The poses in the CSF file
\see FromCsv()*/
std::vector<Pose> ReadPoseTrace( std::string fileNameCsv );

/**\brief Write the poses in a CSV file
@param fileNameCsv CSV file to write
@param poseTrace Poses to write
@param useEulerAngles If true write the rotation in Euler angles
\see ToCsv()*/
void WritePoseTrace( std::string fileNameCsv, const std::vector<Pose>& poseTrace, bool useEulerAngles );


namespace detail
{
    /**\brief Rotation matrix from rotation around the x axis*/
    inline cv::Matx33f RotationMatrixFromRotationAroundX( float rx )
    {
        return cv::Matx33f( 1.f, 0.f, 0.f,
            0.f, cos(rx), -sin(rx),
            0.f, sin(rx),  cos(rx)   );
    }

	/**\brief Rotation matrix from rotation around the y axis*/
    inline cv::Matx33f RotationMatrixFromRotationAroundY( float ry )
    {
        return cv::Matx33f(  cos(ry), 0.f, sin(ry),
            0.f, 1.f, 0.f,
            -sin(ry), 0.f, cos(ry)  );
    }

	/**\brief Rotation matrix from rotation around the z axis*/
    inline cv::Matx33f RotationMatrixFromRotationAroundZ( float rz )
    {
        return cv::Matx33f(  cos(rz), -sin(rz), 0.f,
            sin(rz),  cos(rz), 0.f,
            0.f, 0.f, 1.f  );
    }

	/**\brief is a float zero
	@param val float to evaluate
	@param eps Maximal value of the number to be considered as zero
	@return True if the number is almost null*/
    inline bool AlmostZero(float val, float eps = 1e-7)
    {
        return std::abs(val) < eps;
    }
    
    // euler = { yaw, pitch, roll }
    // yaw   = [-pi  , pi]
    // pitch = <-pi/2, pi/2>
    // roll  = [-pi  , pi]    

	/**\brief Rotation matrix to Euler angles 

	Order: XYZ
	@param R Rotation matrix
	@return Euler angles vector (yaw pitch roll)*/
    cv::Vec3f RotationMatrixToEulerAngles( const cv::Matx33f& R );

	/**\brief Rotation matrix from Euler angles

	Order: XYZ
	@param euler Euler angles vector (yaw pitch roll)
	@return Rotation matrix*/
    cv::Matx33f EulerAnglesToRotationMatrix(const cv::Vec3f &euler);
    
	/**\brief Convert a file into an array of strings
	@param fileName File to read
	@return Array of string. Each string is a line of the file*/
    std::vector<std::string>  GetTextLines( const std::string& fileName );

	/**
	@param line
	@return String*/
    std::string Trim(std::string line);

	/**
	@param str
	@param ch
	*/
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
