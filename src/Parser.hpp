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

#include "Parameters.hpp"
#include "View.hpp"
#include "Config.hpp"

#include <vector>


/**
@file Parser.hpp
*/

/**
Parsing of the config file
*/
class Parser
{
public:
	/**
	\brief Parse the config file.
	@param filename File to be parsed. 
	*/
	Parser(const std::string & filename);

	/**
	\brief Destructor.
	*/
	~Parser();

	/**\brief Get the View number idx*/
	View operator[](size_t idx) {
		return real_images[idx];
	}

	/**
	\brief Returns the input views
	*/
	std::vector<View>& get_images() {
		return real_images;
	}

	/**
	\brief Returns the configuration (see Config) 
	*/
	Config get_config() { return config; };



private:

	bool is_SVS_file(const std::string & filename) const;

	void read_vsrs_config_file();

	void read_SVS_config_file();
	void read_ZValues();


	void generate_output_filenames();

	void print_results(const std::vector<std::string>& InputCameraParameterFile, const std::vector<std::string>& texture_names, const std::vector<std::string>& depth_names, const std::vector<std::string>& VirtualCameraParameterFile, const int number_RGB_cameras, const int number_D_cameras) const;

public:

	/**
	\brief Working color space
	*/
	enum {
		RGB = 0,
		DEPTH = 1
	};

private:

	const std::string filename_parameter_file;

	Config config;


	std::vector<View> real_images;

};

/**
\brief Reads the camera parameters for the cameras in the file filename
@param filename Input file with the cameras parameters
@param camnames Cameras to be read in the file
@param params Output cameras parameters
@param sensor_size Size of the sensor (should be the same as the input image size)
*/
void read_cameras_parameters(std::string filename, std::vector<std::string>& camnames, std::vector<Parameters>& params, float& sensor_size,
	std::vector<float> *zfar, std::vector<float> *znear);
