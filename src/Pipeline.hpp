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

/*------------------------------------------------------------------------------ -

This source file has been modified by Koninklijke Philips N.V. for the purpose of
of the 3DoF+ Investigation.
Modifications copyright © 2018 Koninklijke Philips N.V.

Extraction of a generalized unproject -> translate/rotate -> project flow

Author  : Bart Kroon, Bart Sonneveldt
Contact : bart.kroon@philips.com

------------------------------------------------------------------------------ -*/

#include "View.hpp"
#include "Config.hpp"

/**
@file Pipeline.hpp
\brief The file containing the pipeline functions
*/

/**
\brief The pipeline of view synthesis

The pipeline executes the following steps:
	- Parsing the configuration file (see Parser)
	- Loading the input reference views (see InputView, and load_images());
	- View synthesis of the target view once for each input reference view (see SynthetizedView);
	- Blending all the SynthetizedView together by assigning a per-pixel quality to each synthesized view (see BlendedView);
	- Inpainting to fill the remaining holes (see inpaint());
	- Writing the output (see write_color()).
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

	Execution consists in parsing, image loading, view computation (warping, blending, inpainting), view writing
	*/
	void execute();

private:
	/**
	\brief Parse the config file
	*/
	void parse();

	/**
	\brief Loads all the input views
	@param frame Frame number to load (for YUV image)
	*/
	void load_images(int frame);

	/**
	\brief Computes all the virtual views

	Executes the view view computation (warping, blending, inpainting) and writing.
	@param frame Frame number of the frame being computed (for YUV image)
	*/
	void compute_views(int frame);

	/**
	\brief Dump maps for analysis purposes
	*/
	void dump_maps(std::size_t input_idx, std::size_t virtual_idx, View const& synthesizer, View const& blender);

	/** File containing the parameters of the view synthesis*/
	std::string filename;

	/** Config of the view synthesis*/
	Config config;

	/** Input reference views */
	std::vector<InputView> input_images;
};

