if(NOT SIMDJSON_LEGACY_VISUAL_STUDIO AND NOT SIMDJSON_WINDOWS_DLL)
  option(SIMDJSON_ENABLE_FUZZING "enable building the fuzzers" ON)
else()
  option(SIMDJSON_ENABLE_FUZZING "enable building the fuzzers" OFF)
endif()


if(SIMDJSON_ENABLE_FUZZING)

  # First attempt at a fuzzer, using libFuzzer.
  #
  # compile like this:
  # mkdir build-fuzzer
  # cd build-fuzzer
  # export LDFLAGS="-fsanitize=address,undefined"
  # export CXXFLAGS="-fsanitize=fuzzer-no-link,address,undefined"
  # export CFLAGS="-fsanitize=fuzzer-no-link,address,undefined"
  # export CXX=clang++
  # export CC=clang++
  # cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DSIMDJSON_ENABLE_FUZZING=On -DSIMDJSON_FUZZ_LINKMAIN=Off -DSIMDJSON_FUZZ_LDFLAGS=-fsanitize=fuzzer
  # ninja

  # settings this links in a main. useful for reproducing,
  # kcov, gdb, afl, valgrind.
  # (note that libFuzzer can also reproduce, just pass it the files)
  #
  # Using this by default, means the fuzzers will be built as a part of the normal
  # workflow, meaning they won't bitrot and will participate in refactoring etc.
  #
  option(SIMDJSON_FUZZ_LINKMAIN "links a main into fuzz targets for building reproducers" On)

  # For oss-fuzz - insert $LIB_FUZZING_ENGINE into the link flags, but only for
  # the fuzz targets, otherwise the cmake configuration step fails.
  set(SIMDJSON_FUZZ_LDFLAGS "" CACHE STRING "LDFLAGS for the fuzz targets")

  # Fuzzer build flags and libraries
  add_library(simdjson-fuzzer INTERFACE)
  target_link_libraries(simdjson-fuzzer INTERFACE simdjson)
  if (SIMDJSON_FUZZ_LINKMAIN)
    target_sources(simdjson-fuzzer INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/main.cpp)
  else ()
    target_link_libraries(simdjson-fuzzer INTERFACE ${SIMDJSON_FUZZ_LDFLAGS})
  endif ()
  target_link_libraries(simdjson-fuzzer INTERFACE simdjson-internal-flags)

  # Define the fuzzers
  add_custom_target(all_fuzzers)

  set(fuzzernames)
  function(implement_fuzzer name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE simdjson-fuzzer)
    add_dependencies(all_fuzzers ${name})
    set(fuzzernames ${fuzzernames} ${name} PARENT_SCOPE)
  endfunction()

  implement_fuzzer(fuzz_atpointer)
  implement_fuzzer(fuzz_dump)
  implement_fuzzer(fuzz_dump_raw_tape)
  implement_fuzzer(fuzz_element)
  implement_fuzzer(fuzz_implementations) # parses and serializes again, compares across implementations
  implement_fuzzer(fuzz_minify)          # minify *with* parsing
  implement_fuzzer(fuzz_minifyimpl)      # minify *without* parsing, plus compare implementations
  implement_fuzzer(fuzz_ndjson)          # the ndjson api
  implement_fuzzer(fuzz_ondemand)
  implement_fuzzer(fuzz_padded)
  implement_fuzzer(fuzz_parser)
  implement_fuzzer(fuzz_print_json)
  implement_fuzzer(fuzz_utf8)            # utf8 verification, compares across implementations

  # to be able to get a list of all fuzzers from within a script
  add_custom_target(print_all_fuzzernames
    COMMAND ${CMAKE_COMMAND} -E echo ${fuzzernames})

endif()
