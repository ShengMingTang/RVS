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


#include "Timer.hpp"
#include "Pipeline.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[])
{
	try
	{
		PROF_BEGIN();

		// BK: As soon as we have the manual as input document (and when accepted as output document), then we add it to this statement
		std::cout
			<< " - -------------------------------------------------------------------------------------- -\n"
			<< "|    Reference View Synthesizer (RVS), branch: master                                      |\n"
			<< "|    MPEG2018/N17618 Common Test Conditions on 3DoF+ and Windowed 6DoF                     |\n"
			<< "|    Based on ULB Several inputs View Synthesis (SVS) with 3DoF+ extensions by Philips     |\n"
			<< " - -------------------------------------------------------------------------------------- -" << std::endl;

		PROF_START("parsing");

		std::string filename = (argc > 1) ? argv[1] : "./config_files/parameter_file.txt";

		Pipeline p(filename);

		PROF_END("parsing");
		PROF_START("view synthesis");

		p.execute();

		PROF_END("view synthesis");

		PROF_FINALIZE();
		return 0;
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}

