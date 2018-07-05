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

erp::Unprojector::Unprojector(Parameters const& parameters, const cv::Size& size)
    : ::Unprojector(parameters)
{
    create(size);
}

void erp::Unprojector::create(cv::Size size)
{
    if( verticesXYZNormalized.size() == size )
        return;

    phiTheta.create(size);

    int height = size.height;
    int width = size.width;

	// Expand horizontally or vertically to 360deg x 180deg
	auto full_width = std::max(width, 2 * height);
	auto offset = (full_width - width) / 2;

    verticesXYZNormalized.create(size);
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

            phiTheta(i,j) = cv::Vec2f(phi, theta);
            
            verticesXYZNormalized(i, j) = erp::calculate_euclidian_coordinates( phiTheta(i,j) );
        }
    }


}




cv::Mat3f erp::Unprojector::unproject( cv::Mat1f radiusMap) const 
{
    cv::Size size = radiusMap.size();

    CV_Assert( size == verticesXYZNormalized.size() );

    cv::Mat3f verticesXYZ(size);

    for( int i=0; i < verticesXYZ.rows; ++i )
        for( int j =0; j < verticesXYZ.cols; ++j )
            verticesXYZ(i,j) = radiusMap(i,j) * verticesXYZNormalized(i,j);

    return verticesXYZ;
}

cv::Mat3f erp::Unprojector::unproject( cv::Mat2f /*image_pos*/, cv::Mat1f radiusMap) const 
{
    // TODO: Use image_pos instead of calculating them again
    
    return unproject(radiusMap);
}



erp::Projector::Projector(Parameters const& parameters, cv::Size size)
    : ::Projector(size)
{
}


cv::Mat2f erp::Projector::project( cv::Mat3f vecticesXYZ, cv::Mat1f& imRadius, WrappingMethod& wrapping_method ) const
{
	auto input_size = vecticesXYZ.size();
	auto output_width = get_size().width;
	auto output_height = get_size().height;

	// Expand horizontally or vertically to 360deg x 180deg
	auto full_width = std::max(output_width, 2 * output_height);
	auto offset = (full_width - output_width) / 2;

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
            imUV(i,j)[1] = erp::calculate_vertical_image_coordinate(phiTheta[1], output_height);
        }

	wrapping_method = output_width == full_width
		? WrappingMethod::HORIZONTAL
		: WrappingMethod::NONE;

    return imUV;
}

