# Helper so we don't have to repeat ourselves so much
# Usage: add_cpp_test(testname [COMPILE_ONLY] [SOURCES a.cpp b.cpp ...] [LABELS acceptance per_implementation ...])
# SOURCES defaults to testname.cpp if not specified.
function(add_cpp_test TEST_NAME)
  # Parse arguments
  cmake_parse_arguments(PARSE_ARGV 1 ARGS "COMPILE_ONLY;LIBRARY;WILL_FAIL" "" "SOURCES;LABELS")
  if (NOT ARGS_SOURCES)
    list(APPEND ARGS_SOURCES ${TEST_NAME}.cpp)
  endif()
  if (ARGS_COMPILE_ONLY)
    list(APPEND ${ARGS_LABELS} compile)
  endif()

  # Add the compile target
  if (ARGS_LIBRARY)
    add_library(${TEST_NAME} STATIC ${ARGS_SOURCES})
  else(ARGS_LIBRARY)
    add_executable(${TEST_NAME} ${ARGS_SOURCES})
  endif(ARGS_LIBRARY)

  # Add test
  if (ARGS_COMPILE_ONLY OR ARGS_LIBRARY)
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

function(add_compile_only_test TEST_NAME)
  add_test(
    NAME ${TEST_NAME}
    COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME} --config $<CONFIGURATION>
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )
  set_target_properties(${TEST_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
endfunction()