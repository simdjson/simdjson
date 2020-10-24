cmake_minimum_required(VERSION 3.12)

project(simdjson
        VERSION 0.6.0
        DESCRIPTION "Parsing gigabytes of JSON per second"
        HOMEPAGE_URL "https://github.com/simdjson/simdjson"
        LANGUAGES CXX C)

set(SIMDJSON_SEMANTIC_VERSION "${PROJECT_VERSION}" CACHE STRING "simdjson semantic version")
set(SIMDJSON_LIB_VERSION "4.0.0" CACHE STRING "simdjson library version")
set(SIMDJSON_LIB_SOVERSION "4" CACHE STRING "simdjson library soversion")

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(windows)

include(cmake/implementations.cmake)
include(cmake/options.cmake)

include(cmake/cpack.cmake)
