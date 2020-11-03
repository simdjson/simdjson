#!/bin/bash

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
cd $SCRIPTPATH/..
datadirectory=$SCRIPTPATH/data/$(uname -n)
mkdir -p $datadirectory

os=$(uname)


make parsingcompetition allparsingcompetition
echo "parsing (with competition)"
echo
for i in $SCRIPTPATH/../jsonexamples/*.json; do
    [ -f "$i" ] || break
    echo $i
    shortname=$(basename $SCRIPTPATH/$i.table)
    corename=$(basename ${shortname%.*})".pdf"
    $SCRIPTPATH/../parsingcompetition -t $i > $datadirectory/$shortname
    $SCRIPTPATH/../allparsingcompetition -t $i > $datadirectory/all$shortname
    echo
done

echo "see results in "$datadirectory

cd $datadirectory && gnuplot bar.gnuplot
