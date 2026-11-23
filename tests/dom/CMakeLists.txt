
link_libraries(simdjson)
include_directories(..)

if(NOT SIMDJSON_LEGACY_VISUAL_STUDIO AND NOT SIMDJSON_WINDOWS_DLL)
  add_cpp_test(random_string_number_tests LABELS dom acceptance per_implementation)
endif()
add_cpp_test(basictests                 LABELS dom acceptance per_implementation)
add_cpp_test(document_stream_tests      LABELS dom acceptance per_implementation)
add_cpp_test(document_stream_fuzz_tests LABELS dom acceptance per_implementation)
add_cpp_test(document_tests             NEEDS_DATA LABELS dom acceptance per_implementation)
add_cpp_test(unpadded_tests             LABELS dom acceptance per_implementation)
add_cpp_test(padding_contents_tests     LABELS dom acceptance per_implementation)
add_cpp_test(errortests                 LABELS dom acceptance per_implementation)
add_cpp_test(extracting_values_example  LABELS dom acceptance per_implementation)
add_cpp_test(integer_tests              LABELS dom acceptance per_implementation)
add_cpp_test(big_integer_tests          LABELS dom acceptance per_implementation)
add_cpp_test(nan_inf_tests              LABELS dom acceptance per_implementation)
add_cpp_test(locale_independence_tests  LABELS dom acceptance per_implementation)
add_cpp_test(jsoncheck                  NEEDS_DATA LABELS dom acceptance per_implementation)
add_cpp_test(json_path_tests            LABELS dom acceptance per_implementation)
add_cpp_test(array_reverse_tests        LABELS dom acceptance per_implementation)
add_cpp_test(minefieldcheck             NEEDS_DATA LABELS dom acceptance per_implementation)
add_cpp_test(numberparsingcheck         NEEDS_DATA LABELS dom acceptance per_implementation) # https://tools.ietf.org/html/rfc6901
add_cpp_test(parse_many_test            NEEDS_DATA LABELS dom acceptance per_implementation)
add_cpp_test(pointercheck               LABELS dom acceptance per_implementation) # https://tools.ietf.org/html/rfc6901
add_cpp_test(stringparsingcheck         NEEDS_DATA LABELS dom acceptance per_implementation) # https://tools.ietf.org/html/rfc6901
add_cpp_test(trivially_copyable_test    LABELS dom acceptance per_implementation)



find_program(BASH bash)

# Below we skip anything on Windows, not just visual studio, because running bash under Windows requires you to
# map app paths to their "mounted" equivalent (e.g., /mnt/c/...). So even if you have bash under Windows, extra work would be
# required to make things work robustly. Simply put: bash is not quite portable.

