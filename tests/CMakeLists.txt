# Helper so we don't have to repeat ourselves so much
function(add_cpp_test TEST_NAME TEST_FILE)
  add_executable(${TEST_NAME} ${TEST_FILE})
  add_test(${TEST_NAME} ${TEST_NAME})
  target_link_libraries(${TEST_NAME} test-data)
endfunction(add_cpp_test)

#
# These test explicitly do #include "simdjson.cpp"
#
if (NOT MSVC) # Can't get simdjson-source to compile on Windows for some reason.
  add_cpp_test(numberparsingcheck numberparsingcheck.cpp)
  target_link_libraries(numberparsingcheck simdjson-source test-data simdjson-windows-headers)
  add_cpp_test(stringparsingcheck stringparsingcheck.cpp)
  target_link_libraries(stringparsingcheck simdjson-source test-data simdjson-windows-headers)
endif()

#
# All remaining tests link with simdjson proper
#
link_libraries(simdjson)

add_cpp_test(basictests basictests.cpp)
add_cpp_test(errortests errortests.cpp)
add_cpp_test(integer_tests integer_tests.cpp)
add_cpp_test(jsoncheck jsoncheck.cpp)
target_link_libraries(jsoncheck simdjson-windows-headers)
add_cpp_test(parse_many_test parse_many_test.cpp)
target_link_libraries(parse_many_test simdjson-windows-headers)
add_cpp_test(pointercheck pointercheck.cpp)

#
# Competition parse test
#
add_executable(allparserscheckfile allparserscheckfile.cpp)
target_link_libraries(allparserscheckfile competition-all)

#
# Compile-only tests
#
function(add_compile_test TEST_NAME TEST_FILE EXPECT_SUCCESS)
  add_executable(${TEST_NAME} ${TEST_FILE})
  set_target_properties(${TEST_NAME} PROPERTIES
                        EXCLUDE_FROM_ALL TRUE
                        EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  add_test(NAME ${TEST_NAME}
           COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME} --config $<CONFIGURATION>
           WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  if(NOT ${EXPECT_SUCCESS})
    set_tests_properties(${TEST_NAME} PROPERTIES WILL_FAIL TRUE)
  endif()
endfunction(add_compile_test)

# Don't add the tests if we're on VS2017 or older; they don't succeed.
if(NOT (MSVC AND MSVC_VERSION LESS 1920))
  add_compile_test(readme_examples readme_examples.cpp TRUE)
  # Test that readme_examples does NOT compile when SIMDJSON_EXCEPTIONS=0 (i.e. test that the define
  # works even if exceptions are on)
  add_compile_test(readme_examples_will_fail_with_exceptions_off readme_examples.cpp FALSE)
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
