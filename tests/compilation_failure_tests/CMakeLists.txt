#
# This directory contains files aimed to verify that constructs that
# are supposed to fail at compile time, indeed do so.
# To prevent bit rot, the same source file is compiled twice with
# the macro COMPILATION_TEST_USE_FAILING_CODE set to 0 or 1.
#

# adds a compilation test. Two targets are created, one expected to
# succeed compilation and one that is expected to fail.
function(add_dual_compile_test TEST_NAME)
  add_cpp_test(${TEST_NAME}_should_compile SOURCES ${TEST_NAME}.cpp COMPILE_ONLY LABELS no_mingw)
  add_cpp_test(${TEST_NAME}_should_not_compile SOURCES ${TEST_NAME}.cpp COMPILE_ONLY WILL_FAIL LABELS acceptance no_mingw)
  target_compile_definitions(${TEST_NAME}_should_not_compile PRIVATE COMPILATION_TEST_USE_FAILING_CODE=1)
endfunction(add_dual_compile_test)


if(NOT BUILD_SHARED_LIBS)
  # Add multiple_include BEFORE link_libraries(simdjson) so that myexe only receives
  # simdjson once (through mylib PUBLIC simdjson::simdjson) and not a second time from
  # a directory-wide link entry.
  add_subdirectory(multiple_include)
endif()

# Dual-compile tests build executables that reference simdjson symbols, so they need
# the library linked in.
link_libraries(simdjson)

add_dual_compile_test(example_compiletest)
add_dual_compile_test(object_position_opaque)
# These don't compile with exceptions off
if (SIMDJSON_EXCEPTIONS)
  add_dual_compile_test(bad_array_count)
  add_dual_compile_test(dangling_parser_load)
  add_dual_compile_test(dangling_parser_parse_uint8)
  add_dual_compile_test(dangling_parser_parse_uchar)
  add_dual_compile_test(dangling_parser_parse_stdstring)
  add_dual_compile_test(dangling_parser_parse_padstring)
  add_dual_compile_test(unsafe_parse_many)
  # parse_many()/iterate_many() return a document_stream that points at the input,
  # so they must reject temporaries. A std::string_view argument converts implicitly
  # to a padded_string temporary.
  add_dual_compile_test(dangling_parse_many_stdstring)
  add_dual_compile_test(dangling_parse_many_string_view)
  add_dual_compile_test(dangling_parse_many_format)
  add_dual_compile_test(dangling_iterate_many_stdstring)
  add_dual_compile_test(dangling_iterate_many_padstring)
  add_dual_compile_test(dangling_iterate_many_load)
  add_dual_compile_test(dangling_iterate_many_string_view)
  add_dual_compile_test(dangling_iterate_many_format)
  if(NOT SIMDJSON_DISABLE_DEPRECATED_API)
    add_dual_compile_test(dangling_iterate_many_deprecated)
  endif()
endif()

# An array, object or value taken from the temporary returned by simdjson::from
# would point into a destroyed document. simdjson::from needs C++20; we match
# "deleted" so that the test cannot pass because of some unrelated error.
if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  foreach(name dangling_from_array dangling_from_get)
    add_dual_compile_test(${name})
    target_compile_features(${name}_should_compile PRIVATE cxx_std_20)
    target_compile_features(${name}_should_not_compile PRIVATE cxx_std_20)
    set_property(TEST ${name}_should_not_compile PROPERTY WILL_FAIL FALSE)
    set_property(TEST ${name}_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION "deleted")
  endforeach()
endif()

