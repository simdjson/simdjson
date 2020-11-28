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

# common options
CXX_CLAGS_COMMON=-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
COMMON="-GNinja -DCMAKE_CXX_COMPILER=clang++$CLANGSUFFIX -DCMAKE_C_COMPILER=clang$CLANGSUFFIX -DSIMDJSON_BUILD_STATIC=Off -DENABLE_FUZZING=On -DSIMDJSON_COMPETITION=OFF -DSIMDJSON_GOOGLE_BENCHMARKS=OFF -DSIMDJSON_DISABLE_DEPRECATED_API=On"

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
# to do something oss-fuzz doesn't.
variant=sanitizers-O3

    if [ ! -d build-$variant ] ; then

	mkdir build-$variant
	cd build-$variant
	cmake .. \
	      $COMMON \
	      -DCMAKE_CXX_FLAGS="-O3 -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined $CXX_CLAGS_COMMON" \
	      -DCMAKE_C_FLAGS="-O3 -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined" \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DSIMDJSON_FUZZ_LINKMAIN=Off \
	      -DSIMDJSON_FUZZ_LDFLAGS="-fsanitize=fuzzer"

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
	      -DSIMDJSON_FUZZ_LINKMAIN=Off \
	      -DSIMDJSON_FUZZ_LDFLAGS="-fsanitize=fuzzer"

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
	      -DSIMDJSON_FUZZ_LINKMAIN=Off \
	      -DSIMDJSON_FUZZ_LDFLAGS="-fsanitize=fuzzer"

	ninja all_fuzzers
	cd ..
    fi
