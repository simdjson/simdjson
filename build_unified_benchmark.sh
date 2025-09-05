#!/usr/bin/env bash
set -euo pipefail

# Build script for the unified JSON benchmark
# Usage:
#   ./build_unified_benchmark.sh [build-dir]
# If no build-dir is provided, defaults to "buildreflect".

BUILD_DIR="${1:-buildreflect}"

echo "Building Unified JSON Benchmark into directory: ${BUILD_DIR}"

# Configure CMake with recommended flags
CMAKE_ARGS=(
  -B "${BUILD_DIR}"
  -S .
  -D SIMDJSON_STATIC_REFLECTION=ON
  -D SIMDJSON_DEVELOPER_MODE=ON
)

# Run cmake configure step only if the build directory did not previously exist
if [ ! -d "${BUILD_DIR}" ]; then
  echo "Creating build directory ${BUILD_DIR} and running CMake configure"
  mkdir -p "${BUILD_DIR}"
  CXX=clang++ CC=clang cmake "${CMAKE_ARGS[@]}"
else
  echo "Build directory '${BUILD_DIR}' already exists — skipping CMake configure step."
fi


cmake --build "${BUILD_DIR}"  -j 4 --target yyjson benchmark_serialization_citm_catalog benchmark_serialization_twitter

# Try multiple possible locations for dependencies (prefer the chosen build dir)
NLOHMANN_PATH=""
RAPIDJSON_PATH=""
SERDE_PATH=""
YYJSON_PATH=""
YYJSON_LIB=""

# Search helper
_search_dir_list=("${BUILD_DIR}" build build20 build26)

for dir in "${_search_dir_list[@]}"; do
    if [ -d "${dir}/_deps/nlohmann_json-src/include" ]; then
        NLOHMANN_PATH="-I./${dir}/_deps/nlohmann_json-src/include -DHAS_NLOHMANN"
        echo "✓ Found nlohmann/json in ${dir}"
        break
    fi
done
if [ -z "${NLOHMANN_PATH}" ]; then
    echo "✗ nlohmann/json not found"
fi

for dir in "${_search_dir_list[@]}"; do
    if [ -d "${dir}/_deps/rapidjson-src/include" ]; then
        RAPIDJSON_PATH="-I./${dir}/_deps/rapidjson-src/include -DHAS_RAPIDJSON"
        echo "✓ Found RapidJSON in ${dir}"
        break
    fi
done
if [ -z "${RAPIDJSON_PATH}" ]; then
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

# Check for yyjson inside the chosen build dir or other likely locations
if [ -f "${BUILD_DIR}/dependencies/libyyjson.a" ]; then
    YYJSON_PATH="-I./${BUILD_DIR}/_deps/yyjson-src/src -DHAS_YYJSON"
    YYJSON_LIB="${BUILD_DIR}/dependencies/libyyjson.a"
    echo "✓ Found yyjson library in ${BUILD_DIR}"
elif [ -f "${BUILD_DIR}/_deps/yyjson-build/libyyjson.a" ]; then
    YYJSON_PATH="-I./${BUILD_DIR}/_deps/yyjson-src/src -DHAS_YYJSON"
    YYJSON_LIB="${BUILD_DIR}/_deps/yyjson-build/libyyjson.a"
    echo "✓ Found yyjson library in ${BUILD_DIR}"
elif [ -f "build/dependencies/libyyjson.a" ]; then
    YYJSON_PATH="-I./build/_deps/yyjson-src/src -DHAS_YYJSON"
    YYJSON_LIB="build/dependencies/libyyjson.a"
    echo "✓ Found yyjson library in build"
else
    echo "✗ yyjson not found"
fi

echo "Compiling unified benchmark... (this can take a while)"
# Compile the unified benchmark (single-file invocation)
clang++ -std=c++26 \
    -freflection \
    -fexpansion-statements \
    -stdlib=libc++ \
    -DSIMDJSON_STATIC_REFLECTION=1 \
    -DSIMDJSON_EXCEPTIONS=1 \
    -I./include \
    -I./src \
    -I./benchmark/static_reflect/serde-benchmark \
    ${NLOHMANN_PATH} \
    ${RAPIDJSON_PATH} \
    ${YYJSON_PATH} \
    -O3 \
    benchmark/unified_benchmark.cpp \
    src/simdjson.cpp \
    ${YYJSON_LIB} \
    ${SERDE_PATH} \
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
    if [ ! -z "${NLOHMANN_PATH}" ]; then
        echo "  - nlohmann/json"
    fi
    if [ ! -z "${RAPIDJSON_PATH}" ]; then
        echo "  - RapidJSON"
    fi
    if [ ! -z "${SERDE_PATH}" ]; then
        echo "  - Serde (Rust)"
    fi
else
    echo "Build failed!"
    exit 1
fi