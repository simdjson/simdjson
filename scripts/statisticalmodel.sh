#!/bin/bash

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
cd $SCRIPTPATH/..
datadirectory=$SCRIPTPATH/modeldata/$(uname -n)
mkdir -p $datadirectory

os=$(uname)


if [ "$os" = "Linux" ]; then
  echo "You are using linux."
  echo "We are going to just parse using simdjson, and collect perf stats."

  make statisticalmodel
  myfile=$datadirectory"/modeltable.txt"
  echo $myfile
  echo "" > $myfile

  for i in $SCRIPTPATH/../jsonexamples/*.json; do
    [ -f "$i" ] || break
    echo $i
    echo "#"$i >> "$myfile"
    $SCRIPTPATH/../statisticalmodel  "$i" >> "$myfile"
  done
  echo "see results in "$myfile
else
  echo "Linux not detected"
fi


