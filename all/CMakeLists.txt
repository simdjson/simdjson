cmake_minimum_required(VERSION 3.12)

project(aggregateForIDEs CXX)

set(root "${CMAKE_CURRENT_LIST_DIR}/..")
macro(add_rootdir DIR)
  add_subdirectory("${root}/${DIR}" ${ARGN})
endmacro()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message (STATUS "The simdjson repository appears to be standalone.")  
  option(SIMDJSON_JUST_LIBRARY "Build just the library, omit tests, tools and benchmarks" OFF)
  message (STATUS "By default, we attempt to build everything.")
else()
  message (STATUS "The simdjson repository appears to be used as a subdirectory.")
  option(SIMDJSON_JUST_LIBRARY "Build just the library, omit tests, tools and benchmarks" ON)
  message (STATUS "By default, we just build the library.")
endif()

if(SIMDJSON_JUST_LIBRARY)
  message(STATUS "Building just the library, omitting all tests, tools and benchmarks.")
else()
  # Setup tests
  enable_testing()
  add_rootdir(jsonchecker)
  add_rootdir(jsonexamples)
  add_library(test-data INTERFACE)
  target_link_libraries(test-data INTERFACE jsonchecker-data jsonchecker-minefield-data jsonexamples-data)
endif()

if(NOT SIMDJSON_JUST_LIBRARY)
  add_rootdir(tools)  ## This needs to be before tests because of cxxopts
  add_rootdir(singleheader)
endif()

if(NOT SIMDJSON_JUST_LIBRARY)
  add_rootdir(tests)
  add_rootdir(examples)
  add_rootdir(benchmark)
  add_rootdir(fuzz)
endif()

#
# Source files should be just ASCII
#
find_program(FIND find)
find_program(FILE file)
find_program(GREP grep)
if(FIND AND FILE AND GREP)
  add_test(NAME "just_ascii"
          COMMAND sh -c "${FIND} include src windows tools singleheader tests examples benchmark -path benchmark/checkperf-reference -prune -name '*.h' -o -name '*.cpp' -type f  -exec ${FILE} '{}' \; | ${GREP} -v ASCII || exit 0 && exit 1"
          WORKING_DIRECTORY "${root}")
endif()
