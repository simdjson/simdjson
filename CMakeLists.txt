cmake_minimum_required(VERSION 3.9) # CMP0069 NEW
project(simdjson
  DESCRIPTION "Parsing gigabytes of JSON per second"
  LANGUAGES CXX C
)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 3)
set(PROJECT_VERSION_PATCH 1)
set(SIMDJSON_LIB_VERSION "0.3.1" CACHE STRING "simdjson library version")
set(SIMDJSON_LIB_SOVERSION "1" CACHE STRING "simdjson library soversion")
set(SIMDJSON_GITHUB_REPOSITORY https://github.com/simdjson/simdjson)

#
# simdjson flags and global options
#
include(simdjson-flags.cmake)

#
# ${SIMDJSON_USER_CMAKECACHE} contains the *user-specified* simdjson options so you can call cmake on
# another branch or repository with the same options.
#
# Not supported on Windows at present, because the only thing that uses it is checkperf, which we
# don't run on Windows.
#
if (NOT MSVC)
  set(SIMDJSON_USER_CMAKECACHE ${CMAKE_CURRENT_BINARY_DIR}/.simdjson-user-CMakeCache.txt)
  add_custom_command(
    OUTPUT ${SIMDJSON_USER_CMAKECACHE}
    COMMAND bash -c "grep SIMDJSON_ ${PROJECT_BINARY_DIR}/CMakeCache.txt | grep -v SIMDJSON_LIB_ > ${SIMDJSON_USER_CMAKECACHE}"
    VERBATIM # Makes it not do weird escaping with the command
  )
  add_custom_target(simdjson-user-cmakecache ALL DEPENDS ${SIMDJSON_USER_CMAKECACHE})
endif()

#
# simdjson core library and tools

#
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(windows)
add_subdirectory(tools)

#
# Compile tools / tests / benchmarks
#
enable_testing()

add_library(test-data INTERFACE)
target_compile_definitions(test-data INTERFACE SIMDJSON_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/jsonchecker/")
target_compile_definitions(test-data INTERFACE SIMDJSON_BENCHMARK_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/jsonexamples/")
set(EXAMPLE_JSON ${CMAKE_CURRENT_SOURCE_DIR}/jsonexamples/twitter.json)

add_subdirectory(dependencies)
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(benchmark)

# for fuzzing, read the comments in the fuzz/CMakeLists.txt file
option(ENABLE_FUZZING "enable building the fuzzers" ON)
if(ENABLE_FUZZING AND SIMDJSON_EXCEPTIONS)
  add_subdirectory(fuzz)
endif()

#
# CPack
#
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
