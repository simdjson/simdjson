#!/bin/bash
[[ "$(command -v gnuplot)" ]] || { echo "gnuplot is not installed" 1>&2 ; exit 1; }

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"

gnuplot -e "filename='plots/skylake/parselinuxtable.txt';name='plots/skylake/stackedperf.pdf'" $SCRIPTPATH/stackbar.gnuplot
gnuplot -e "filename='plots/nuc/parselinuxtable.txt';name='plots/nuc/stackedperf.pdf'" $SCRIPTPATH/stackbar.gnuplot

echo "plots/skylake/stackedperf.pdf"
echo "plots/nuc/stackedperf.pdf"

cd $SCRIPTPATH/data/nuc/; gnuplot bar.gnuplot
cd $SCRIPTPATH/data/skylake/; gnuplot bar.gnuplot
echo "data/skylake/gbps.pdf"
echo "data/nuc/gbps.pdf"
