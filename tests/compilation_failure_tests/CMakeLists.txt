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
# These don't compile with exceptions off
if (SIMDJSON_EXCEPTIONS)
  add_dual_compile_test(bad_array_count)
  add_dual_compile_test(dangling_parser_load)
  add_dual_compile_test(dangling_parser_parse_uint8)
  add_dual_compile_test(dangling_parser_parse_uchar)
  add_dual_compile_test(dangling_parser_parse_stdstring)
  add_dual_compile_test(dangling_parser_parse_padstring)
  add_dual_compile_test(unsafe_parse_many)
endif()

# key_selector enforces a 63-character key-length limit at compile time. This
# needs C++20 concepts, which in this project are enabled together with static
# reflection, so we only register it there.
if (SIMDJSON_STATIC_REFLECTION)
  # (1) A 63-character key (exactly at the limit) must compile.
  add_cpp_test(key_selector_too_long_should_compile SOURCES key_selector_too_long.cpp COMPILE_ONLY LABELS acceptance no_mingw)
  # (2) A 64-character key must fail to compile with the documented static_assert
  #     message. We check the message text -- not merely that it fails -- so the
  #     pass/fail is driven from the compiler output via PASS_REGULAR_EXPRESSION
  #     (which ignores the build's non-zero exit code) rather than WILL_FAIL.
  add_cpp_test(key_selector_too_long_should_not_compile SOURCES key_selector_too_long.cpp COMPILE_ONLY LABELS acceptance no_mingw)
  target_compile_definitions(key_selector_too_long_should_not_compile PRIVATE COMPILATION_TEST_USE_FAILING_CODE=1)
  set_property(TEST key_selector_too_long_should_not_compile PROPERTY PASS_REGULAR_EXPRESSION "key_selector keys must be at most 63 characters long")
endif()