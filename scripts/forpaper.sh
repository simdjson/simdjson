#!/bin/bash
[[ "$(command -v gnuplot)" ]] || { echo "gnuplot is not installed" 1>&2 ; exit 1; }

python -c "import pandas"
if [ $? -ne 0 ]; then
  echo "pandas is not installed, try pip install pandas" 1>&2 ; exit 1; 
fi

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"

cd $SCRIPTPATH/..; make clean

cd $SCRIPTPATH/setupfortesting/; sudo ./setupfortesting.sh

$SCRIPTPATH/parsingcompdata.sh

$SCRIPTPATH/plotparse.sh

$SCRIPTPATH/statisticalmodel.sh

