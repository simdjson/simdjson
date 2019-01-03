#!/bin/bash
[[ "$(command -v gnuplot)" ]] || { echo "gnuplot is not installed" 1>&2 ; exit 1; }

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"

cd $SCRIPTPATH/..; make clean

cd $SCRIPTPATH/setupfortesting/; sudo ./setupfortesting.sh

$SCRIPTPATH/parsingcompdata.sh

$SCRIPTPATH/plotparse.sh

$SCRIPTPATH/statisticalmodel.sh