# string_builder::append() must reject 128-bit integers rather than truncate
# them. We match "deleted" in the compiler output so that the test cannot pass
# because of some unrelated error.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#ifndef __SIZEOF_INT128__
#error no 128-bit integers
#endif
int main() { return 0; }
" SIMDJSON_HAS_INT128)
if(SIMDJSON_HAS_INT128)
  foreach(TEST_NAME string_builder_append_int128 string_builder_append_uint128)
    add_dual_compile_test(${TEST_NAME})
    set_property(TEST ${TEST_NAME}_should_not_compile PROPERTY WILL_FAIL FALSE)
    set_property(TEST ${TEST_NAME}_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION "deleted")
  endforeach()
  # The range overloads of append() exist only from C++20.
  if(CMAKE_CXX_STANDARD VERSION_GREATER_EQUAL 20)
    add_dual_compile_test(string_builder_append_int128_range)
    set_property(TEST string_builder_append_int128_range_should_not_compile PROPERTY WILL_FAIL FALSE)
    set_property(TEST string_builder_append_int128_range_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION "deleted")
  endif()
endif()

# key_selector enforces a 63-character key-length limit at compile time. This
# needs C++20 concepts, which in this project are enabled together with static
# reflection, so we only register it there.
if (SIMDJSON_STATIC_REFLECTION_ENABLED)
  # (1) A 63-character key (exactly at the limit) must compile.
  add_cpp_test(key_selector_too_long_should_compile SOURCES key_selector_too_long.cpp COMPILE_ONLY LABELS acceptance no_mingw)
  # (2) A 64-character key must fail to compile with the documented static_assert
  #     message. We check the message text -- not merely that it fails -- so the
  #     pass/fail is driven from the compiler output via PASS_REGULAR_EXPRESSION
  #     (which ignores the build's non-zero exit code) rather than WILL_FAIL.
  add_cpp_test(key_selector_too_long_should_not_compile SOURCES key_selector_too_long.cpp COMPILE_ONLY LABELS acceptance no_mingw)
  target_compile_definitions(key_selector_too_long_should_not_compile PRIVATE COMPILATION_TEST_USE_FAILING_CODE=1)
  set_property(TEST key_selector_too_long_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION "key_selector keys must be at most 63 characters long")

  # A number that overflows binary64 has no representation to return and no way
  # to report an error from a constant expression, so it must fail to compile.
  add_dual_compile_test(compile_time_json_overflow)

  # A struct with a 128-bit integer member must not be serialized with a
  # truncated value.
  if(SIMDJSON_HAS_INT128)
    add_dual_compile_test(to_json_int128)
    set_property(TEST to_json_int128_should_not_compile PROPERTY WILL_FAIL FALSE)
    set_property(TEST to_json_int128_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION "Integers wider than 64 bits are not supported")
  endif()

  # Misused annotations must fail to compile with a message that names the
  # problem: the text of std::meta::exception with exceptions, the name of the
  # function that stops the constant evaluation without them.
  add_dual_compile_test(annotation_flatten_reference)
  set_property(TEST annotation_flatten_reference_should_not_compile PROPERTY WILL_FAIL FALSE)
  set_property(TEST annotation_flatten_reference_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION
    "flatten requires a member that is not a reference|flatten_requires_a_member_that_is_not_a_reference")
  add_dual_compile_test(annotation_flatten_non_class)
  set_property(TEST annotation_flatten_non_class_should_not_compile PROPERTY WILL_FAIL FALSE)
  set_property(TEST annotation_flatten_non_class_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION
    "flatten requires a member of class type|flatten_requires_a_member_of_class_type")
  add_dual_compile_test(annotation_transparent_two_members)
  set_property(TEST annotation_transparent_two_members_should_not_compile PROPERTY WILL_FAIL FALSE)
  set_property(TEST annotation_transparent_two_members_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION
    "transparent requires exactly one data member|transparent_requires_exactly_one_data_member")
  add_dual_compile_test(annotation_flatten_container)
  set_property(TEST annotation_flatten_container_should_not_compile PROPERTY WILL_FAIL FALSE)
  set_property(TEST annotation_flatten_container_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION
    "flatten requires a member whose type is a structure|flatten_requires_a_member_deserialized_member_by_member")
endif()