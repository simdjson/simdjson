#!/bin/bash
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
cd $SCRIPTPATH/..
for i in jsonchecker/adversarial/issue150/*.json ; do
  echo $i;
  ./allparserscheckfile -m $i;
  if [ $? -ne 0 ];
    then echo "potential bug";
    exit 1
  fi;
done

echo "Code is probably ok. All parsers agree."
exit 0
