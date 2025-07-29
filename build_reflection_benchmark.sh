#!/bin/bash

# Clean build script for simdjson reflection benchmark
set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Navigate to simdjson root directory (where this script is located)
cd "$SCRIPT_DIR"

# Clean any existing build
rm -rf build

# Create new build directory
mkdir build
cd build

# Configure with the specified settings
cmake .. \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DSIMDJSON_DEVELOPER_MODE=ON \
  -DSIMDJSON_STATIC_REFLECTION=ON \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_BUILD_TYPE=Release

# Build the specific target
make benchmark_serialization_twitter

echo "Build completed successfully!"
echo "To run the benchmark with simdjson static reflection filter, use:"
echo "./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection"