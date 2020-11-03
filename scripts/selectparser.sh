#!/bin/bash
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
cd $SCRIPTPATH/..
make parsingcompetition
echo
for i in "$SCRIPTPATH/../jsonexamples/twitter.json" "$SCRIPTPATH/../jsonexamples/update-center.json" "$SCRIPTPATH/../jsonexamples/github_events.json" "$SCRIPTPATH/../jsonexamples/gsoc-2018.json" ; do
    [ -f "$i" ] || break
    echo $i
    $SCRIPTPATH/../parsingcompetition $i
    echo
done
