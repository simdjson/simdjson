#!/bin/bash
echo "Note: the SIMD parser does a bit more work."
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
cd $SCRIPTPATH
make bench
echo 
for i in $SCRIPTPATH/../jsonexamples/*.json; do
    [ -f "$i" ] || break
    echo $i
    $SCRIPTPATH/bench $i
    echo
done
