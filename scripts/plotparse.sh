#!/bin/bash
[[ "$(command -v gnuplot)" ]] || { echo "gnuplot is not installed" 1>&2 ; exit 1; }

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
cd $SCRIPTPATH/..
plotdirectory=$SCRIPTPATH/plots/$(uname -n)
mkdir -p $plotdirectory

os=$(uname)


if [ "$os" = "Linux" ]; then
  echo "You are using linux."
  echo "We are going to just parse using simdjson, and collect perf stats."

  make parse
  for i in $SCRIPTPATH/../jsonexamples/*.json; do
    [ -f "$i" ] || break
    echo $i
    shortname=$(basename $SCRIPTPATH/$i"justparse.table")
    corename=$(basename ${shortname%.*})".pdf"
    $SCRIPTPATH/../parse -t $i > $plotdirectory/$shortname
    gnuplot -e "filename='$plotdirectory/$shortname';name='$plotdirectory/$corename'" $SCRIPTPATH/bar.gnuplot
    rm $plotdirectory/$shortname
    echo
  done
fi

make parsingcompetition
echo "parsing (with competition)"
echo 
for i in $SCRIPTPATH/../jsonexamples/*.json; do
    [ -f "$i" ] || break
    echo $i
    shortname=$(basename $SCRIPTPATH/$i.table)
    corename=$(basename ${shortname%.*})".pdf"
    $SCRIPTPATH/../parsingcompetition -t $i > $plotdirectory/$shortname
    sort $plotdirectory/$shortname > $plotdirectory/$shortname.table.sorted
    gnuplot -e "filename='$plotdirectory/$shortname.table.sorted';name='$plotdirectory/$corename'" $SCRIPTPATH/bar.gnuplot
    rm $plotdirectory/$shortname
    rm $plotdirectory/$shortname.table.sorted
    echo
done


make parseandstatcompetition
echo "parsing and collecting basic stats on json documents as quickly as possible"
echo 
for i in $SCRIPTPATH/../jsonexamples/*.json; do
    [ -f "$i" ] || break
    echo $i
    shortname=$(basename $SCRIPTPATH/$i"parseandstat.table")
    corename=$(basename ${shortname%.*})".pdf"
    $SCRIPTPATH/../parseandstatcompetition  -t $i> $plotdirectory/$shortname
    sort $plotdirectory/$shortname > $plotdirectory/$shortname.table.sorted
    gnuplot -e "filename='$plotdirectory/$shortname.table.sorted';name='$plotdirectory/$corename'" $SCRIPTPATH/bar.gnuplot
    rm $plotdirectory/$shortname
    rm $plotdirectory/$shortname.table.sorted
    echo
done

make distinctuseridcompetition
echo "parsing and finding all user.id"
echo 

for i in $SCRIPTPATH/../jsonexamples/twitter.json; do
    [ -f "$i" ] || break
    echo $i
    shortname=$(basename $SCRIPTPATH/$i"distinctuserid.table")
    corename=$(basename ${shortname%.*})".pdf"
    $SCRIPTPATH/../distinctuseridcompetition   -t jsonexamples/twitter.json> $plotdirectory/$shortname
    sort $plotdirectory/$shortname > $plotdirectory/$shortname.table.sorted
    gnuplot -e "filename='$plotdirectory/$shortname.table.sorted';name='$plotdirectory/$corename'" $SCRIPTPATH/bar.gnuplot
    rm $plotdirectory/$shortname
    rm $plotdirectory/$shortname.table.sorted
    echo
done

echo "see results in "$plotdirectory