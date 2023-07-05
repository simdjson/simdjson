# All remaining tests link with simdjson proper
link_libraries(simdjson)
include_directories(..)
add_subdirectory(compilation_failure_tests)
add_cpp_test(singlestage_log_tests              LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_log_error_tests        LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_tostring_tests         LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_active_tests           LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_array_tests            LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_array_error_tests      LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_compilation_tests      LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_document_stream_tests  LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_error_tests            LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_error_location_tests   LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_json_pointer_tests     LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_key_string_tests       LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_misc_tests             LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_number_tests           LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_number_in_string_tests LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_object_tests           LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_object_error_tests     LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_ordering_tests         LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_parse_api_tests        LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_readme_examples        LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_scalar_tests           LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_twitter_tests          LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_wrong_type_error_tests LABELS singlestage acceptance per_implementation)
add_cpp_test(singlestage_iterate_many_csv       LABELS singlestage acceptance per_implementation)

if(HAVE_POSIX_FORK AND HAVE_POSIX_WAIT) # assert tests use fork and wait, which aren't on MSVC
  add_cpp_test(singlestage_assert_out_of_order_values LABELS assert per_implementation explicitonly singlestage)
endif()

# Copy the simdjson dll into the tests directory
if(MSVC AND BUILD_SHARED_LIBS)
  add_custom_command(TARGET singlestage_parse_api_tests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:singlestage_parse_api_tests>")                 # <--this is out-file path
endif(MSVC AND BUILD_SHARED_LIBS)
