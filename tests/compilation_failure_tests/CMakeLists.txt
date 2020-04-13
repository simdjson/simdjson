#
# This directory contains files aimed to verify that constructs that
# are supposed to fail at compile time, indeed do so.
# To prevent bit rot, the same source file is compiled twice with
# the macro COMPILATION_TEST_USE_FAILING_CODE set to 0 or 1.
#

# internal function for add_dual_compile_test
# which adds a target and a test target trying to compile it
function(detail_add_dual_compile_test TEST_NAME TEST_FILE)
  add_executable(${TEST_NAME} ${TEST_FILE})
  set_target_properties(${TEST_NAME} PROPERTIES
                        EXCLUDE_FROM_ALL TRUE
                        EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  add_test(NAME ${TEST_NAME}
           COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME} --config $<CONFIGURATION>
           WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endfunction(add_compile_test)

# adds a compilation test. two targets are created, one expected to
# succeed compilation and one that is expected to fail.
function(add_dual_compile_test TEST_NAME TEST_FILE)
    detail_add_dual_compile_test(${TEST_NAME}_should_compile ${TEST_FILE})
    detail_add_dual_compile_test(${TEST_NAME}_should_not_compile ${TEST_FILE})
    target_compile_definitions(${TEST_NAME}_should_compile     PRIVATE COMPILATION_TEST_USE_FAILING_CODE=0)
    target_compile_definitions(${TEST_NAME}_should_not_compile PRIVATE COMPILATION_TEST_USE_FAILING_CODE=1)
    set_tests_properties(${TEST_NAME}_should_compile           PROPERTIES WILL_FAIL FALSE)
    set_tests_properties(${TEST_NAME}_should_not_compile       PROPERTIES WILL_FAIL TRUE)
endfunction(add_compile_test)


add_dual_compile_test(example_compiletest example_compiletest.cpp)
add_dual_compile_test(dangling_parser_load     dangling_parser_load.cpp)

