# Most tests need test data, and many need windows headers.
link_libraries(simdjson-internal-flags test-data simdjson-windows-headers)
include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)
add_subdirectory(dom)
add_subdirectory(ondemand)


# All remaining tests link with simdjson proper
link_libraries(simdjson)

add_cpp_test(unicode_tests       LABELS dom acceptance per_implementation)
add_cpp_test(minify_tests        LABELS other acceptance per_implementation)
add_cpp_test(padded_string_tests LABELS other acceptance                   )
add_cpp_test(prettify_tests      LABELS other acceptance per_implementation)

if(MSVC AND BUILD_SHARED_LIBS)
  # Copy the simdjson dll into the tests directory
  add_custom_command(TARGET unicode_tests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:unicode_tests>")                 # <--this is out-file path
endif()

# This tests validates that the implementation is what we think it is if we get passed
# SIMDJSON_FORCE_IMPLEMENTATION, so we know we're testing what we think we're testing
add_cpp_test(checkimplementation LABELS other per_implementation)

add_subdirectory(compilation_failure_tests)
