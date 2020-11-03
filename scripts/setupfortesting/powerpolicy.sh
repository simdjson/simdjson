#!/usr/bin/env bash
# sudo /usr/bin/cpupower frequency-set -g performance
#######
# taken from http://hbfs.wordpress.com/2013/06/18/fast-path-finding-part-ii/
# might require sudo apt-get install cpufrequtils
# invoke with performance or ondemand
# type cpufreq-info to check results, you can also verify with cat /proc/cpuinfo
# enumerate found CPUs
cpus=$( grep processor /proc/cpuinfo | cut -d: -f 2 )


if [ "$1" = "ondemand" ]; then
  echo "setting up ondemand"
  policy="ondemand"
elif [ "$1" = "performance" ]; then
  echo "setting up for performance"
  policy="performance"
elif [ "$1" = "list" ]; then
  cpufreq-info
  exit 0
else
  echo "usage: powerpolicy.sh ondemand | performance list"
  exit -1
fi

echo "chosen policy " $1
# set governor for each CPU
#
for cpu in ${cpus[@]}
do
  cpufreq-set -c $cpu -g $1
done
