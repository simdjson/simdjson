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

  make parse parse_noutf8validation parse_nonumberparsing parse_nostringparsing
  myfile=$plotdirectory"/parselinuxtable.txt"
  echo $myfile
  echo "" > $myfile

  myfile_noutf8validation=$plotdirectory"/parselinuxtable_noutf8validation.txt"
  echo $myfile_noutf8validation
  echo "" > $myfile_noutf8validation

  myfile=$plotdirectory"/parselinuxtable_nonumberparsing.txt"
  echo $myfile_nonumberparsing
  echo "" > $myfile_nonumberparsing

  myfile=$plotdirectory"/parselinuxtable_nostringparsing.txt"
  echo $myfile_nostringparsing
  echo "" > $myfile_nostringparsing


  for i in $SCRIPTPATH/../jsonexamples/*.json; do
    [ -f "$i" ] || break
    echo $i
    $SCRIPTPATH/../parse -t "$i" >> "$myfile"
    $SCRIPTPATH/../parse_noutf8validation -t "$i" >> "$myfile_noutf8validation"
    $SCRIPTPATH/../parse_nonumberparsing -t "$i" >> "$myfile_nonumberparsing"
    $SCRIPTPATH/../parse_nostringparsing -t "$i" >> "$myfile_nostringparsing"
  done
  paste $myfile $myfile_noutf8validation $myfile_nonumberparsing $myfile_nostringparsing > $myfile.tmp
  mv $myfile.tmp $myfile
  rm  $myfile_noutf8validation $myfile_nonumberparsing $myfile_nostringparsing
  gnuplot -e "filename='$myfile';name='$plotdirectory/stackedperf.pdf'" $SCRIPTPATH/stackbar.gnuplot
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
