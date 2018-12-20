#!/bin/bash
[[ "$(command -v gnuplot)" ]] || { echo "gnuplot is not installed" 1>&2 ; exit 1; }

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"

gnuplot -e "filename='plots/skylake/parselinuxtable.txt';name='plots/skylake/stackedperf.pdf'" $SCRIPTPATH/stackbar.gnuplot

echo "plots/skylake/stackedperf.pdf"