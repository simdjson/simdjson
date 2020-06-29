#!/bin/bash
#
# Usage: [VARIABLES] collect-perf.sh
#
# e.g. hosts="skylake skylake-x" iterations=5000 examples="jsonexamples/*.json" collect-perf.sh
#
# Runs simdjson perf tests against your current (local) source on multiple remote machines in parallel.
#
# Variables
# ---------
# hosts="skylake ampere" -- runs against the given hosts. Default: skylake skylake-x ampere
# 
# ### Copying source
# update_source=0 -- don't copy source to the remote machine. Default: off
# local_root=~/simdjson -- location to copy source 
#
# ### Configuration / Build
# configure_clean=1 -- Completely redo cmake configuration. Required to update compiler. Defaults to 0.
# build=0 -- Don't rebuild. Defaults to 1 if update_source=1, 0 if update_source=0.
# default_host_compiler="CC=gcc-9 CXX=g++-9" -- Set compiler to run by default on hosts. Does not affect ampere. Only has effect if configure_clean=1. Defaults to "CXX=gcc-9 CXX=g++-9"
#
# ### Perf
# implementation=haswell -- force a given implementation. Default is whatever is native to the machine.
# iterations=5000 -- runs parse -n <iterations>
# examples="jsonexamples/*.json" -- run against the given file(s). Default is jsonexamples/twitter.json
#

set -ex

hosts=${hosts:-skylake skylake-x ampere}

root=${root:-~/simdjson-remote}
build_root=$root/build
local_root=${local_root:-~/simdjson}

trap 'for job in `jobs -p`; do kill $job; done' INT

#
# Update source
#
update_source=${update_source:-1}
local_source_files="$local_root/include $local_root/src $local_root/benchmark $local_root/jsonexamples"

if [ "$update_source" = "1" ]; then
  echo ""
  echo ""
  echo "Updating source ..."
  echo ""
  echo ""
  for host in $hosts; do
    rsync -azP --delete $local_source_files --filter=":- $local_root/.gitignore" $host:$root/ &
  done
  for job in `jobs -p`; do wait $job; done
fi

#
# Clean configuration
#
configure_clean=${configure_clean:-}
prepare_build_root="mkdir -p $build_root"
if [ "$configure_clean" = "1" ]; then
  prepare_build_root="echo \"Cleaning build root\" && rm -rf $build_root && $prepare_build_root"
fi

#
# Reconfigure (cmake)
#
configure=${configure:-$update_source}
cmake_options=${cmake_options:-}
default_host_compiler=${default_host_compiler:-CXX=g++-9 CC=gcc-9}
# compiler="CXX=/home/jkeiser/.linuxbrew/opt/llvm@9/bin/clang++ CC=/home/jkeiser/.linuxbrew/opt/llvm@9/bin/clang"
declare -A host_compiler=()
host_compiler[ampere]="CXX=g++ CC=gcc"
declare -A host_configure_command=()
for host in $hosts; do
  if [ -z "${host_compiler[$host]}"]; then host_compiler[$host]=$default_host_compiler; fi
  if [ "$configure" = "1" ]; then
    host_configure_command[$host]="echo \"Running cmake\" && ${host_compiler[$host]} cmake -DSIMDJSON_ENABLE_THREADS=ON -DSIMDJSON_COMPETITION=OFF -DSIMDJSON_GOOGLE_BENCHMARKS=OFF $cmake_options $root"
  else
    host_configure_command[$host]="echo \"Skipping cmake\""
  fi
done

#
# Build
#
build=${build:-$update_source}
if [ "$build" = "1" ]; then
  # The sed command below is used to make the command 
  build_command="echo \"Building\" && cmake --build . --clean-first --target parse 2>&1 | sed \"s/simdjson-remote/simdjson/g\""
else
  build_command="echo \"Skipping build\""
fi

#
# Benchmark
#
implementation=${implementation:-}
iterations=${iterations:-5000}
examples=${examples:-jsonexamples/twitter.json}
benchmark_output=$build_root/output.txt
benchmark_command="benchmark/parse -i 200 -n $iterations $examples > $benchmark_output"
if [ -n "$implementation" ]; then
  benchmark_command="SIMDJSON_FORCE_IMPLEMENTATION=$implementation $benchmark_command"
fi
benchmark_command="echo \"Benchmarking\" && $benchmark_command"

#
# Run configure+build+benchmark
#
if [ -n "$build" ]; then
  echo ""
  echo ""
  echo "Build and run ..."
  echo ""
  echo ""
  for host in $hosts; do
    ssh $host ". ~/.profile && $prepare_build_root && cd $build_root && ${host_configure_command[$host]} && $build_command && $benchmark_command" &
  done
  for job in `jobs -p`; do wait $job; done
fi

#
# Print results
#
echo ""
for host in $hosts; do
  echo ""
  echo "$host:"
  ssh $host cat $benchmark_output
done
