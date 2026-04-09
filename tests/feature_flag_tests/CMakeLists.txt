# Feature flag tests: compile and run against the amalgamated single-header file.
# Each test uses target_compile_definitions to set its own feature macros,
# independent of the project-wide settings.

set(SINGLEHEADER_DIR ${PROJECT_SOURCE_DIR}/singleheader)

function(add_feature_flag_test TEST_NAME)
  cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "DEFS")
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp ${SINGLEHEADER_DIR}/simdjson.cpp)
  target_include_directories(${TEST_NAME} PRIVATE ${SINGLEHEADER_DIR})
  target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
  # Override any inherited feature flag definitions with our own.
  target_compile_definitions(${TEST_NAME} PRIVATE ${ARG_DEFS})
  # Remove inherited link libraries (simdjson-internal-flags brings -Werror
  # which conflicts with the amalgamated header's warning profile).
  set_target_properties(${TEST_NAME} PROPERTIES LINK_LIBRARIES "")
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
  set_property(TEST ${TEST_NAME} APPEND PROPERTY LABELS feature_flags acceptance)
endfunction()

add_feature_flag_test(dom_only DEFS
  SIMDJSON_FEATURE_DOM_API=1 SIMDJSON_FEATURE_ONDEMAND_API=0 SIMDJSON_FEATURE_BUILDER_API=0)
# Note: OnDemand tests also enable DOM because the builtin infrastructure
# is currently included via the DOM/builtin path in the amalgamated header.
add_feature_flag_test(ondemand_only DEFS
  SIMDJSON_FEATURE_DOM_API=1 SIMDJSON_FEATURE_ONDEMAND_API=1 SIMDJSON_FEATURE_BUILDER_API=0)
add_feature_flag_test(dom_and_ondemand DEFS
  SIMDJSON_FEATURE_DOM_API=1 SIMDJSON_FEATURE_ONDEMAND_API=1 SIMDJSON_FEATURE_BUILDER_API=0)
add_feature_flag_test(builder_and_dom DEFS
  SIMDJSON_FEATURE_DOM_API=1 SIMDJSON_FEATURE_ONDEMAND_API=0 SIMDJSON_FEATURE_BUILDER_API=1)
add_feature_flag_test(builder_and_ondemand DEFS
  SIMDJSON_FEATURE_DOM_API=1 SIMDJSON_FEATURE_ONDEMAND_API=1 SIMDJSON_FEATURE_BUILDER_API=1)
add_feature_flag_test(builder_dom_and_ondemand DEFS
  SIMDJSON_FEATURE_DOM_API=1 SIMDJSON_FEATURE_ONDEMAND_API=1 SIMDJSON_FEATURE_BUILDER_API=1)
add_feature_flag_test(builder_only DEFS
  SIMDJSON_FEATURE_DOM_API=1 SIMDJSON_FEATURE_ONDEMAND_API=0 SIMDJSON_FEATURE_BUILDER_API=1)
