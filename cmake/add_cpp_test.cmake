# Helper so we don't have to repeat ourselves so much
# Usage: add_cpp_test(<testname>
#                     [COMPILE_ONLY]
#                     [DEBUG_ONLY]
#                     [WILL_FAIL]
#                     [LIBRARY]
#                     [SOURCES a.cpp b.cpp ...]
#                     [LABELS acceptance per_implementation ...]
#                     [DEPENDENCY_OF ...])
# SOURCES defaults to <testname>.cpp if not specified.
function(add_cpp_test TEST_NAME)
  # Parse arguments
  cmake_parse_arguments(PARSE_ARGV 1 ARGS "COMPILE_ONLY;LIBRARY;WILL_FAIL;DEBUG_ONLY" "" "SOURCES;LABELS;DEPENDENCY_OF")

  # Is generator multi-config?
  get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)

  # Return if debug only test
  if(NOT is_multi_config AND ARGS_DEBUG_ONLY AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    return()
  endif()

  # Set configurations
  set(config_args)
  if(is_multi_config AND ARGS_DEBUG_ONLY)
    set(config_args CONFIGURATIONS Debug)
  endif()

  if(NOT ARGS_SOURCES)
    list(APPEND ARGS_SOURCES ${TEST_NAME}.cpp)
  endif()

  # Wrap sources in a generator expression and provide a dummy source
  if(is_multi_config AND ARGS_DEBUG_ONLY)
    list(TRANSFORM ARGS_SOURCES PREPEND "$<$<CONFIG:Debug>:")
    list(TRANSFORM ARGS_SOURCES APPEND ">")
    list(PREPEND ARGS_SOURCES "$<$<NOT:$<CONFIG:Debug>>:${simdjson_SOURCE_DIR}/cmake/empty.cpp>")
  endif()

  if(ARGS_COMPILE_ONLY)
    list(APPEND ARGS_LABELS compile)
  endif()

  # Add the compile target
  if(ARGS_LIBRARY)
    add_library(${TEST_NAME} STATIC ${ARGS_SOURCES})
  else()
    add_executable(${TEST_NAME} ${ARGS_SOURCES})
  endif()

  # Add test
  if(ARGS_COMPILE_ONLY OR ARGS_LIBRARY)
    add_test(
      NAME ${TEST_NAME}
      COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME} --config $<CONFIGURATION>
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      ${config_args}
    )
    set_target_properties(${TEST_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  else()
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} ${config_args})

    # Add to <label>_tests make targets
    foreach(label IN LISTS ARGS_LABELS)
      list(APPEND ARGS_DEPENDENCY_OF ${label})
    endforeach()
  endif()

  # Add to test labels
  if (ARGS_LABELS)
    set_property(TEST ${TEST_NAME} APPEND PROPERTY LABELS ${ARGS_LABELS})
  endif()

  # Add as a dependency of given targets
  foreach(dependency_of IN LISTS ARGS_DEPENDENCY_OF)
    if (NOT TARGET ${dependency_of}_tests)
      add_custom_target(${dependency_of}_tests)
      add_dependencies(all_tests ${dependency_of}_tests)
    endif()
    add_dependencies(${dependency_of}_tests ${TEST_NAME})
  endforeach()

  # If it will fail, mark the test as such
  if (ARGS_WILL_FAIL)
    set_property(TEST ${TEST_NAME} PROPERTY WILL_FAIL TRUE)
  endif()
endfunction()
