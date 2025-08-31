#!/bin/bash

# Build script for the unified JSON benchmark
# This compiles the benchmark with all available libraries

echo "Building Unified JSON Benchmark..."

# Check for dependencies directories
NLOHMANN_PATH=""
RAPIDJSON_PATH=""
SERDE_PATH=""

# Try multiple possible locations for dependencies
for dir in build build20 build26; do
    if [ -d "$dir/_deps/nlohmann_json-src/include" ]; then
        NLOHMANN_PATH="-I./$dir/_deps/nlohmann_json-src/include -DHAS_NLOHMANN"
        echo "✓ Found nlohmann/json in $dir"
        break
    fi
done

if [ -z "$NLOHMANN_PATH" ]; then
    echo "✗ nlohmann/json not found"
fi

for dir in build build20 build26; do
    if [ -d "$dir/_deps/rapidjson-src/include" ]; then
        RAPIDJSON_PATH="-I./$dir/_deps/rapidjson-src/include -DHAS_RAPIDJSON"
        echo "✓ Found RapidJSON in $dir"
        break
    fi
done

if [ -z "$RAPIDJSON_PATH" ]; then
    echo "✗ RapidJSON not found"
fi

# Check for Serde benchmark library (.so or .a)
if [ -f "benchmark/static_reflect/serde-benchmark/target/release/libserde_benchmark.so" ] || [ -f "benchmark/static_reflect/serde-benchmark/target/release/libserde_benchmark.a" ]; then
    SERDE_PATH="-L./benchmark/static_reflect/serde-benchmark/target/release -lserde_benchmark -ldl -lpthread -DHAS_SERDE"
    echo "✓ Found Serde benchmark library"
else
    echo "✗ Serde benchmark library not found"
    echo "  To build it: cd benchmark/static_reflect/serde-benchmark && cargo build --release"
fi

# Check for yyjson
YYJSON_PATH=""
YYJSON_LIB=""
if [ -d "build/_deps/yyjson-src/src" ] || [ -f "build/dependencies/libyyjson.a" ]; then
    if [ -f "build/dependencies/libyyjson.a" ]; then
        YYJSON_PATH="-I./build/_deps/yyjson-src/src -DHAS_YYJSON"
        YYJSON_LIB="build/dependencies/libyyjson.a"
        echo "✓ Found yyjson library"
    elif [ -f "build/_deps/yyjson-build/libyyjson.a" ]; then
        YYJSON_PATH="-I./build/_deps/yyjson-src/src -DHAS_YYJSON"
        YYJSON_LIB="build/_deps/yyjson-build/libyyjson.a"
        echo "✓ Found yyjson library"
    fi
else
    echo "✗ yyjson not found"
fi

# Note: reflect-cpp disabled due to complex linking requirements
# REFLECTCPP_PATH=""

# Compile the benchmark
clang++ -std=c++26 \
    -freflection \
    -fexpansion-statements \
    -stdlib=libc++ \
    -DSIMDJSON_STATIC_REFLECTION=1 \
    -DSIMDJSON_EXCEPTIONS=1 \
    -I./include \
    -I./benchmark/static_reflect/serde-benchmark \
    $NLOHMANN_PATH \
    $RAPIDJSON_PATH \
    $YYJSON_PATH \
    -O3 \
    benchmark/unified_benchmark.cpp \
    singleheader/simdjson.cpp \
    $YYJSON_LIB \
    $SERDE_PATH \
    -o benchmark/unified_benchmark

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful! Run with: ./benchmark/unified_benchmark"
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
    if [ ! -z "$SERDE_PATH" ]; then
        echo "  - Serde (Rust)"
    fi
    if [ ! -z "$REFLECTCPP_PATH" ]; then
        echo "  - reflect-cpp"
    fi
else
    echo "Build failed!"
    exit 1
fi