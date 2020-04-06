# Helper so we don't have to repeat ourselves so much
function(add_cpp_test TEST_NAME TEST_FILE)
  add_executable(${TEST_NAME} ${TEST_FILE})
  add_test(${TEST_NAME} ${TEST_NAME})
endfunction(add_cpp_test)

# Sets a target to only build when you run the test, and expect failure
function(add_compile_fail_test TEST_NAME)
  set_target_properties(${TEST_NAME} PROPERTIES
                        EXCLUDE_FROM_ALL TRUE
                        EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  add_test(NAME ${TEST_NAME}
          COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME} --config $<CONFIGURATION>
          WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_tests_properties(${TEST_NAME} PROPERTIES WILL_FAIL TRUE)
endfunction(add_compile_fail_test)

#
# These test explicitly do #include "simdjson.cpp"
#
add_cpp_test(numberparsingcheck numberparsingcheck.cpp)
target_link_libraries(numberparsingcheck simdjson-source)
add_cpp_test(stringparsingcheck stringparsingcheck.cpp)
target_link_libraries(stringparsingcheck simdjson-source)

#
# All remaining tests link with simdjson proper
#
link_libraries(simdjson)

# add_executable(allparserscheckfile allparserscheckfile.cpp)
add_cpp_test(basictests basictests.cpp)
add_cpp_test(errortests errortests.cpp)
add_cpp_test(integer_tests integer_tests.cpp)
add_cpp_test(jsoncheck jsoncheck.cpp)
add_cpp_test(parse_many_test parse_many_test.cpp)
add_cpp_test(pointercheck pointercheck.cpp)

# Compile-only tests
add_executable(readme_examples readme_examples.cpp)
add_executable(readme_examples_noexceptions readme_examples_noexceptions.cpp)
target_compile_options(readme_examples_noexceptions PRIVATE -fno-exceptions)

# Test that readme_examples does NOT compile when SIMDJSON_EXCEPTIONS=0 (i.e. test that the define
# works even if exceptions are on)
add_executable(readme_examples_will_fail_with_exceptions_off readme_examples.cpp)
target_compile_definitions(readme_examples_will_fail_with_exceptions_off PRIVATE SIMDJSON_EXCEPTIONS=0)
add_compile_fail_test(readme_examples_will_fail_with_exceptions_off)

if(MSVC)
  include_directories($<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/windows>)
  add_custom_command(TARGET basictests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:simdjson>"
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE_DIR:basictests>"
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
# target_link_libraries(singleheader simdjson)
# add_test(singleheader singleheader)
