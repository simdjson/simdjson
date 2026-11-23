#!/bin/bash
set -e

TMPDIR1=$(mktemp -d -t simdjsonXXXXXXXX)
trap "exit 1"         HUP INT PIPE QUIT TERM
trap "rm -rf $TMPDIR1" EXIT

echo "running sjmin on jsonexamples and jsonchecker files (prints test successful on success) ..."
SJMIN=$1
JSONEXAMPLES=$2
JSONCHECKER=$3

check_file() {
  local input=$1
  local name=$(basename $input)
  echo $name
  $SJMIN $input > $TMPDIR1/$name
  $SJMIN $TMPDIR1/$name > $TMPDIR1/again$name
  cmp $TMPDIR1/$name $TMPDIR1/again$name
  $SJMIN < $input > $TMPDIR1/stdin$name
  cmp $TMPDIR1/$name $TMPDIR1/stdin$name
  $SJMIN -o $TMPDIR1/output$name - < $input
  cmp $TMPDIR1/$name $TMPDIR1/output$name
}

count=0
for i in $JSONEXAMPLES/*.json $JSONCHECKER/pass*.json; do
  count=$((count + 1))
  check_file $i
done
if [ $count -lt 10 ]; then
  echo "expected the test files in $JSONEXAMPLES and $JSONCHECKER"
  exit 1
fi

expect_status() {
  local expected=$1
  shift
  local status=0
  "$@" > /dev/null 2>&1 || status=$?
  if [ $status -ne $expected ]; then
    echo "expected exit status $expected, got $status: $*"
    exit 1
  fi
}

printf '{"a": "unterminated' > $TMPDIR1/unterminated.json
expect_status 1 $SJMIN $TMPDIR1/unterminated.json
expect_status 1 $SJMIN $TMPDIR1/does_not_exist.json
expect_status 1 $SJMIN -o $TMPDIR1/no/such/dir.json $JSONCHECKER/pass01.json
expect_status 2 $SJMIN --no-such-option
expect_status 2 $SJMIN $JSONCHECKER/pass01.json $JSONCHECKER/pass02.json
expect_status 0 $SJMIN --help
expect_status 0 $SJMIN --version

echo "test successful"
exit 0
