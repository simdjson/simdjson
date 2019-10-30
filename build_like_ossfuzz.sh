#!/bin/sh
#
# this file is not meant to be merged, just used during developement to avoid
# having to do the full clone/docker/init step for debugging the cmake conf

set -eu

mkdir build-plain
cd build-plain

cmake .. \
-GNinja \
-DCMAKE_BUILD_TYPE=Debug \
-DSIMDJSON_BUILD_STATIC=On \
-DCMAKE_CXX_STANDARD=14 \
-DENABLE_FUZZING=On \
-DSIMDJSON_FUZZ_LINKMAIN=On

ninja

cd ..

export CC=clang
export CXX="clang++"
export CXXFLAGS="-fsanitize=fuzzer-no-link"
export LIB_FUZZING_ENGINE="-fsanitize=fuzzer"

mkdir build-ossfuzz
cd build-ossfuzz

cmake .. \
-GNinja \
-DCMAKE_BUILD_TYPE=Debug \
-DSIMDJSON_BUILD_STATIC=On \
-DCMAKE_CXX_STANDARD=14 \
-DENABLE_FUZZING=On \
-DSIMDJSON_FUZZ_LINKMAIN=Off \
-DSIMDJSON_FUZZ_LDFLAGS=$LIB_FUZZING_ENGINE

ninja


