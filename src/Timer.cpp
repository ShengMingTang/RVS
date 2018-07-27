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
void Timer::start(std::string timername, uint32_t /*opt_color*/) {
	chronos[omp_get_thread_num()][timername] = std::chrono::system_clock::now();
}

void Timer::end(std::string timername) {
	int tid = omp_get_thread_num();
	auto it = chronos[tid].find(timername);
	if (it != chronos[tid].end()) {
		auto end_clk = std::chrono::system_clock::now();
		long long time = std::chrono::duration_cast<std::chrono::milliseconds>(end_clk - chronos[tid][timername]).count();
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



