cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
set(PROJECT_NAME "SVS")
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


include_directories(
				    #vendors/.../include/
					)

file(GLOB VENDORS_SOURCES
					#vendors/.../src/
				    )
file(GLOB_RECURSE PROJECT_SOURCES src/*.hpp
                          src/*.h
						  src/*.cpp) 

						  
#file(GLOB PROJECT_CONFIGS CMakeLists.txt
#                          Readme.md
#                         .gitattributes
#                         .gitignore
#                         .gitmodules)
						 
						 	 
source_group("Source" FILES ${PROJECT_SOURCES})
source_group("Vendors" FILES ${VENDORS_SOURCES})
#source_group("Configs" FILES ${PROJECT_CONFIGS})

add_definitions(
                -DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES}
                               ${VENDORS_SOURCES}
                               ${PROJECT_CONFIGS})
target_link_libraries(${PROJECT_NAME}
					  ${OpenCV_LIBS}
					  ${CUDA_LIB}
					  ${EASYPROFILER_LIB}
                      )
					  
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

	
file(COPY config_files DESTINATION ${CMAKE_BINARY_DIR}/)

