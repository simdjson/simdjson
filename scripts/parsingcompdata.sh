#!/bin/bash

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
cd $SCRIPTPATH/..
datadirectory=$SCRIPTPATH/data/$(uname -n)
mkdir -p $datadirectory

os=$(uname)


make parsingcompetition
echo "parsing (with competition)"
echo 
for i in $SCRIPTPATH/../jsonexamples/*.json; do
    [ -f "$i" ] || break
    echo $i
    shortname=$(basename $SCRIPTPATH/$i.table)
    corename=$(basename ${shortname%.*})".pdf"
    $SCRIPTPATH/../parsingcompetition -t $i > $datadirectory/$shortname
    echo
done

echo "see results in "$datadirectory
