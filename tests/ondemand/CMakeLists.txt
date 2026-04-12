# All remaining tests link with simdjson proper
link_libraries(simdjson)
include_directories(..)
add_subdirectory(compilation_failure_tests)
add_cpp_test(ondemand_car_tag_invoke_deserialization LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_car_deserialization            LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_log_tests                      LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_log_error_tests                LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_tostring_tests                 LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_active_tests                   LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_array_tests                    LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_array_error_tests              LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_compilation_tests              LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_document_stream_tests          LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_document_stream_fuzz_tests     LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_error_tests                    LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_error_location_tests           LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_json_pointer_tests             LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_json_path_tests                LABELS ondemand acceptance per_implementation)
add_cpp_test(compile_time_json_path_tests            LABELS ondemand acceptance per_implementation)
add_cpp_test(compile_time_json_pointer_tests         LABELS ondemand acceptance per_implementation)
add_cpp_test(compile_time_no_validation_tests        LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_key_string_tests               LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_misc_tests                     LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_number_tests                   LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_number_in_string_tests         LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_object_tests                   LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_object_error_tests             LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_ordering_tests                 LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_parse_api_tests                LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_readme_examples                LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_scalar_tests                   LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_to_string                      LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_twitter_tests                  LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_wrong_type_error_tests         LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_iterate_many_csv               LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_custom_types_tests             LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_custom_types_document_tests    LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_stl_types_tests                LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_convert_tests                  LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_unknown_tests                  LABELS ondemand acceptance per_implementation)
add_cpp_test(ondemand_wildcard_tests                 LABELS ondemand acceptance per_implementation)
if(NOT SIMDJSON_SANITIZE)
  add_cpp_test(ondemand_padded_input              LABELS ondemand acceptance per_implementation)
  add_cpp_test(ondemand_cacheline              LABELS ondemand acceptance per_implementation)
endif()

# Add the tests if we're on:
#   1. Visual Studio 2022 v17.6 or later
#   2. GCC v14.0.0 or later (GCC v13.0.0 cannot handle pipe operator of lambda)
#   3. Clang v15.0.0 or later (certain version C++ headers occur error when compiling)
#   4. or if we are targeting C++20 or better
if(
  (MSVC AND MSVC_VERSION GREATER_EQUAL 1930) OR
  (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "14.0.0") OR
  (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.0.0") OR
  (CMAKE_CXX_STANDARD GREATER_EQUAL 20)
  )
  message(STATUS "compiler id: ${CMAKE_CXX_COMPILER_ID} version: ${CMAKE_CXX_COMPILER_VERSION}")
  add_cpp_test(ondemand_ranges_tests LABELS ondemand acceptance per_implementation)
  if(NOT SIMDJSON_STATIC_REFLECTION AND NOT  CMAKE_CXX_STANDARD GREATER_EQUAL 20)
     set_target_properties(ondemand_ranges_tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  endif()
endif()


if(HAVE_POSIX_FORK AND HAVE_POSIX_WAIT) # assert tests use fork and wait, which aren't on MSVC
  add_cpp_test(ondemand_assert_out_of_order_values LABELS assert per_implementation explicitonly ondemand)
endif()

# Copy the simdjson dll into the tests directory
if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(TARGET ondemand_parse_api_tests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:ondemand_parse_api_tests>")                 # <--this is out-file path
endif(WIN32 AND BUILD_SHARED_LIBS)
