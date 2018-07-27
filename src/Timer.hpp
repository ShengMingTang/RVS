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

#ifndef _TIMER_HPP_
#define _TIMER_HPP_

/**
@file Timer.hpp
*/

#ifdef WITH_EASY_PROFILER
#include <easy/profiler.h>
#else 
#include <chrono>
#include <map>
#include <string>

/**
\brief Profiler class

If WITH_EASY_PROFILER flag is on, saves the computation time wit a easy profiler file

Else the computation time is displayed during the execution
*/
class Timer {
public:
	/**\brief Start the profiling here
	@param timername Name of the profiled block*/
	static void start(std::string timername, uint32_t opt_color = 0xfff080aa);

	/**\brief End the profiling 
	@param timername Name of the profiled block*/
	static void end(std::string timername);

	static void begin();

	static void finalize();

private:
	Timer();

public:

	typedef std::chrono::time_point<std::chrono::system_clock> timepoint;
	typedef std::string timername;
	typedef int threadid;

	static std::map <threadid, std::map<timername, timepoint >> chronos;
};
#endif

#ifdef WITH_EASY_PROFILER
#define PROF_START_1_ARGS(TIMERNAME) EASY_BLOCK((TIMERNAME));
#define PROF_START_2_ARGS(TIMERNAME, OPT_COLOR) EASY_BLOCK((TIMERNAME), (OPT_COLOR));
#define PROF_END(TIMERNAME) EASY_END_BLOCK;
#ifdef WITH_EASY_PROFILER_TO_FILE
void Timer_Finalize();
#define PROF_BEGIN() EASY_PROFILER_ENABLE;
#define PROF_FINALIZE() Timer_Finalize();
#else
#define PROF_BEGIN() profiler::startListen();
#define PROF_FINALIZE() profiler::stopListen();
#endif
#else 
#define PROF_START_1_ARGS(TIMERNAME) Timer::start((TIMERNAME));
#define PROF_START_2_ARGS(TIMERNAME, OPT_COLOR) Timer::start((TIMERNAME), (OPT_COLOR));
#define PROF_END(TIMERNAME) Timer::end((TIMERNAME));
#define PROF_BEGIN() Timer::begin();
#define PROF_FINALIZE() Timer::finalize();
#endif

#define GET_3TH_ARG(arg1, arg2, arg3, ...) arg3

#define PROF_START_MACRO_CHOOSER(...) \
    GET_3TH_ARG(__VA_ARGS__, PROF_START_2_ARGS, \
		PROF_START_1_ARGS, )

#define PROF_START(...) PROF_START_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#endif