#!/bin/bash

# Build script for the unified benchmark
# Automatically detects available libraries and builds accordingly

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/build"

echo "=== Building Unified JSON Benchmark ==="
echo ""

# Check for clang++ with C++26 support
if ! command -v /usr/local/bin/clang++ &> /dev/null; then
    echo "Error: Clang++ with C++26 support not found at /usr/local/bin/clang++"
    echo "Please install the bloomberg/clang-p2996 compiler"
    exit 1
fi

# Detect available libraries
COMPILE_FLAGS="-std=c++26 -freflection -O3"
COMPILE_FLAGS="$COMPILE_FLAGS -DSIMDJSON_STATIC_REFLECTION=1"
COMPILE_FLAGS="$COMPILE_FLAGS -DSIMDJSON_EXCEPTIONS=1"
INCLUDES="-I$ROOT_DIR/include"

echo "Checking for optional libraries..."

# Check for nlohmann/json
if [ -d "$BUILD_DIR/_deps/nlohmann_json-src" ]; then
    echo "✓ Found nlohmann/json"
    COMPILE_FLAGS="$COMPILE_FLAGS -DHAS_NLOHMANN"
    INCLUDES="$INCLUDES -I$BUILD_DIR/_deps/nlohmann_json-src/include"
elif [ -d "$BUILD_DIR/build20/_deps/nlohmann_json-src" ]; then
    echo "✓ Found nlohmann/json (in build20)"
    COMPILE_FLAGS="$COMPILE_FLAGS -DHAS_NLOHMANN"
    INCLUDES="$INCLUDES -I$BUILD_DIR/build20/_deps/nlohmann_json-src/include"
else
    echo "✗ nlohmann/json not found (will skip nlohmann benchmarks)"
fi

# Check for RapidJSON
if [ -d "$BUILD_DIR/_deps/rapidjson-src" ]; then
    echo "✓ Found RapidJSON"
    COMPILE_FLAGS="$COMPILE_FLAGS -DHAS_RAPIDJSON"
    INCLUDES="$INCLUDES -I$BUILD_DIR/_deps/rapidjson-src/include"
elif [ -d "$BUILD_DIR/build20/_deps/rapidjson-src" ]; then
    echo "✓ Found RapidJSON (in build20)"
    COMPILE_FLAGS="$COMPILE_FLAGS -DHAS_RAPIDJSON"
    INCLUDES="$INCLUDES -I$BUILD_DIR/build20/_deps/rapidjson-src/include"
else
    echo "✗ RapidJSON not found (will skip RapidJSON benchmarks)"
fi

echo ""
echo "Compiling unified benchmark..."

# Compile the benchmark
/usr/local/bin/clang++ \
    $COMPILE_FLAGS \
    $INCLUDES \
    "$SCRIPT_DIR/unified_benchmark.cpp" \
    "$ROOT_DIR/singleheader/simdjson.cpp" \
    -o "$SCRIPT_DIR/unified_benchmark"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Running benchmark..."
    echo "==================="
    echo ""

    # Run the benchmark from the correct directory
    cd "$ROOT_DIR"
    "$SCRIPT_DIR/unified_benchmark"

    if [ $? -eq 0 ]; then
        echo ""
        echo "✓ Benchmark completed successfully!"
    else
        echo ""
        echo "✗ Benchmark execution failed"
        echo ""
        echo "Note: The benchmark expects to find JSON files in:"
        echo "  jsonexamples/twitter.json"
        echo "  jsonexamples/citm_catalog.json"
        exit 1
    fi
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi