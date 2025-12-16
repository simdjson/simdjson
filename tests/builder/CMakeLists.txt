# All remaining tests link with simdjson proper
include_directories(..)
add_cpp_test(builder_string_builder_tests                LABELS ondemand acceptance per_implementation)
if(SIMDJSON_STATIC_REFLECTION)
  add_cpp_test(static_reflection_custom_tests            LABELS ondemand acceptance per_implementation)
  add_cpp_test(static_reflection_builder_tests           LABELS ondemand acceptance per_implementation)
  add_cpp_test(static_reflection_comprehensive_tests     LABELS ondemand acceptance per_implementation)
  add_cpp_test(static_reflection_edge_cases_tests        LABELS ondemand acceptance per_implementation)
  add_cpp_test(static_reflection_enum_tests              LABELS ondemand acceptance per_implementation)
endif(SIMDJSON_STATIC_REFLECTION)
# Copy the simdjson dll into the tests directory
if(MSVC AND BUILD_SHARED_LIBS)
  add_custom_command(TARGET builder_string_builder_tests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:builder_string_builder_tests>")                 # <--this is out-file path
endif(MSVC AND BUILD_SHARED_LIBS)
