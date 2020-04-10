# Helper so we don't have to repeat ourselves so much
function(add_cpp_test TEST_NAME TEST_FILE)
  # If a source file is passed, add an executable
  add_executable(${TEST_NAME} ${TEST_FILE})
  add_test(${TEST_NAME} ${TEST_NAME})
  if ($ARGN)
    set_property(TEST ${TEST_NAME} APPEND PROPERTY LABELS ${ARGN})
  else()
    set_property(TEST ${TEST_NAME} APPEND PROPERTY LABELS slowtests)
  endif()
endfunction()

function(add_compile_test TEST_NAME TEST_FILE)
  add_executable(${TEST_NAME} ${TEST_FILE})
  set_target_properties(${TEST_NAME} PROPERTIES
                        EXCLUDE_FROM_ALL TRUE
                        EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  add_test(
    NAME ${TEST_NAME}
    COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME} --config $<CONFIGURATION>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  if (${ARGN}) # Labels
    set_property(TEST ${TEST_NAME} APPEND PROPERTY LABELS ${ARGN})
  else()
    set_property(TEST ${TEST_NAME} APPEND PROPERTY LABELS slowtests)
  endif()
endfunction(add_compile_test)

# Most tests need test data, and many need windows headers.
link_libraries(simdjson-flags test-data simdjson-windows-headers)

#
# These tests explicitly do #include "simdjson.cpp" so they can override stuff
#
if (NOT MSVC) # Can't get simdjson-source to compile on Windows for some reason.
  add_cpp_test(numberparsingcheck numberparsingcheck.cpp quicktests)
  target_link_libraries(numberparsingcheck simdjson-include-source)
  add_cpp_test(stringparsingcheck stringparsingcheck.cpp quicktests)
  target_link_libraries(stringparsingcheck simdjson-include-source)
endif()

# All remaining tests link with simdjson proper
link_libraries(simdjson)
add_cpp_test(basictests basictests.cpp quicktests)
add_cpp_test(errortests errortests.cpp quicktests)
add_cpp_test(integer_tests integer_tests.cpp quicktests)
add_cpp_test(jsoncheck jsoncheck.cpp quicktests)
add_cpp_test(parse_many_test parse_many_test.cpp quicktests)
add_cpp_test(pointercheck pointercheck.cpp quicktests)
add_cpp_test(extracting_values_example extracting_values_example.cpp quicktests)

# Script tests
if (NOT MSVC) # Can't run .sh on windows
  #
  # json2json test
  #
  add_test(
    NAME testjson2json
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/testjson2json.sh
    WORKING_DIRECTORY $<TARGET_FILE_DIR:minify>
  )
  set_property(TEST testjson2json APPEND PROPERTY DEPENDS minify json2json)
  set_property(TEST testjson2json APPEND PROPERTY LABELS slowtests)

  #
  # Competition parse test
  #
  if (SIMDJSON_COMPETITION)
    add_executable(allparserscheckfile allparserscheckfile.cpp)
    target_link_libraries(allparserscheckfile competition-all)

    add_test(issue150 ${CMAKE_CURRENT_SOURCE_DIR}/issue150.sh)
    set_property(TEST issue150 APPEND PROPERTY DEPENDS allparserscheckfile)
    set_property(TEST issue150 APPEND PROPERTY LABELS slowtests)
  endif()
endif()

#
# Compile-only tests with simdjson flags on
#

# Don't add the tests if we're on VS2017 or older; they don't succeed.
if(NOT (MSVC AND MSVC_VERSION LESS 1920))
  if(SIMDJSON_EXCEPTIONS)
    add_compile_test(readme_examples readme_examples.cpp quicktests)
    set_property(
      TEST readme_examples
      APPEND PROPERTY LABELS quicktests
    )
  endif()
  add_compile_test(readme_examples_noexceptions readme_examples_noexceptions.cpp quicktests)

  add_compile_test(readme_examples11 readme_examples.cpp quicktests)
  set_target_properties(readme_examples11 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  if (!MSVC)
    target_compile_options(readme_examples11 PRIVATE -Werror)
  endif()

  add_compile_test(readme_examples_noexceptions11 readme_examples_noexceptions.cpp quicktests)
  set_target_properties(readme_examples_noexceptions11 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  if (!MSVC)
    target_compile_options(readme_examples11 PRIVATE -Werror)
  endif()

  # Compile tests that *should fail*
  add_compile_test(readme_examples_will_fail_with_exceptions_off readme_examples.cpp quicktests)
  target_compile_definitions(readme_examples_will_fail_with_exceptions_off PRIVATE SIMDJSON_EXCEPTIONS=0)
  set_tests_properties(readme_examples_will_fail_with_exceptions_off PROPERTIES WILL_FAIL TRUE)
  set_property(
    TEST readme_examples_noexceptions readme_examples_will_fail_with_exceptions_off
    APPEND PROPERTY LABELS quicktests
  )

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

add_subdirectory(compilation_failure_tests)
