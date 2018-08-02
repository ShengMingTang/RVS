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

#include "EquirectangularProjection.hpp"



cv::Vec3f erp::calculate_euclidian_coordinates( const cv::Vec2f& phiTheta )
{
    const float& phi   = phiTheta[0];
    const float& theta = phiTheta[1];

    return cv::Vec3f(  std::cos(phi) * std::cos(theta),
                       std::sin(phi)*std::cos(theta),
                       std::sin(theta) );
}

cv::Vec2f erp::calculate_spherical_coordinates( const cv::Vec3f& xyz_norm )
{
    const float& x = xyz_norm[0];
    const float& y = xyz_norm[1];
    const float& z = xyz_norm[2];

    float phi   = std::atan2(y,x);
    float theta = std::asin( z );

    return cv::Vec2f(phi, theta);
}

erp::Unprojector::Unprojector(Parameters const& parameters)
{
	auto size = parameters.getSize();

    if( m_verticesXYZNormalized.size() == size )
        return;

    m_phiTheta.create(size);

    int height = size.height;
    int width = size.width;

	// Expand horizontally or vertically to 360deg x 180deg
	auto full_width = std::max(width, 2 * height);
	auto offset = (full_width - width) / 2;

    m_verticesXYZNormalized.create(size);
    for (int i = 0; i < height; ++i)
    {
        float vPos;
		float const eps = 1e-3f;

		// Adjust vPos to render poles
		if (i == 0)
			vPos = eps;
		else if (i == height - 1)
			vPos = height - eps;
		else
			vPos = i + 0.5f;

        float theta = erp::calculate_theta( vPos, height );

        for (int j = 0; j < width; ++j)
        {
	    float hPos = 0.5f + j;
            float phi  = erp::calculate_phi( offset + hPos, full_width );

            m_phiTheta(i,j) = cv::Vec2f(phi, theta);
            
            m_verticesXYZNormalized(i, j) = erp::calculate_euclidian_coordinates( m_phiTheta(i,j) );
        }
    }
}

cv::Mat3f erp::Unprojector::unproject( cv::Mat1f radiusMap) const 
{
    cv::Size size = radiusMap.size();

    CV_Assert( size == m_verticesXYZNormalized.size() );

    cv::Mat3f verticesXYZ(size);

    for( int i=0; i < verticesXYZ.rows; ++i )
        for( int j =0; j < verticesXYZ.cols; ++j )
            verticesXYZ(i,j) = radiusMap(i,j) * m_verticesXYZNormalized(i,j);

    return verticesXYZ;
}

cv::Mat3f erp::Unprojector::unproject( cv::Mat2f /*image_pos*/, cv::Mat1f radiusMap) const 
{
    return unproject(radiusMap);
}

erp::Projector::Projector(Parameters const& parameters)
	: m_parameters(parameters)
{}


cv::Mat2f erp::Projector::project( cv::Mat3f vecticesXYZ, cv::Mat1f& imRadius, WrappingMethod& wrapping_method ) const
{
	auto input_size = vecticesXYZ.size();
	auto output_size = m_parameters.getSize();

	// Expand horizontally or vertically to 360deg x 180deg
	auto full_width = std::max(output_size.width, 2 * output_size.height);
	auto offset = (full_width - output_size.width) / 2;

	cv::Mat2f imUV(input_size);
	imRadius.create(input_size);

    for(int i = 0; i < input_size.height; ++i )
        for( int j = 0; j < input_size.width; ++j )
        {
            cv::Vec3f xyz      = vecticesXYZ(i,j);
            float radius       = static_cast<float>( cv::norm(xyz) );
            imRadius(i,j)      = radius;

            cv::Vec3f xyzNorm  = xyz / radius;
            cv::Vec2f phiTheta = erp::calculate_spherical_coordinates(xyzNorm);

            imUV(i,j)[0] = erp::calculate_horizontal_image_coordinate(phiTheta[0], full_width) - offset;
            imUV(i,j)[1] = erp::calculate_vertical_image_coordinate(phiTheta[1], output_size.height);
        }

	wrapping_method = output_size.width == full_width
		? WrappingMethod::horizontal
		: WrappingMethod::none;

    return imUV;
}

