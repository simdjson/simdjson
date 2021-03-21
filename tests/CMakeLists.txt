# Most tests need test data, and many need windows headers.
link_libraries(simdjson-internal-flags test-data simdjson-windows-headers)
include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)

add_subdirectory(ondemand)
add_subdirectory(dom)

if(NOT SIMDJSON_LEGACY_VISUAL_STUDIO AND NOT SIMDJSON_WINDOWS_DLL)
  add_cpp_test(staticchecks        LABELS other acceptance                   )
  target_link_libraries(staticchecks simdjson-include-source)
endif()

link_libraries(simdjson)
add_cpp_test(padded_string_tests LABELS other acceptance                   )
add_cpp_test(unicode_tests       LABELS other acceptance per_implementation)


find_program(BASH bash)

# Below we skip anything on Windows, not just visual studio, because running bash under Windows requires you to
# map app paths to their "mounted" equivalent (e.g., /mnt/c/...). So even if you have bash under Windows, extra work would be
# required to make things work robustly. Simply put: bash is not quite portable.

# Script tests
if (BASH AND (NOT WIN32) AND SIMDJSON_BASH AND (TARGET json2json)) # The scripts are not robust enough to run under Windows even if bash is available
  # This tests validates that the implementation is what we think it is if we get passed
  # SIMDJSON_FORCE_IMPLEMENTATION, so we know we're testing what we think we're testing
  add_cpp_test(checkimplementation LABELS other per_implementation)

  #
  # SIMDJSON_FORCE_IMPLEMENTATION tests: run json2json with SIMDJSON
  #
  if (SIMDJSON_IMPLEMENTATION MATCHES "fallback")
    add_test(
      NAME simdjson_force_implementation
      COMMAND
        ${CMAKE_COMMAND} -E env
        SIMDJSON_FORCE_IMPLEMENTATION=fallback
        $<TARGET_FILE:checkimplementation>
    )
  endif()
  add_test(
    NAME simdjson_force_implementation_error
    COMMAND
      ${CMAKE_COMMAND} -E env
      SIMDJSON_FORCE_IMPLEMENTATION=doesnotexist
      $<TARGET_FILE:json2json> ${EXAMPLE_JSON}
  )
  set_tests_properties(simdjson_force_implementation_error PROPERTIES WILL_FAIL TRUE)
endif()

if(NOT MSVC)
  ######
  # This tests is to guard us against ever again removing the soversion
  # number from the library. See Bug
  # https://github.com/simdjson/simdjson/issues/1014
  #####
  get_target_property(REPORTED_SIMDJSON_SOVERSION simdjson SOVERSION)
  if(NOT ${REPORTED_SIMDJSON_SOVERSION} STREQUAL ${SIMDJSON_LIB_SOVERSION})
    message(FATAL_ERROR "The library target does not have the proper soversion information." )
  endif()
  get_target_property(REPORTED_SIMDJSON_VERSION simdjson VERSION)
  if(NOT ${REPORTED_SIMDJSON_VERSION} STREQUAL ${SIMDJSON_LIB_VERSION})
    message(FATAL_ERROR "The library target does not have the proper version information." )
  endif()
endif()
