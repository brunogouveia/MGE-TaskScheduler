cmake_minimum_required(VERSION 2.8)

PROJECT(MGETaskScheduler)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if ( CMAKE_BUILD_TYPE STREQUAL "Release" )
	add_definitions( -O3 )
endif ( CMAKE_BUILD_TYPE STREQUAL "Release" )

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

SET(HDRS 
		include/Fiber.h
		include/FiberContext.h
		include/TaskCounter.h
		include/TaskList.h
		include/TaskScheduler.h
		include/ThreadContext.h
)

SET(SRCS 
		src/Main.cpp
		src/Fiber.cpp
		src/FiberContext.cpp
		src/TaskCounter.cpp
		src/TaskList.cpp
		src/TaskScheduler.cpp
		src/ThreadContext.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(Sample ${SRCS} ${HDRS})