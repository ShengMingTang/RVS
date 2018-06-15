cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
set(PROJECT_NAME "RVS")
project(${PROJECT_NAME})

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_SOURCE_DIR}/cmake/easy_profiler/")

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})


find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# force static runtime libraries for msvc builds
if(MSVC)
  set(variables 
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
	CMAKE_CXX_FLAGS_RELWITHDEBINFO
	CMAKE_CXX_FLAGS_MINSIZEREL
  )
  foreach(variable ${variables})
	if(${variable} MATCHES "/MD")
	  string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
	endif()
  endforeach()
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /PROFILE")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")
    if(NOT WIN32)
        set(GLAD_LIBRARIES dl)
    endif()
endif()

option(WITH_CUDA "Build the cuda version" OFF)

set(CUDA_LIB "")
if(WITH_CUDA)
	find_package(CUDA QUIET)
	if(CUDA_FOUND)
		include_directories(${CUDA_INCLUDE_DIRS})
		set(ALL_CUDA_LIBS ${CUDA_LIBRARIES} ${CUDA_cusparse_LIBRARY} ${CUDA_cublas_LIBRARY})
		set(LIBS ${LIBS} ${ALL_CUDA_LIBS})
		message(STATUS "CUDA_LIBRARIES: ${CUDA_INCLUDE_DIRS} ${ALL_CUDA_LIBS}")
		set(CUDA_PROPAGATE_HOST_FLAGS ON)
		set(CUDA_SEPARABLE_COMPILATION OFF)
		list(APPEND CUDA_NVCC_FLAGS -gencode=arch=compute_30,code=compute_30)
		list(APPEND CUDA_NVCC_FLAGS -gencode=arch=compute_52,code=sm_52)
		file(GLOB_RECURSE CUDA_SOURCES src/*.cu)
		cuda_add_library(${PROJECT_NAME}_cuda STATIC ${CUDA_SOURCES})
		set(CUDA_LIB ${PROJECT_NAME}_cuda)
	endif()
endif()


option(WITH_EASY_PROFILER "Build the easy_profiler" OFF)

set(EASYPROFILER_LIB "")
if(WITH_EASY_PROFILER)
	find_package(easy_profiler QUIET)
	# D:/LIB/INSTALL/easy_profiler/lib/cmake/easy_profiler
	if(easy_profiler_FOUND)
		set(EASYPROFILER_LIB easy_profiler)
		add_definitions(-DWITH_EASY_PROFILER)
		option(EASY_PROFILER_TO_FILE "Save the profile to a file" OFF)
		if (EASY_PROFILER_TO_FILE)
			add_definitions(-DWITH_EASY_PROFILER_TO_FILE)
		endif()
	endif()
endif()


set(PROJECT_SOURCES
	src/BlendedView.cpp
	src/Config.cpp
	src/Parameters.cpp
	src/Parser.cpp
	src/Pipeline.cpp
	src/SynthetizedView.cpp
	src/Timer.cpp
	src/View.cpp
	src/blending.cpp
	src/image_loading.cpp
	src/image_writing.cpp
	src/inpainting.cpp
	src/transform.cpp
	src/EquirectangularProjection.cpp
	src/Unprojector.cpp
	src/Projector.cpp
	src/PerspectiveUnprojector.cpp
	src/PerspectiveProjector.cpp
	src/PoseTraces.cpp)

set(PROJECT_HEADERS
	src/BlendedView.hpp
	src/Config.hpp
	src/Parser.hpp
	src/Pipeline.hpp
	src/SynthetizedView.hpp
	src/Timer.hpp
	src/View.hpp
	src/blending.hpp
	src/Parameters.hpp
	src/image_loading.hpp
	src/image_writing.hpp
	src/inpainting.hpp
	src/transform.hpp
	src/IntegralImage2D.h
	src/EquirectangularProjection.hpp
	src/Unprojector.hpp
	src/Projector.hpp
	src/PerspectiveUnprojector.hpp
	src/PerspectiveProjector.hpp
	src/PoseTraces.hpp)

set(CONFIGURATION_FILES
	config_files/example_config_file.cfg
	config_files/Unicorn_Triangles_Simple.cfg
	config_files/Unicorn_Triangles_Simple_ToErp.cfg
	config_files/Unicorn_Triangles_MultiSpectral.cfg
	config_files/Unicorn_10b.cfg
	config_files/Unicorn_Same_View.cfg
	config_files/ClassroomVideo-SVS-v0_to_v0.cfg
	config_files/ClassroomVideo-SVS-v7v8_to_v0.cfg
	config_files/ClassroomVideo-SVS-v7v8_to_v0_270deg.cfg
	config_files/TechnicoloMuseum-SVS-v0_to_v0.cfg
	config_files/TechnicoloMuseum-SVS-v0v2v13v17v19_to_v1.cfg
	config_files/TechnicoloMuseum-SVS-v5_to_v6.cfg
	config_files/TechnicolorHijack-SVS-v1v4_to_v9.cfg
	config_files/TechnicolorHijack-BlendByMax.cfg)
				 						 	 
source_group("Source" FILES ${PROJECT_SOURCES} src/view_synthesis.cpp src/test.cpp)
source_group("Headers" FILES ${PROJECT_HEADERS})
source_group("Configurations" FILES ${CONFIGURATION_FILES})

add_library(${PROJECT_NAME}Lib ${PROJECT_SOURCES} ${PROJECT_HEADERS})
add_executable(${PROJECT_NAME} src/view_synthesis.cpp extra/example)
add_executable(${PROJECT_NAME}UnitTest src/unit_test.cpp src/unit_test_spikes.cpp)
add_executable(${PROJECT_NAME}IntegrationTest src/integration_test.cpp ${CONFIGURATION_FILES})

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}Lib ${OpenCV_LIBS} ${CUDA_LIB} ${EASYPROFILER_LIB})
target_link_libraries(${PROJECT_NAME}UnitTest ${PROJECT_NAME}Lib ${OpenCV_LIBS} ${CUDA_LIB} ${EASYPROFILER_LIB})
target_link_libraries(${PROJECT_NAME}IntegrationTest ${PROJECT_NAME}Lib ${OpenCV_LIBS} ${CUDA_LIB} ${EASYPROFILER_LIB})

enable_testing()
add_test(NAME UnitTest${PROJECT_NAME} COMMAND ${PROJECT_NAME}UnitTest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME IntegrationTest${PROJECT_NAME} COMMAND ${PROJECT_NAME}IntegrationTest WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

