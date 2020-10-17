# Helper so we don't have to repeat ourselves so much
# Usage: add_cpp_test(testname [COMPILE_ONLY] [SOURCES a.cpp b.cpp ...] [LABELS acceptance per_implementation ...])
# SOURCES defaults to testname.cpp if not specified.
function(add_cpp_test TEST_NAME)
  # Parse arguments
  cmake_parse_arguments(PARSE_ARGV 1 ARGS "COMPILE_ONLY;WILL_FAIL" "" "SOURCES;LABELS")
  if (NOT ARGS_SOURCES)
    list(APPEND ARGS_SOURCES ${TEST_NAME}.cpp)
  endif()
  if (COMPILE_ONLY)
    list(APPEND ${ARGS_LABELS} compile)
  endif()

  # Add executable
  add_executable(${TEST_NAME} ${ARGS_SOURCES})

  # Add test
  if (ARGS_COMPILE_ONLY)
    add_test(
      NAME ${TEST_NAME}
      COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME} --config $<CONFIGURATION>
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    set_target_properties(${TEST_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  else()
    add_test(${TEST_NAME} ${TEST_NAME})
  endif()

  if (ARGS_LABELS)
    set_property(TEST ${TEST_NAME} APPEND PROPERTY LABELS ${ARGS_LABELS})
  endif()

  if (ARGS_WILL_FAIL)
    set_property(TEST ${TEST_NAME} PROPERTY WILL_FAIL TRUE)
  endif()
endfunction()

# Most tests need test data, and many need windows headers.
link_libraries(simdjson-internal-flags test-data simdjson-windows-headers)

include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)

add_subdirectory(ondemand)

#
# These tests explicitly do #include "simdjson.cpp" so they can override stuff
#
add_cpp_test(numberparsingcheck LABELS acceptance per_implementation)
target_link_libraries(numberparsingcheck simdjson-include-source simdjson-windows-headers)
target_compile_definitions(numberparsingcheck PRIVATE NOMINMAX)
add_cpp_test(stringparsingcheck LABELS acceptance per_implementation)
target_link_libraries(stringparsingcheck simdjson-include-source simdjson-windows-headers)
target_compile_definitions(stringparsingcheck PRIVATE NOMINMAX)

# All remaining tests link with simdjson proper
link_libraries(simdjson)
add_cpp_test(random_string_number_tests LABELS acceptance per_implementation)
add_cpp_test(basictests LABELS acceptance per_implementation)
add_cpp_test(minify_tests LABELS acceptance per_implementation)
add_cpp_test(document_stream_tests LABELS acceptance per_implementation)
add_cpp_test(document_tests LABELS acceptance per_implementation)
add_cpp_test(errortests LABELS acceptance per_implementation)
add_cpp_test(integer_tests LABELS acceptance per_implementation)
add_cpp_test(jsoncheck LABELS acceptance per_implementation)
add_cpp_test(minefieldcheck LABELS acceptance per_implementation)
add_cpp_test(parse_many_test LABELS acceptance per_implementation)
add_cpp_test(pointercheck LABELS acceptance per_implementation) # https://tools.ietf.org/html/rfc6901
add_cpp_test(extracting_values_example LABELS acceptance per_implementation)
add_cpp_test(unicode_tests LABELS acceptance per_implementation)
add_cpp_test(padded_string_tests)

find_program(BASH bash)

# Below we skip anything on Windows, not just visual studio, because running bash under Windows requires you to 
# map app paths to their "mounted" equivalent (e.g., /mnt/c/...). So even if you have bash under Windows, extra work would be
# required to make things work robustly. Simply put: bash is not quite portable.

# Script tests
if (BASH AND (NOT WIN32) AND SIMDJSON_BASH AND (TARGET json2json)) # The scripts are not robust enough to run under Windows even if bash is available
  #
  # json2json test
  #
  add_test(
    NAME testjson2json
    COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/testjson2json.sh
    WORKING_DIRECTORY $<TARGET_FILE_DIR:minify>
  )
  set_property(TEST testjson2json APPEND PROPERTY DEPENDS minify json2json)
  set_property(TEST testjson2json APPEND PROPERTY LABELS per_implementation)

  #
  # Competition parse test
  #
  if ((SIMDJSON_COMPETITION) AND (!SIMDJSON_SANITIZE))
    # It looks like RapidJSON does not pass the sanitizer under some conditions (Clang 10)
    add_executable(allparserscheckfile allparserscheckfile.cpp)
    target_link_libraries(allparserscheckfile competition-all)

    add_test(issue150 ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/issue150.sh)
    set_property(TEST issue150 APPEND PROPERTY DEPENDS allparserscheckfile)
    set_property(TEST issue150 APPEND PROPERTY LABELS per_implementation)
  endif()

  #
  # json2json tool test: check that json2json can parse twitter.json
  #

  # This tests validates that the implementation is what we think it is if we get passed
  # SIMDJSON_FORCE_IMPLEMENTATION, so we know we're testing what we think we're testing
  add_cpp_test(checkimplementation LABELS per_implementation)

  add_test(NAME json2json COMMAND $<TARGET_FILE:json2json> ${EXAMPLE_JSON})
  set_property(TEST json2json APPEND PROPERTY LABELS acceptance per_implementation)

  #
  # SIMDJSON_FORCE_IMPLEMENTATION tests: run json2json with SIMDJSON
  #
  if (SIMDJSON_IMPLEMENTATION MATCHES "fallback")
    add_test(
      NAME simdjson_force_implementation
      COMMAND
        ${CMAKE_COMMAND} -E env
        SIMDJSON_FORCE_IMPLEMENTATION=fallback
        $<TARGET_FILE:checkimplementation>
    )
  endif()
  add_test(
    NAME simdjson_force_implementation_error
    COMMAND
      ${CMAKE_COMMAND} -E env
      SIMDJSON_FORCE_IMPLEMENTATION=doesnotexist
      $<TARGET_FILE:json2json> ${EXAMPLE_JSON}
  )
  set_tests_properties(simdjson_force_implementation_error PROPERTIES WILL_FAIL TRUE)
endif()

#
# Compile-only tests with simdjson flags on
#

# Don't add the tests if we're on VS2017 or older; they don't succeed.
if(NOT (MSVC AND MSVC_VERSION LESS 1920))
  if(SIMDJSON_EXCEPTIONS)
    add_cpp_test(readme_examples COMPILE_ONLY LABELS acceptance)
    add_cpp_test(readme_examples11 COMPILE_ONLY LABELS acceptance SOURCES readme_examples.cpp)
    set_target_properties(readme_examples11 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  endif()

  add_cpp_test(readme_examples_noexceptions COMPILE_ONLY LABELS acceptance)

  add_cpp_test(readme_examples_noexceptions11 COMPILE_ONLY LABELS acceptance SOURCES readme_examples_noexceptions.cpp)
  set_target_properties(readme_examples_noexceptions11 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

  # Compile tests that *should fail*
  add_cpp_test(readme_examples_will_fail_with_exceptions_off WILL_FAIL COMPILE_ONLY LABELS acceptance SOURCES readme_examples.cpp)
  target_compile_definitions(readme_examples_will_fail_with_exceptions_off PRIVATE SIMDJSON_EXCEPTIONS=0)
endif()


if(MSVC)
  add_custom_command(TARGET basictests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:simdjson>"
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE_DIR:basictests>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:basictests>")                 # <--this is out-file path
endif()

# Copy the simdjson dll into the tests directory
if(MSVC)
  add_custom_command(TARGET basictests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:basictests>")                 # <--this is out-file path
endif()

## Next bit should not be needed!
#if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
# next line is a workaround for an odr-violation in basictests regarding the globals 0x432a40 and 0x52045c under clang
#set_tests_properties(basictests PROPERTIES
#    ENVIRONMENT ASAN_OPTIONS="detect_odr_violation=0")
#endif()

## This causes problems
# add_executable(singleheader ./singleheadertest.cpp ${PROJECT_SOURCE_DIR}/singleheader/simdjson.cpp)
# target_link_libraries(singleheader simdjson simdjson-flags)
# add_test(singleheader singleheader)

if(NOT MSVC)
  ######
  # This tests is to guard us against ever again removing the soversion
  # number from the library. See Bug
  # https://github.com/simdjson/simdjson/issues/1014
  #####
  get_target_property(REPORTED_SIMDJSON_SOVERSION simdjson SOVERSION)
  if(NOT ${REPORTED_SIMDJSON_SOVERSION} STREQUAL ${SIMDJSON_LIB_SOVERSION})
    message(FATAL_ERROR "The library target does not have the proper soversion information." )
  endif()
  get_target_property(REPORTED_SIMDJSON_VERSION simdjson VERSION)
  if(NOT ${REPORTED_SIMDJSON_VERSION} STREQUAL ${SIMDJSON_LIB_VERSION})
    message(FATAL_ERROR "The library target does not have the proper version information." )
  endif()
endif()

add_subdirectory(compilation_failure_tests)
