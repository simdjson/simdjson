#!/bin/sh
#
# this file is not meant to be merged, just used during developement to avoid
# having to do the full clone/docker/init step for debugging the cmake conf

set -eu

ossfuzz=$(readlink -f $(dirname $0))/ossfuzz.sh

mkdir -p ossfuzz-out
export OUT=$(pwd)/ossfuzz-out
export CC=clang
export CXX="clang++"
export CFLAGS="-fsanitize=fuzzer-no-link"
export CXXFLAGS="-fsanitize=fuzzer-no-link"
export LIB_FUZZING_ENGINE="-fsanitize=fuzzer"

$ossfuzz

echo "look at the results in $OUT"
