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

extern bool with_opengl;
#if WITH_OPENGL
#include "helpersGL.hpp"
#endif

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

		if (argc > 2) // -noopengl
			with_opengl = false;

#if !WITH_OPENGL
		with_opengl = false;
		if (argc > 2 && !with_opengl)
			throw std::logic_error("Too many parameters - not compiled with OpenGL");
#endif


#if WITH_OPENGL
		if (with_opengl) {
			PROF_START("OpenGL Context");
			context_init();
			PROF_END("OpenGL Context");
		}
#endif
		
		// BK: As soon as we have the manual as input document (and when accepted as output document), then we add it to this statement
		std::cout
			<< " - -------------------------------------------------------------------------------------- -\n"
			<< "|    Reference View Synthesizer (RVS), branch: v1.0-dev                                    |\n"
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

