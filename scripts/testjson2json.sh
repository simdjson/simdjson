#!/bin/bash

TMPDIR1=$(mktemp -d -t simdjsonXXXXXXXX)
TMPDIR2=$(mktemp -d -t simdjsonXXXXXXXX)
TMPDIR3=$(mktemp -d -t simdjsonXXXXXXXX)
TMPDIR4=$(mktemp -d -t simdjsonXXXXXXXX)
trap "exit 1"         HUP INT PIPE QUIT TERM
trap "rm -rf $TMPDIR1 $TMPDIR2 $TMPDIR3 $TMPDIR4" EXIT

function founderror() {
  echo "code is wrong"
  exit 1
}

make minify json2json
for i in `cd jsonexamples && ls -1 *.json`; do
  echo $i
  ./json2json jsonexamples/$i > $TMPDIR1/$i
  ./json2json -a jsonexamples/$i > $TMPDIR3/$i
  ./json2json $TMPDIR1/$i > $TMPDIR2/$i
  ./json2json -a $TMPDIR3/$i > $TMPDIR4/$i
  cmp $TMPDIR1/$i $TMPDIR2/$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    founderror
  fi
  cmp $TMPDIR3/$i $TMPDIR4/$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    founderror
  fi
  ./minify $TMPDIR1/$i > $TMPDIR1/minify$i
  ./minify $TMPDIR2/$i > $TMPDIR2/minify$i
  ./minify $TMPDIR3/$i > $TMPDIR3/minify$i
  ./minify $TMPDIR4/$i > $TMPDIR4/minify$i
  cmp $TMPDIR1/minify$i $TMPDIR2/minify$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    founderror
  fi
  cmp $TMPDIR3/minify$i $TMPDIR4/minify$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    founderror
  fi
  ./json2json $TMPDIR1/minify$i > $TMPDIR2/bisminify$i
  cmp $TMPDIR1/$i $TMPDIR2/bisminify$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    founderror
  fi
done

for i in `cd jsonchecker && ls -1 pass*.json`; do
  echo $i
  ./json2json jsonchecker/$i > $TMPDIR1/$i
  ./json2json -a jsonchecker/$i > $TMPDIR3/$i
  ./json2json $TMPDIR1/$i > $TMPDIR2/$i
  ./json2json -a $TMPDIR3/$i > $TMPDIR4/$i
  cmp $TMPDIR1/$i $TMPDIR2/$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    echo "reg failure"
    founderror
  fi
  cmp $TMPDIR3/$i $TMPDIR4/$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    echo "-a failure"
    founderror
  fi
  ./minify $TMPDIR1/$i > $TMPDIR1/minify$i
  ./minify $TMPDIR2/$i > $TMPDIR2/minify$i
  ./minify $TMPDIR3/$i > $TMPDIR3/minify$i
  ./minify $TMPDIR4/$i > $TMPDIR4/minify$i
  cmp $TMPDIR1/minify$i $TMPDIR2/minify$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    echo "reg failure, step 2"
    founderror
  fi
  cmp $TMPDIR3/minify$i $TMPDIR4/minify$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    echo "-a failure, step 2"
    founderror
  fi
  ./json2json $TMPDIR1/minify$i > $TMPDIR2/bisminify$i
  cmp $TMPDIR1/$i $TMPDIR2/bisminify$i
  retVal=$?
  if [ $retVal -ne 0 ]; then
    founderror
  fi
done


echo "test successful"

exit 0
