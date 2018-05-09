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

#ifdef WITH_EASY_PROFILER
#ifdef WITH_EASY_PROFILER_TO_FILE
#include <chrono>
void Timer_Finalize() {
	std::chrono::milliseconds epoch = std::chrono::duration_cast< std::chrono::milliseconds >(
		std::chrono::system_clock::now().time_since_epoch()
		);
	std::string filename = std::to_string(epoch.count()) + ".prof";
	profiler::dumpBlocksToFile(filename.c_str());
}
#endif
#else
#include <omp.h>
std::map <Timer::threadid, std::map<Timer::timername, Timer::timepoint >> Timer::chronos;
void Timer::start(std::string timername, uint32_t opt_color) {
	chronos[omp_get_thread_num()][timername] = std::chrono::system_clock::now();
}

void Timer::end(std::string timername) {
	int tid = omp_get_thread_num();
	auto it = chronos[tid].find(timername);
	if (it != chronos[tid].end()) {
		auto end_clk = std::chrono::system_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_clk - chronos[tid][timername]).count();
		printf("%d - %s: %lld ms.\n", tid, timername.c_str(), time);

		chronos[tid].erase(it);
	}
	else {
		printf("ERROR: Chrono %s ended before starting on thread %d\n", timername.c_str(), tid);
	}
}

void Timer::begin()
{
}

void Timer::finalize()
{
}
#endif



