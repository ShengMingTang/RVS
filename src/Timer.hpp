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

#pragma once

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