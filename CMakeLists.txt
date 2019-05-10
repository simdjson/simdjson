cmake_minimum_required(VERSION 3.9) # CMP0069 NEW
include(CheckIPOSupported)
check_ipo_supported(RESULT ltoresult)
if(ltoresult)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH OFF)
if (NOT CMAKE_BUILD_TYPE)
                message(STATUS "No build type selected, default to Release")
                set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

project(simdjson)
set(SIMDJSON_LIB_NAME simdjson)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 2)
set(SIMDJSON_LIB_VERSION "0.1.2" CACHE STRING "simdjson library version")
set(SIMDJSON_LIB_SOVERSION "0" CACHE STRING "simdjson library soversion")

if(NOT MSVC)
option(SIMDJSON_BUILD_STATIC "Build a static library" OFF) # turning it on disables the production of a dynamic library
else()
option(SIMDJSON_BUILD_STATIC "Build a static library" ON) # turning it on disables the production of a dynamic library
endif()
option(SIMDJSON_SANITIZE "Sanitize addresses" OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake")

find_package(CTargets)
find_package(Options)

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
