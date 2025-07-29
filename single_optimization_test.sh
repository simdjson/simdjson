#!/bin/bash

# Single optimization testing script
# Tests one optimization at a time with before/after measurements

set -e

VARIANT_NAME="$1"
CMAKE_FLAGS="$2"
DESCRIPTION="$3"

if [ -z "$VARIANT_NAME" ]; then
    echo "Usage: $0 <variant_name> <cmake_flags> <description>"
    echo "Example: $0 \"SIMD Numbers\" \"-DSIMDJSON_ENABLE_SIMD_NUMBERS=ON\" \"SIMD-accelerated number serialization\""
    exit 1
fi

echo "=== Testing: $VARIANT_NAME ==="
echo "Description: $DESCRIPTION"
echo "Flags: $CMAKE_FLAGS"
echo ""

# Clean build
rm -rf build
mkdir build
cd build

# Configure
echo "Configuring build..."
cmake -DCMAKE_CXX_COMPILER=clang++ \
      -DSIMDJSON_DEVELOPER_MODE=ON \
      -DSIMDJSON_STATIC_REFLECTION=ON \
      -DBUILD_SHARED_LIBS=OFF \
      $CMAKE_FLAGS \
      .. > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "❌ Configuration failed"
    exit 1
fi

# Build
echo "Building..."
start_time=$(date +%s.%N)
if cmake --build . --target benchmark_serialization_twitter > /dev/null 2>&1; then
    end_time=$(date +%s.%N)
    compile_time=$(python3 -c "print(f'{$end_time - $start_time:.2f}')")
    echo "✅ Build successful (${compile_time}s)"
else
    echo "❌ Build failed"
    exit 1
fi

# Run benchmark 3 times
echo "Running benchmarks..."
total_throughput=0
for i in {1..3}; do
    result=$(./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection 2>/dev/null)
    echo "Run $i: $result"
    throughput=$(echo "$result" | grep "bench_simdjson_static_reflection" | awk '{print $3}' | sed 's/MB\/s//')
    total_throughput=$(python3 -c "print($total_throughput + $throughput)")
done

avg_throughput=$(python3 -c "print(f'{$total_throughput / 3:.2f}')")
echo ""
echo "📊 Average throughput: ${avg_throughput} MB/s"
echo "⏱️  Compilation time: ${compile_time}s"
echo ""

cd ..