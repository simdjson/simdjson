#!/bin/sh
# stolen from https://github.com/DropD/fnc-simplex/blob/master/linux_turboboost.sh

# you might need to run sudo apt-get install msr-tools
# Toggle Turbo Boost for Ivy Bridge CPUs (should work for all newer Core)
# Requires a fairly new Linux kernel (let's say 3.0+)
# Written by Donjan Rodic, released for free use

# check current real frequency with  sudo turbostat -s -i1

sudo modprobe msr

# all_cores FOO
# perform FOO(i) for each core i
all_cores() {
  NPROCS=`cat /proc/cpuinfo | grep "core id" | wc -l`
  NPROCS=$(($NPROCS - 1))
  for i in `seq 0 1 $NPROCS`; do
    $1 $i
  done
}


# report Turbo Boost state on core $1
read_tb() {
  ret=`sudo rdmsr -p"$1" 0x1a0 -f 38:38`
  [ $ret -eq 0 ] && echo "$1": on || echo "$1": off
}

# enable Turbo Boost on core $1
enable_tb() {
  sudo wrmsr -p"$1" 0x1a0 0x850089
}

# disable Turbo Boost on core $1
disable_tb() {
  sudo wrmsr -p"$1" 0x1a0 0x4000850089
}


if [ "$1" = "on" ]; then
  all_cores enable_tb
  all_cores read_tb
elif [ "$1" = "off" ]; then
  all_cores disable_tb
  all_cores read_tb
elif [ "$1" = "list" ]; then
  all_cores read_tb
else
  echo "usage: turboboost.sh on | off | list"
fi
