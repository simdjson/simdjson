# Helper so we don't have to repeat ourselves so much
# Usage: add_cpp_test(testname [COMPILE_ONLY] [SOURCES a.cpp b.cpp ...] [LABELS acceptance per_implementation ...])
# SOURCES defaults to testname.cpp if not specified.
function(add_cpp_test TEST_NAME)
  # Parse arguments
  cmake_parse_arguments(PARSE_ARGV 1 ARGS "COMPILE_ONLY;LIBRARY;WILL_FAIL" "" "SOURCES;LABELS;DEPENDENCY_OF")
  if (NOT ARGS_SOURCES)
    list(APPEND ARGS_SOURCES ${TEST_NAME}.cpp)
  endif()
  if (ARGS_COMPILE_ONLY)
    list(APPEND ${ARGS_LABELS} compile_only)
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

    # Add to <label>_tests make targets
    foreach(label ${ARGS_LABELS})
      list(APPEND ARGS_DEPENDENCY_OF ${label})
    endforeach(label ${ARGS_LABELS})
  endif()

  # Add to test labels
  if (ARGS_LABELS)
    set_property(TEST ${TEST_NAME} APPEND PROPERTY LABELS ${ARGS_LABELS})
  endif()

  # Add as a dependency of given targets
  foreach(dependency_of ${ARGS_DEPENDENCY_OF})
    if (NOT TARGET ${dependency_of}_tests)
      add_custom_target(${dependency_of}_tests)
      add_dependencies(all_tests ${dependency_of}_tests)
    endif(NOT TARGET ${dependency_of}_tests)
    add_dependencies(${dependency_of}_tests ${TEST_NAME})
  endforeach(dependency_of ${ARGS_DEPENDENCY_OF})

  # If it will fail, mark the test as such
  if (ARGS_WILL_FAIL)
    set_property(TEST ${TEST_NAME} PROPERTY WILL_FAIL TRUE)
  endif()
endfunction()
