#!/bin/sh
#
# This file builds multiple variants of the fuzzers
# - different sanitizers
# - different build options
# - reproduce build, for running through valgrind
#
# Set environment variable CLANGSUFFIX to select clang version (example: "-11")

# fail on error
set -e

unset CXX CC CFLAGS CXXFLAGS LDFLAGS

me=$(basename $0)


if [ -z $CLANGSUFFIX ] ; then
    # the default clang version is set low enough to be found on current Debian stable (Buster)
    CLANGSUFFIX=-8
fi

# detect unset variables
set -u

if ! which clang++$CLANGSUFFIX >/dev/null 2>&1 ; then
  echo "could not find clang++$CLANGSUFFIX"
  exit 1
fi

# find out how to build fuzzer. On amd64 and arm64, libFuzzer is built with the compiler and activated
# with -fsanitize=fuzzer at link time. On power, libFuzzer is shipped separately.
testfuzzer=testfuzzer.cpp
/bin/echo -e "#include <cstddef>\n#include <cstdint>\nextern \"C\" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {return 0;}" >$testfuzzer
if clang++$CLANGSUFFIX -o testfuzzer $testfuzzer -fsanitize=fuzzer && ./testfuzzer -runs=1 >/dev/null 2>&1 ; then
   echo "will use -fsanitize=fuzzer to link libFuzzer"
   SIMDJSON_FUZZ_LDFLAGS="-fsanitize=fuzzer"
elif clang++$CLANGSUFFIX -o testfuzzer $testfuzzer -fsanitize=fuzzer-no-link -lFuzzer  && ./testfuzzer -runs=1 >/dev/null 2>&1 ; then
   echo "will use -lFuzzer to link libFuzzer"
   SIMDJSON_FUZZ_LDFLAGS="-lFuzzer"
else
  echo "could not link to the fuzzer with -fsanitize=fuzzer or -lFuzzer"
  exit 1
fi

if [ -e testfuzzer ] ; then rm testfuzzer; fi
if [ -e $testfuzzer ] ; then rm $testfuzzer; fi

# common options
CXX_CLAGS_COMMON=-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
COMMON="-GNinja -DCMAKE_CXX_COMPILER=clang++$CLANGSUFFIX -DCMAKE_C_COMPILER=clang$CLANGSUFFIX -DSIMDJSON_DEVELOPER_MODE=ON -DBUILD_SHARED_LIBS=ON -DSIMDJSON_ENABLE_FUZZING=On -DSIMDJSON_COMPETITION=OFF -DSIMDJSON_GOOGLE_BENCHMARKS=OFF -DSIMDJSON_DISABLE_DEPRECATED_API=On -DSIMDJSON_FUZZ_LDFLAGS=$SIMDJSON_FUZZ_LDFLAGS"

# A replay build, as plain as it gets. For use with valgrind/gdb.
variant=replay
if [ ! -d build-$variant ] ; then
    mkdir build-$variant
    cd build-$variant

    cmake .. \
	  $COMMON \
	  -DCMAKE_BUILD_TYPE=Debug \
	  -DSIMDJSON_FUZZ_LINKMAIN=On

    ninja all_fuzzers
    cd ..
fi


# A fuzzer with sanitizers. For improved capability to find bugs.
# About the optimization level: (measured for fuzz_atpointer)
# -O0 gives  4000 executions/s
# -O1 gives 20000 executions/s
# -O2 gives 32000 executions/s
# -O3 gives 32000 executions/s
# for reference, the release build (without sanitizers, but with fuzzing instrumentation)
# gives 80000 executions/s.
# A low level is good for debugging. A higher level gets more work done.
# Different levels may uncover different types of bugs, see this interesting
# thread: https://github.com/google/oss-fuzz/issues/2295#issuecomment-481493392
# Oss-fuzz uses -O1 so it may be relevant to use something else than that,
# to do something oss-fuzz does not do.
variant=sanitizers-O3

    if [ ! -d build-$variant ] ; then

	mkdir build-$variant
	cd build-$variant
	cmake .. \
	      $COMMON \
	      -DCMAKE_CXX_FLAGS="-O3 -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined $CXX_CLAGS_COMMON" \
	      -DCMAKE_C_FLAGS="-O3 -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined" \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DSIMDJSON_FUZZ_LINKMAIN=Off

	ninja all_fuzzers
	cd ..
    fi

variant=sanitizers-O0

    if [ ! -d build-$variant ] ; then

	mkdir build-$variant
	cd build-$variant
	cmake .. \
	      $COMMON \
	      -DCMAKE_CXX_FLAGS="-O0 -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined $CXX_CLAGS_COMMON" \
	      -DCMAKE_C_FLAGS="-O0 -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined" \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DSIMDJSON_FUZZ_LINKMAIN=Off

	ninja all_fuzzers
	cd ..
    fi


# A fast fuzzer, for fast exploration rather than finding bugs.
variant=fast
 if [ ! -d build-$variant ] ; then

	mkdir build-$variant
	cd build-$variant

	cmake .. \
	      $COMMON \
	      -DCMAKE_CXX_FLAGS="-fsanitize=fuzzer-no-link $CXX_CLAGS_COMMON" \
	      -DCMAKE_C_FLAGS="-fsanitize=fuzzer-no-link" \
	      -DCMAKE_BUILD_TYPE=Release \
	      -DSIMDJSON_FUZZ_LINKMAIN=Off

	ninja all_fuzzers
	cd ..
    fi
