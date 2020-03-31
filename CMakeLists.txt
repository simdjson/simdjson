cmake_minimum_required(VERSION 3.9) # CMP0069 NEW

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

project(simdjson
  DESCRIPTION "Parsing gigabytes of JSON per second"
  LANGUAGES CXX
)

# LTO seems to create all sorts of fun problems. Let us
# disable temporarily.
#include(CheckIPOSupported)
#check_ipo_supported(RESULT ltoresult)
#if(ltoresult)
#  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH OFF)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(SIMDJSON_LIB_NAME simdjson)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 3)
set(PROJECT_VERSION_PATCH 0)
set(SIMDJSON_LIB_VERSION "0.3.0" CACHE STRING "simdjson library version")
set(SIMDJSON_LIB_SOVERSION "0" CACHE STRING "simdjson library soversion")

option(SIMDJSON_IMPLEMENTATION_HASWELL "Include the haswell implementation" ON)
option(SIMDJSON_IMPLEMENTATION_WESTMERE "Include the westmere implementation" ON)
option(SIMDJSON_IMPLEMENTATION_ARM64 "Include the arm64 implementation" ON)
option(SIMDJSON_IMPLEMENTATION_FALLBACK "Include the fallback implementation" ON)
if(NOT MSVC)
  option(SIMDJSON_BUILD_STATIC "Build a static library" OFF) # turning it on disables the production of a dynamic library
else()
  option(SIMDJSON_BUILD_STATIC "Build a static library" ON) # turning it on disables the production of a dynamic library
endif()
option(SIMDJSON_SANITIZE "Sanitize addresses" OFF)
option(SIMDJSON_GOOGLE_BENCHMARKS "compile the Google Benchmark benchmarks" OFF)
option(SIMDJSON_ENABLE_THREADS "enable threaded operation" ON)
option(SIMDJSON_EXCEPTIONS "Enable simdjson's exception-throwing interface" ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake")

find_package(CTargets)
find_package(Options)

install(DIRECTORY include/${SIMDJSON_LIB_NAME} DESTINATION include)
set (TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jsonchecker/")
set (BENCHMARK_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jsonexamples/")
add_definitions(-DSIMDJSON_TEST_DATA_DIR="${TEST_DATA_DIR}")
add_definitions(-DSIMDJSON_BENCHMARK_DATA_DIR="${TEST_DATA_DIR}")
enable_testing()
include(include/CMakeLists.txt)
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(tests)
add_subdirectory(benchmark)

if (SIMDJSON_GOOGLE_BENCHMARKS)
  if(NOT EXISTS dependencies/benchmark/CMakeLists.txt)
    # message(STATUS "Unable to find dependencies/benchmark/CMakeLists.txt")
    execute_process(COMMAND git submodule update --init -- dependencies/benchmark
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  option(BENCHMARK_ENABLE_TESTING OFF)
  set(BENCHMARK_ENABLE_TESTING OFF)
  option(BENCHMARK_ENABLE_INSTALL OFF)
  set(BENCHMARK_ENABLE_INSTALL OFF)
  add_subdirectory(dependencies/benchmark)
endif()

# for fuzzing, read the comments in the fuzz/CMakeLists.txt file
option(ENABLE_FUZZING "enable building the fuzzers" ON)
if(ENABLE_FUZZING)
  add_subdirectory(fuzz)
endif()

set(CPACK_PACKAGE_VENDOR "Daniel Lemire")
set(CPACK_PACKAGE_CONTACT "lemire@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Parsing gigabytes of JSON per second")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_RPM_PACKAGE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")

include(CPack)


