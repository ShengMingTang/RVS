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
  Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be  Daniele Bonatto, Daniele.Bonatto@ulb.ac.be  Arnaud Schenkel, arnaud.schenkel@ulb.ac.be

Koninklijke Philips N.V., Eindhoven, The Netherlands:
  Bart Kroon, bart.kroon@philips.com
  Bart Sonneveldt, bart.sonneveldt@philips.com
*/

#ifndef _POSE_TRACES_HPP_
#define _POSE_TRACES_HPP_

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

#endif