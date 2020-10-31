#!/bin/sh
#
# This file builds multiple variants of the fuzzers
# - different sanitizers
# - different build options
# - reproduce build, for running through valgrind
#
# Set environment variable CLANGVERSION to select clang version (example: "-11")

# fail on error
set -e

unset CXX CC CFLAGS CXXFLAGS LDFLAGS

me=$(basename $0)


if [ -z $CLANGVERSION ] ; then
    # the default clang version is set low enough to be found on current Debian stable (Buster)
    CLANGVERSION=-8
fi

# detect unset variables
set -u

# common options
COMMON="-GNinja -DCMAKE_CXX_COMPILER=clang++$CLANGVERSION -DCMAKE_C_COMPILER=clang$CLANGVERSION -DSIMDJSON_BUILD_STATIC=Off -DENABLE_FUZZING=On -DSIMDJSON_COMPETITION=OFF -DSIMDJSON_GOOGLE_BENCHMARKS=OFF -DSIMDJSON_GIT=Off"

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
variant=sanitizers

    if [ ! -d build-$variant ] ; then
	
	mkdir build-$variant
	cd build-$variant
	
	cmake .. \
	      $COMMON \
	      -DCMAKE_CXX_FLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined" \
	      -DCMAKE_C_FLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined" \
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
	      -DCMAKE_CXX_FLAGS="-fsanitize=fuzzer-no-link" \
	      -DCMAKE_C_FLAGS="-fsanitize=fuzzer-no-link" \
	      -DCMAKE_BUILD_TYPE=Release \
	      -DSIMDJSON_FUZZ_LINKMAIN=Off \
	      -DSIMDJSON_FUZZ_LDFLAGS="-fsanitize=fuzzer"
	
	ninja all_fuzzers
	cd ..
    fi
