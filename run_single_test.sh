#!/bin/bash

# Simple test runner for measuring optimization impact
# Usage: ./run_single_test.sh "Test Name" "CMAKE_FLAGS"

set -e

TEST_NAME="$1"
CMAKE_FLAGS="$2"

echo "=== Testing: $TEST_NAME ==="
echo "CMake flags: $CMAKE_FLAGS"

# Clean and build
rm -rf build
mkdir build
cd build

# Configure
cmake -DCMAKE_CXX_COMPILER=clang++ \
      -DSIMDJSON_DEVELOPER_MODE=ON \
      -DSIMDJSON_STATIC_REFLECTION=ON \
      -DBUILD_SHARED_LIBS=OFF \
      $CMAKE_FLAGS \
      ..

# Build
cmake --build . --target benchmark_serialization_twitter

# Run benchmark (single run for now)
echo "Running benchmark..."
./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection

cd ..