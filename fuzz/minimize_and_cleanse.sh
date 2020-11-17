#!/bin/sh

# usage: fuzzer crash


fuzzer=$1

if [ ! -x $fuzzer ] ; then
  echo "arg 1 should be a fuzzer executable"
  exit 1
fi

crash=$2
if [ ! -e $crash ] ; then
  echo "arg2 should be a crashing test case"
  exit 1
fi
if [ -d $crash ] ; then
  echo "crash should be a file, not a dir"
  exit 1
fi

echo "checking that the crash crashes..."
origreport=$(mktemp --tmpdir orig_crash_report.XXXXXXXXX)
if $fuzzer $crash >$origreport 2>&1; then
  echo "your crash does not crash."
  exit 1
else
  echo "...it does."
fi

sizeofcrash=$(stat --format=%s $crash)
echo "starting to minimize crash of size $sizeofcrash..."
minimized=$(mktemp --tmpdir minimized_crash.XXXXXXXXX)
$fuzzer -minimize_crash=1 -exact_artifact_path=$minimized -max_total_time=5 $crash  >/dev/null 2>&1

sizeofminimized=$(stat --format=%s $minimized)
echo "got it down to $sizeofminimized"

echo "checking that the minimized crash crashes..."
report=$(mktemp --tmpdir minimized_crash_report.XXXXXXXXX)
if $fuzzer $minimized >$report 2>&1; then
  echo "your minimized crash does not crash."
  exit 1
else
 echo "...it does."
fi

echo "starting cleansing..."
cleansed=$(mktemp --tmpdir cleansed_crash.XXXXXXXXX)
cleansingreport=$(mktemp --tmpdir cleansing_output.XXXXXXXXX)
$fuzzer $minimized -cleanse_crash=1 -exact_artifact_path=$cleansed >$cleansingreport 2>&1

echo "checking that the cleansed crash crashes..."
report=$(mktemp --tmpdir cleansed_crash_report.XXXXXXXXX)
if $fuzzer $cleansed >$report 2>&1; then
  echo "your cleansed crash $cleansed does not crash. see cleansing report: $cleansingreport"
  exit 1
else
  echo "....it does."
fi

echo "your minimized and cleansed crash (report $origreport) is here: $cleansed and the report for the cleansed crash is here: $report"

