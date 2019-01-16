cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH OFF)
if (NOT CMAKE_BUILD_TYPE)
                message(STATUS "No build type selected, default to Release")
                set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

project(simdjson)
set(SIMDJSON_LIB_NAME simdjson)

if(NOT MSVC)
option(SIMDJSON_BUILD_STATIC "Build a static library" OFF) # turning it on disables the production of a dynamic library
else()
option(SIMDJSON_BUILD_STATIC "Build a static library" ON) # turning it on disables the production of a dynamic library
endif()
option(SIMDJSON_BUILD_LTO "Build library with Link Time Optimization" OFF)
option(SIMDJSON_SANITIZE "Sanitize addresses" OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake")

find_package(CTargets)
find_package(Options)
find_package(LTO)

install(DIRECTORY include/${SIMDJSON_LIB_NAME} DESTINATION include)
set (TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jsonchecker/")
set (BENCHMARK_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jsonexamples/")
add_definitions(-DSIMDJSON_TEST_DATA_DIR="${TEST_DATA_DIR}")
add_definitions(-DSIMDJSON_BENCHMARK_DATA_DIR="${TEST_DATA_DIR}")
enable_testing()
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(tests)
add_subdirectory(benchmark)
