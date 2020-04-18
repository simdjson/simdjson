#!/bin/bash

set -ex

set | grep CHECKPERF

if [ -z "$CHECKPERF_REPOSITORY" ]; then CHECKPERF_REPOSITORY=.; fi

# Arguments: perfdiff.sh <branch> <test json files>
if [ -z "$CHECKPERF_BRANCH" ]; then CHECKPERF_BRANCH=master; fi
if [ -z "$CHECKPERF_DIR" ]; then CHECKPERF_DIR=benchbranch/$CHECKPERF_BRANCH; fi
if [ -z "$CHECKPERF_ARGS" ]; then
  if [ -z "$*" ]; then
    CHECKPERF_ARGS=jsonexamples/twitter.json;
  else
    CHECKPERF_ARGS=$*;
  fi
fi

# Clone and build the reference branch's parse
if [ -d $CHECKPERF_DIR/.git ]; then
  echo "Checking out the reference branch ($CHECKPERF_BRANCH) into $CHECKPERF_DIR ..."
  pushd $CHECKPERF_DIR
  git remote update
  git reset --hard origin/$CHECKPERF_BRANCH
else
  echo "Cloning the reference branch ($CHECKPERF_BRANCH) into $CHECKPERF_DIR ..."
  mkdir -p $CHECKPERF_DIR
  git clone --depth 1 -b $CHECKPERF_BRANCH $CHECKPERF_REPOSITORY $CHECKPERF_DIR
  pushd $CHECKPERF_DIR
fi

echo "Building $CHECKPERF_DIR/parse ..."
if [ -n "$CHECKPERF_CMAKECACHE" ]; then
  cp $CHECKPERF_CMAKECACHE $CHECKPERF_DIR/CMakeCache.txt
  cmake -DSIMDJSON_GOOGLE_BENCHMARKS=OFF -DSIMDJSON_COMPETITION=OFF .
  make parse
  REFERENCE_PARSE=$CHECKPERF_DIR/benchmark/parse
else
  make parse
  REFERENCE_PARSE=$CHECKPERF_DIR/parse
fi
popd

# Run them and diff performance
echo "Running perfdiff:"
echo ./perfdiff \"./parse -t $CHECKPERF_ARGS\" \"$REFERENCE_PARSE -t $CHECKPERF_ARGS\"
./perfdiff "./parse -t $CHECKPERF_ARGS" "$REFERENCE_PARSE -t $CHECKPERF_ARGS"
