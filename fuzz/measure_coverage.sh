#!/bin/sh
#
# makes a coverage build.
#
# To measure and display the coverage:
#
#cd build-coverage
#fuzz/fuzz_parser path/to/corpus/* # repeat with other fuzzers
#gcovr -r . --html --html-details --sort-uncovered -o out.html
# and view the results in out.html

bdir=build-coverage
if [ ! -d $bdir ] ; then
    mkdir -p $bdir
    cd $bdir

    export CC=gcc
    export CXX="g++"
    export CFLAGS="-fprofile-arcs -ftest-coverage"
    export CXXFLAGS="-fprofile-arcs -ftest-coverage"
    export LDFLAGS="-fprofile-arcs -ftest-coverage"

    cmake .. \
          -GNinja \
          -DCMAKE_BUILD_TYPE=Debug \
          -DSIMDJSON_BUILD_STATIC=On \
          -DENABLE_FUZZING=On \
	  -DSIMDJSON_FUZZ_LINKMAIN=On
    ninja all_fuzzers
    cd ..
fi



