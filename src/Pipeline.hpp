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

#include "View.hpp"
#include "Config.hpp"

/**
The pipeline of view synthesis
*/
class Pipeline
{
public:
	/**
	\brief Constructor
	@param filename Config file (in VSRS config file format or SVS config fileformat)
	*/
	Pipeline(std::string filename);
	/**
	\brief Destructor
	*/
	~Pipeline();

	/**
	\brief Execution of the view synthesis

	Execution consist in parsing, image loading, view computation (warping, blending, inpainting), view writing
	*/
	void execute();

private:
	/**
	\brief Parse the config file
	*/
	void parse();

	/**
	\brief Create the good number of input views
	*/
	void create_images();
	void create_image(int idx);

	void load_image(int i);
	/**
	\brief Load all the input views
	*/
	void load_images();

	/**
	\brief Compute all the virtual views
	*/
	void compute_views();
	void compute_view(int i);

private:
	std::string filename;

	Config config;
	std::vector<View> input_images;
};

