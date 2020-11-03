#!/bin/bash
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
cd $SCRIPTPATH/..
make parseandstatcompetition
echo "parsing and collecting basic stats on json documents as quickly as possible"
echo
for i in $SCRIPTPATH/../jsonexamples/*.json; do
    [ -f "$i" ] || break
    echo $i
    $SCRIPTPATH/../parseandstatcompetition $i
    echo
done

make distinctuseridcompetition
echo "parsing and finding all user.id"
echo

for i in $SCRIPTPATH/../jsonexamples/twitter.json; do
    [ -f "$i" ] || break
    echo $i
    $SCRIPTPATH/../distinctuseridcompetition  jsonexamples/twitter.json
    echo
done

