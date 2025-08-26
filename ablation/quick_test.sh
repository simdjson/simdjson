#!/bin/bash

# Quick test of ablation setup
set -e

echo "=== Quick Ablation Test ==="
echo "Testing if benchmarks can be built and run..."
echo ""

BUILD_DIR="/Users/random_person/Desktop/simdjson/build"
cd "$BUILD_DIR"

# First ensure we have a clean baseline build (no ablation flags)
echo "Ensuring baseline build (no ablation flags)..."
if grep -q "CMAKE_CXX_FLAGS:STRING=-D" CMakeCache.txt 2>/dev/null; then
    echo "Detected ablation flags in build, reconfiguring..."
    rm -f CMakeCache.txt
    env CXX=/usr/local/bin/clang++ CC=/usr/local/bin/clang cmake .. \
        -DSIMDJSON_DEVELOPER_MODE=ON \
        -DSIMDJSON_STATIC_REFLECTION=ON \
        -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
    make clean > /dev/null 2>&1
    make benchmark_serialization_twitter benchmark_serialization_citm_catalog -j4 > /dev/null 2>&1
fi

# Test Twitter benchmark
echo "Testing Twitter benchmark..."
if [ -x "./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter" ]; then
    result=$(./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection 2>&1 | grep "bench_simdjson_static_reflection" | grep -o '[0-9]*\.[0-9]* MB/s' | grep -o '[0-9]*\.[0-9]*' | head -1)
    if [ -n "$result" ]; then
        echo "✓ Twitter benchmark works: $result MB/s"
    else
        echo "✗ Twitter benchmark failed to produce output"
        exit 1
    fi
else
    echo "✗ Twitter benchmark not found. Building..."
    make benchmark_serialization_twitter
    echo "Retry after building..."
    result=$(./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection 2>&1 | grep "bench_simdjson_static_reflection" | grep -o '[0-9]*\.[0-9]* MB/s' | grep -o '[0-9]*\.[0-9]*' | head -1)
    if [ -n "$result" ]; then
        echo "✓ Twitter benchmark works: $result MB/s"
    else
        echo "✗ Twitter benchmark failed"
        exit 1
    fi
fi

# Test CITM benchmark
echo "Testing CITM benchmark..."
if [ -x "./benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog" ]; then
    result=$(./benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog -f simdjson_static_reflection 2>&1 | grep "bench_simdjson_static_reflection" | grep -o '[0-9]*\.[0-9]* MB/s' | grep -o '[0-9]*\.[0-9]*' | head -1)
    if [ -n "$result" ]; then
        echo "✓ CITM benchmark works: $result MB/s"
    else
        echo "✗ CITM benchmark failed to produce output"
        exit 1
    fi
else
    echo "✗ CITM benchmark not found. Building..."
    make benchmark_serialization_citm_catalog
    echo "Retry after building..."
    result=$(./benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog -f simdjson_static_reflection 2>&1 | grep "bench_simdjson_static_reflection" | grep -o '[0-9]*\.[0-9]* MB/s' | grep -o '[0-9]*\.[0-9]*' | head -1)
    if [ -n "$result" ]; then
        echo "✓ CITM benchmark works: $result MB/s"
    else
        echo "✗ CITM benchmark failed"
        exit 1
    fi
fi

echo ""
echo "✓ All tests passed!"
echo ""
echo "You can now run the full ablation study with:"
echo "  ./ablation/run_ablation_study.sh"
echo ""
echo "For a quick run (2 iterations each):"
echo "  ./ablation/run_ablation_study.sh --runs-twitter 2 --runs-citm 2"