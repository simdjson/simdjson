#!/bin/bash

# Build script for the unified JSON benchmark
# This compiles the benchmark with all available libraries

echo "Building Unified JSON Benchmark..."

# Check for dependencies directories
NLOHMANN_PATH=""
RAPIDJSON_PATH=""

if [ -d "build20/_deps/nlohmann_json-src/include" ]; then
    NLOHMANN_PATH="-I./build20/_deps/nlohmann_json-src/include -DHAS_NLOHMANN"
    echo "✓ Found nlohmann/json"
else
    echo "✗ nlohmann/json not found"
fi

if [ -d "build20/_deps/rapidjson-src/include" ]; then
    RAPIDJSON_PATH="-I./build20/_deps/rapidjson-src/include -DHAS_RAPIDJSON"
    echo "✓ Found RapidJSON"
else
    echo "✗ RapidJSON not found"
fi

# Compile the benchmark
clang++ -std=c++26 \
    -freflection \
    -DSIMDJSON_STATIC_REFLECTION=1 \
    -DSIMDJSON_EXCEPTIONS=1 \
    -DSIMDJSON_ABLATION_NO_CONSTEVAL \
    -I./include \
    $NLOHMANN_PATH \
    $RAPIDJSON_PATH \
    -O3 \
    benchmark_comparison_unified.cpp \
    singleheader/simdjson.cpp \
    -o benchmark_comparison_unified

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful! Run with: ./benchmark_comparison_unified"
    echo ""
    echo "The benchmark will test:"
    echo "  - Twitter dataset (631KB)"
    echo "  - CITM Catalog dataset (1.7MB)"
    echo ""
    echo "With the following methods:"
    echo "  - simdjson manual parsing"
    echo "  - simdjson reflection parsing"
    echo "  - simdjson::from() API"
    if [ ! -z "$NLOHMANN_PATH" ]; then
        echo "  - nlohmann/json"
    fi
    if [ ! -z "$RAPIDJSON_PATH" ]; then
        echo "  - RapidJSON"
    fi
else
    echo "Build failed!"
    exit 1
fi