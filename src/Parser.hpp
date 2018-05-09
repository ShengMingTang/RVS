/*------------------------------------------------------------------------------ -

Copyright © 2018 - 2025 Université Libre de Bruxelles(ULB)

Authors : Sarah Fachada, Daniele Bonatto, Arnaud Schenkel
Contact : Gauthier.Lafruit@ulb.ac.be

SVS – Several inputs View Synthesis
This software synthesizes virtual views at any position and orientation in space,
from any number of camera input views, using depth image - based rendering
techniques.

Permission is hereby granted, free of charge, to the members of the Moving Picture
Experts Group(MPEG) obtaining a copy of this software and associated documentation
files(the "Software"), to use the Software exclusively within the framework of the
MPEG - I(immersive) and MPEG - I Visual activities, for the sole purpose of
developing the MPEG - I standard.This permission includes without limitation the
rights to use, copy, modify and merge copies of the Software, and explicitly
excludes the rights to publish, distribute, sublicense, sell, embed into a product
or a service and/or otherwise commercially exploit copies of the Software without
the written consent of the owner(ULB).

This permission is provided subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies, substantial portions or derivative works of the Software.

------------------------------------------------------------------------------ -*/

#include "helpers.hpp"
#include "View.hpp"
#include "Config.hpp"

#include <vector>



/**
Parsing of the config file
*/
class Parser
{
public:
	Parser(const std::string & filename);
	~Parser();

	View operator[](size_t idx) {
		return real_images[idx];
	}

	std::vector<View>& get_images() {
		return real_images;
	}
	Config get_config() { return config; };



private:

	bool is_SVS_file(const std::string & filename) const;

	void read_vsrs_config_file();

	void read_SVS_config_file();
	void read_ZValues();


	void generate_output_filenames();

	void print_results(const std::vector<std::string>& InputCameraParameterFile, const std::vector<std::string>& texture_names, const std::vector<std::string>& depth_names, const std::vector<std::string>& VirtualCameraParameterFile, const int number_RGB_cameras, const int number_D_cameras) const;

public:

	enum {
		RGB = 0,
		DEPTH = 1
	};

private:

	const std::string filename_parameter_file;

	Config config;


	std::vector<View> real_images;

};

