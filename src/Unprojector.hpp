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


/**
@file Unprojector.hpp
\brief The file containing the Unprojector class
*/
#pragma once
#include "Parameters.hpp"

#include <opencv2/core.hpp>

/**\brief Unprojector. 

Project the pixels from image space and depth map to euclidian coordinate system.
*/
class Unprojector
{
public:
	/**\brief Constructor*/
    Unprojector();

	/**\brief Constructor
	@param parameters Parameters of the view*/
    Unprojector(Parameters const& parameters);
	
	/**\brief Destructor*/
	virtual ~Unprojector();

	/** 
	\brief Unproject points in the image space to 3D space
	@param image_pos Position in image coordinates: u right, v down
	@param depth The depth increases with distance from virtual camera
	@return Result in OMAF Referential: x forward, y left, z up*/
	virtual cv::Mat3f unproject(cv::Mat2f image_pos, cv::Mat1f depth) const = 0;


	/**\brief Return the camera matrix
	@return The camera matrix
	*/
	cv::Matx33f const& get_camera_matrix() const;

private:
	Parameters parameters;
};
