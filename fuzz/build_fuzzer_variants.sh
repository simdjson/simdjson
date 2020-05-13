#!/bin/sh
#
# This file builds multiple variants of the fuzzers
# - different sanitizers
# - different build options
# - reproduce build, for running through valgrind

# fail on error
set -eu

unset CXX CC CFLAGS CXXFLAGS LDFLAGS

me=$(basename $0)

# A reproduce build, without avx but otherwise as plain
# as it gets. No sanitizers or optimization.
variant=plain-noavx
if [ ! -d build-$variant ] ; then
    mkdir build-$variant
    cd build-$variant
    
    cmake .. \
	  -GNinja \
	  -DCMAKE_BUILD_TYPE=Debug \
	  -DSIMDJSON_BUILD_STATIC=On \
	  -DENABLE_FUZZING=On \
	  -DSIMDJSON_COMPETITION=OFF \
	  -DSIMDJSON_GOOGLE_BENCHMARKS=OFF \
	  -DSIMDJSON_FUZZ_LINKMAIN=On \
	  -DSIMDJSON_IMPLEMENTATION_HASWELL=0
    
    ninja all_fuzzers
    cd ..
fi

# A reproduce build as plain as it gets. Everythings tunable is
# using the defaults.
variant=plain-normal
if [ ! -d build-$variant ] ; then
    mkdir build-$variant
    cd build-$variant
    
    cmake .. \
	  -GNinja \
	  -DCMAKE_BUILD_TYPE=Debug \
	  -DSIMDJSON_BUILD_STATIC=On \
	  -DENABLE_FUZZING=On \
	  -DSIMDJSON_COMPETITION=OFF \
	  -DSIMDJSON_GOOGLE_BENCHMARKS=OFF \
	  -DSIMDJSON_FUZZ_LINKMAIN=On
    
    ninja all_fuzzers
    cd ..
fi

# a fuzzer with sanitizers, built with avx disabled.
variant=ossfuzz-noavx
if [ ! -d build-$variant ] ; then
    
    export CC=clang
    export CXX="clang++"
    export CFLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined -mno-avx2 -mno-avx "
    export CXXFLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined -mno-avx2 -mno-avx"
    export LIB_FUZZING_ENGINE="-fsanitize=fuzzer"
    
    mkdir build-$variant
    cd build-$variant
    
    cmake .. \
	  -GNinja \
	  -DCMAKE_BUILD_TYPE=Debug \
	  -DSIMDJSON_BUILD_STATIC=On \
	  -DENABLE_FUZZING=On \
	  -DSIMDJSON_COMPETITION=OFF \
	  -DSIMDJSON_GOOGLE_BENCHMARKS=OFF \
	  -DSIMDJSON_FUZZ_LINKMAIN=Off \
	  -DSIMDJSON_FUZZ_LDFLAGS=$LIB_FUZZING_ENGINE \
	  -DSIMDJSON_IMPLEMENTATION_HASWELL=0
    
    ninja all_fuzzers
    cd ..
fi


# a fuzzer with sanitizers, built with avx disabled.
variant=ossfuzz-noavx9
if which clang++-9 >/dev/null 2>&1 ; then
    if [ ! -d build-$variant ] ; then
	
	export CC=clang-9
	export CXX="clang++-9"
	export CFLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined -mno-avx2 -mno-avx "
	export CXXFLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined -mno-avx2 -mno-avx"
	export LIB_FUZZING_ENGINE="-fsanitize=fuzzer"
	
	mkdir build-$variant
	cd build-$variant
	
	cmake .. \
	      -GNinja \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DSIMDJSON_BUILD_STATIC=On \
	      -DENABLE_FUZZING=On \
	      -DSIMDJSON_COMPETITION=OFF \
	      -DSIMDJSON_GOOGLE_BENCHMARKS=OFF \
	      -DSIMDJSON_FUZZ_LINKMAIN=Off \
	      -DSIMDJSON_FUZZ_LDFLAGS=$LIB_FUZZING_ENGINE \
	      -DSIMDJSON_IMPLEMENTATION_HASWELL=0
	
	ninja all_fuzzers
	cd ..
    fi
else
    echo "$me: WARNING clang++-9 not found, please install it to build $variant"
fi

# a fuzzer with sanitizers, default built
variant=ossfuzz-withavx
if [ ! -d build-$variant ] ; then
    
    export CC=clang
    export CXX="clang++"
    export CFLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined"
    export CXXFLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined"
    export LIB_FUZZING_ENGINE="-fsanitize=fuzzer"
    
    mkdir build-$variant
    cd build-$variant
    
    cmake .. \
	  -GNinja \
	  -DCMAKE_BUILD_TYPE=Debug \
	  -DSIMDJSON_BUILD_STATIC=On \
	  -DENABLE_FUZZING=On \
	  -DSIMDJSON_COMPETITION=OFF \
	  -DSIMDJSON_GOOGLE_BENCHMARKS=OFF \
	  -DSIMDJSON_FUZZ_LINKMAIN=Off \
	  -DSIMDJSON_FUZZ_LDFLAGS=$LIB_FUZZING_ENGINE
    
    ninja all_fuzzers
    cd ..
fi

# a fast fuzzer, for fast exploration
variant=ossfuzz-fast9
if which clang++-9 >/dev/null 2>&1 ; then
    if [ ! -d build-$variant ] ; then
	export CC=clang-9
	export CXX="clang++-9"
	export CFLAGS="-fsanitize=fuzzer-no-link -O3 -g"
	export CXXFLAGS="-fsanitize=fuzzer-no-link -O3 -g"
	export LIB_FUZZING_ENGINE="-fsanitize=fuzzer"
	
	mkdir build-$variant
	cd build-$variant
	
	cmake .. \
	      -GNinja \
	      -DCMAKE_BUILD_TYPE= \
	      -DSIMDJSON_BUILD_STATIC=On \
	      -DENABLE_FUZZING=On \
	      -DSIMDJSON_COMPETITION=OFF \
	      -DSIMDJSON_GOOGLE_BENCHMARKS=OFF \
	      -DSIMDJSON_FUZZ_LINKMAIN=Off \
	      -DSIMDJSON_FUZZ_LDFLAGS=$LIB_FUZZING_ENGINE
	
	ninja all_fuzzers
	
	cd ..
    fi
else
    echo "$me: WARNING clang++-9 not found, please install it to build $variant"
fi

