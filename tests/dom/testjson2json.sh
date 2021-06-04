#!/bin/bash
set -e

TMPDIR1=$(mktemp -d -t simdjsonXXXXXXXX)
TMPDIR2=$(mktemp -d -t simdjsonXXXXXXXX)
trap "exit 1"         HUP INT PIPE QUIT TERM
trap "rm -rf $TMPDIR1 $TMPDIR2" EXIT

echo "running json2json on jsonexamples and jsonchecker files (prints test successful on success) ..."
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
DATADIR="$1/_deps/simdjson-data"

JSONEXAMPLES=$DATADIR/jsonexamples
JSONCHECKER=$DATADIR/jsonchecker
for i in `cd $JSONEXAMPLES && ls -1 *.json`; do
  echo $i
  ./json2json $JSONEXAMPLES/$i > $TMPDIR1/$i
  ./json2json $TMPDIR1/$i > $TMPDIR2/$i
  cmp $TMPDIR1/$i $TMPDIR2/$i
  ./minify $TMPDIR1/$i > $TMPDIR1/minify$i
  ./minify $TMPDIR2/$i > $TMPDIR2/minify$i
  cmp $TMPDIR1/minify$i $TMPDIR2/minify$i
  ./json2json $TMPDIR1/minify$i > $TMPDIR2/bisminify$i
  cmp $TMPDIR1/$i $TMPDIR2/bisminify$i
done

for i in `cd $JSONCHECKER && ls -1 pass*.json`; do
  echo $i
  ./json2json $JSONCHECKER/$i > $TMPDIR1/$i
  ./json2json $TMPDIR1/$i > $TMPDIR2/$i
  cmp $TMPDIR1/$i $TMPDIR2/$i
  ./minify $TMPDIR1/$i > $TMPDIR1/minify$i
  ./minify $TMPDIR2/$i > $TMPDIR2/minify$i
  cmp $TMPDIR1/minify$i $TMPDIR2/minify$i
  ./json2json $TMPDIR1/minify$i > $TMPDIR2/bisminify$i
  cmp $TMPDIR1/$i $TMPDIR2/bisminify$i
done

echo "test successful"

exit 0
