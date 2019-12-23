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
# cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DENABLE_FUZZING=On -DSIMDJSON_FUZZ_LINKMAIN=Off -DSIMDJSON_FUZZ_LDFLAGS=-fsanitize=fuzzer
# ninja


# settings this links in a main. useful for reproducing,
# kcov, gdb, afl, valgrind.
# (note that libFuzzer can also reproduce, just pass it the files)
#
# Using this by default, means the fuzzers will be built as a part of the normal
# workflow, meaning they wont bitrot and will participate in refactoring etc.
#
option(SIMDJSON_FUZZ_LINKMAIN "links a main into fuzz targets for building reproducers" On)

# For oss-fuzz - insert $LIB_FUZZING_ENGINE into the link flags, but only for
# the fuzz targets, otherwise the cmake configuration step fails.
set(SIMDJSON_FUZZ_LDFLAGS "" CACHE STRING "LDFLAGS for the fuzz targets")

set(SOURCES
  fuzz_parser.cpp
#  fuzz_minify.cpp   # <--- does not pass the build check test on oss-fuzz, says "partially instrumented". help needed!
  fuzz_dump.cpp
  fuzz_print_json.cpp
  fuzz_dump_raw_tape.cpp
)

add_custom_target(print_all_fuzz_targets
    COMMAND ${CMAKE_COMMAND} -E echo ${SOURCES}
)

macro(implement_fuzzer sourcefile)
  get_filename_component(basename ${sourcefile} NAME_WE)
  set(name ${basename})
  add_executable(${name} ${sourcefile})
  if (SIMDJSON_FUZZ_LINKMAIN)
      target_sources(${name} PRIVATE main.cpp)
  endif ()
  target_link_libraries(${name} PRIVATE ${SIMDJSON_LIB_NAME})
if (SIMDJSON_FUZZ_LDFLAGS)
  target_link_libraries(${name} PRIVATE ${SIMDJSON_FUZZ_LDFLAGS})
endif ()
endmacro ()

foreach (X IN ITEMS ${SOURCES})
  implement_fuzzer(${X})
endforeach ()

