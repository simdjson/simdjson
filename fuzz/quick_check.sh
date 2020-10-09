#!/bin/sh
#
# This script is to make a quick check that the fuzzers work,
# good when working locally developing the fuzzers or making
# sure code changes still pass the fuzzers.
#
# It will download the corpus from bintray (kept up to date
# by the crontab github actions) unless a local out/ directory
# already exists.
#
# Run it standing in the root of the simdjson repository.
#
# By Paul Dreik 20201003

set -eu

for prog in wget tar cmake; do
  if ! which $prog >/dev/null; then
   echo please install $prog
   exit 1
  fi
done

#download the corpus if it does not already exist
if [ ! -d out ] ; then
  wget --quiet https://dl.bintray.com/pauldreik/simdjson-fuzz-corpus/corpus/corpus.tar
  tar xf corpus.tar && rm corpus.tar
fi

builddir=build-sanitizers

if [ ! -d $builddir ] ; then
  fuzz/build_fuzzer_variants.sh
else
  cmake --build $builddir --target all_fuzzers
fi

fuzzernames=$(cmake --build $builddir --target print_all_fuzzernames |tail -n1)

for fuzzer in $fuzzernames ; do
  exe=$builddir/fuzz/$fuzzer
  shortname=$(echo $fuzzer |cut -f2- -d_)
  echo found fuzzer $shortname with executable $exe
  mkdir -p out/$shortname
  others=$(find out -type d -not -name $shortname -not -name out -not -name cmin)
  $exe -max_total_time=20  -max_len=4000 out/$shortname $others
  echo "*************************************************************************"
done
echo "all is good, no errors found in any of these fuzzers: $fuzzernames"