# Script tests
if (BASH AND (NOT WIN32) AND SIMDJSON_BASH AND (TARGET sjmin)) # The scripts are not robust enough to run under Windows even if bash is available
  #
  # sjmin test
  #
  if(SIMDJSON_HAS_TEST_DATA)
    get_target_property(examples_definitions jsonexamples-data INTERFACE_COMPILE_DEFINITIONS)
    string(REGEX MATCH "SIMDJSON_BENCHMARK_DATA_DIR=\"([^\"]*)\"" _ "${examples_definitions}")
    set(examples_dir "${CMAKE_MATCH_1}")
    get_target_property(checker_definitions jsonchecker-data INTERFACE_COMPILE_DEFINITIONS)
    string(REGEX MATCH "SIMDJSON_TEST_DATA_DIR=\"([^\"]*)\"" _ "${checker_definitions}")
    set(checker_dir "${CMAKE_MATCH_1}")
    add_test(
      NAME testsjmin
      COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/testsjmin.sh $<TARGET_FILE:sjmin> ${examples_dir} ${checker_dir}
    )
    set_property(TEST testsjmin APPEND PROPERTY LABELS per_implementation)
    add_dependencies(per_implementation_tests sjmin)
  endif()

  #
  # Competition parse test
  #
  if ((SIMDJSON_COMPETITION) AND (!SIMDJSON_SANITIZE))
    # It looks like RapidJSON does not pass the sanitizer under some conditions (Clang 10)
    add_executable(allparserscheckfile allparserscheckfile.cpp)
    add_dependencies(competition_tests allparserscheckfile)
    add_dependencies(per_implementation_tests allparserscheckfile)
    target_link_libraries(allparserscheckfile PRIVATE competition-all)

    add_test(issue150 ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/issue150.sh ${PROJECT_BINARY_DIR})
    set_property(TEST issue150 APPEND PROPERTY LABELS per_implementation competition)
    add_dependencies(all_tests allparserscheckfile)
  endif()

  #
  # sjmin tool test: check that sjmin prints the minified document and
  # nothing after it
  #
  add_test(NAME sjmin COMMAND $<TARGET_FILE:sjmin> ${PROJECT_BINARY_DIR}/jsonexamples/example_config.json)
  set_tests_properties(sjmin PROPERTIES PASS_REGULAR_EXPRESSION "\"port\":5432}}\n?$")
  set_property(TEST sjmin APPEND PROPERTY LABELS acceptance per_implementation)
  add_dependencies(acceptance_tests sjmin)
  add_dependencies(per_implementation_tests sjmin)

  #
  # SIMDJSON_FORCE_IMPLEMENTATION tests: run sjmin with SIMDJSON
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
  if((CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL amd64) AND NOT DEFINED CMAKE_CROSSCOMPILING_EMULATOR)
    add_test(
      NAME simdjson_force_implementation_error
      COMMAND
        ${CMAKE_COMMAND} -E env
        SIMDJSON_FORCE_IMPLEMENTATION=doesnotexist
        $<TARGET_FILE:sjmin> -o sjmin_force.json ${EXAMPLE_JSON}
    )
    set_tests_properties(simdjson_force_implementation_error PROPERTIES WILL_FAIL TRUE)
  endif()
endif()

#
# Compile-only tests with simdjson flags on
#

# Don't add the tests if we're on VS2017 or older; they don't succeed.
if(NOT (MSVC AND MSVC_VERSION LESS 1920))
  if(SIMDJSON_EXCEPTIONS)
    add_cpp_test(readme_examples COMPILE_ONLY LABELS acceptance)
    if(NOT SIMDJSON_STATIC_REFLECTION_ENABLED)
      add_cpp_test(readme_examples11 COMPILE_ONLY LABELS acceptance SOURCES readme_examples.cpp)
      set_target_properties(readme_examples11 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    endif(NOT SIMDJSON_STATIC_REFLECTION_ENABLED)
  endif()

  add_cpp_test(readme_examples_noexceptions COMPILE_ONLY LABELS acceptance)

  if(NOT SIMDJSON_STATIC_REFLECTION_ENABLED)
    add_cpp_test(readme_examples_noexceptions11 COMPILE_ONLY LABELS acceptance SOURCES readme_examples_noexceptions.cpp)
    set_target_properties(readme_examples_noexceptions11 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  endif(NOT SIMDJSON_STATIC_REFLECTION_ENABLED)

  # Compile tests that *should fail*
  add_cpp_test(readme_examples_will_fail_with_exceptions_off WILL_FAIL COMPILE_ONLY LABELS acceptance SOURCES readme_examples.cpp)
  target_compile_definitions(readme_examples_will_fail_with_exceptions_off PRIVATE SIMDJSON_EXCEPTIONS=0)
endif()

#
# Test C++20 features
#

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
  add_cpp_test(ranges_test LABELS dom acceptance per_implementation)
  if(NOT SIMDJSON_STATIC_REFLECTION_ENABLED AND NOT CMAKE_CXX_STANDARD GREATER_EQUAL 20)
     set_target_properties(ranges_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  endif()
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(TARGET basictests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:basictests>")                 # <--this is out-file path
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
