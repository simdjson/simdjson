#!/bin/bash
#
# Unified JSON Benchmark Script
# Supports both parsing and serialization benchmarks for Twitter and CITM datasets
# Always uses C++26 reflection support
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default options
RUN_PARSING=false
RUN_SERIALIZATION=false
RUN_TWITTER=false
RUN_CITM=false
FORCE_REBUILD=false
CLEAN_BUILD=false
FILTER=""

# Function to print usage
print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --parsing          Run parsing benchmarks (JSON → C++ structs)"
    echo "  --serialization    Run serialization benchmarks (C++ structs → JSON)"
    echo "  --twitter          Run Twitter dataset benchmarks"
    echo "  --citm             Run CITM catalog dataset benchmarks"
    echo "  --all              Run all benchmarks (default if no options)"
    echo "  -f <filter>        Filter specific libraries (comma-separated)"
    echo "                     e.g., -f \"simdjson_to,nlohmann,rust\""
    echo "  --rebuild          Force rebuild even if executables exist"
    echo "  --clean            Clean build directory before building"
    echo "  --help, -h         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --all                                  # Run everything"
    echo "  $0 --parsing --twitter                    # Twitter parsing only"
    echo "  $0 --serialization --citm -f simdjson_to  # CITM serialization, simdjson only"
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --parsing)
            RUN_PARSING=true
            shift
            ;;
        --serialization)
            RUN_SERIALIZATION=true
            shift
            ;;
        --twitter)
            RUN_TWITTER=true
            shift
            ;;
        --citm)
            RUN_CITM=true
            shift
            ;;
        --all)
            RUN_PARSING=true
            RUN_SERIALIZATION=true
            RUN_TWITTER=true
            RUN_CITM=true
            shift
            ;;
        -f)
            FILTER="$2"
            shift 2
            ;;
        --rebuild)
            FORCE_REBUILD=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --help|-h)
            print_usage
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            print_usage
            exit 1
            ;;
    esac
done

# If no options specified, run all
if [ "$RUN_PARSING" = false ] && [ "$RUN_SERIALIZATION" = false ]; then
    RUN_PARSING=true
    RUN_SERIALIZATION=true
fi

if [ "$RUN_TWITTER" = false ] && [ "$RUN_CITM" = false ]; then
    RUN_TWITTER=true
    RUN_CITM=true
fi

echo -e "${BLUE}=====================================${NC}"
echo -e "${BLUE}    Unified JSON Benchmarks${NC}"
echo -e "${BLUE}    With C++26 Reflection Support${NC}"
echo -e "${BLUE}=====================================${NC}"
echo ""

# Show configuration
echo -e "${YELLOW}Configuration:${NC}"
echo "  Parsing benchmarks: $([ "$RUN_PARSING" = true ] && echo "YES" || echo "NO")"
echo "  Serialization benchmarks: $([ "$RUN_SERIALIZATION" = true ] && echo "YES" || echo "NO")"
echo "  Twitter dataset: $([ "$RUN_TWITTER" = true ] && echo "YES" || echo "NO")"
echo "  CITM dataset: $([ "$RUN_CITM" = true ] && echo "YES" || echo "NO")"
[ -n "$FILTER" ] && echo "  Filter: $FILTER"
echo ""

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Function to check if executable exists and is up to date
needs_rebuild() {
    local executable="$1"

    if [ "$FORCE_REBUILD" = true ]; then
        return 0  # Need rebuild
    fi

    if [ ! -f "$executable" ]; then
        return 0  # Need rebuild
    fi

    return 1  # No rebuild needed
}

# Function to configure CMake with reflection
configure_cmake() {
    echo -e "${YELLOW}Configuring CMake with C++26 reflection...${NC}"
    cd "$BUILD_DIR"

    CXX=/usr/local/bin/clang++ CC=/usr/local/bin/clang \
    CXXFLAGS="-std=c++26 -freflection" \
    cmake .. \
        -DSIMDJSON_DEVELOPER_MODE=ON \
        -DSIMDJSON_COMPETITION=ON \
        -DSIMDJSON_STATIC_REFLECTION=ON \
        -DSIMDJSON_RUST_VERSION=ON \
        -DSIMDJSON_COMPETITION_RAPIDJSON=ON \
        -DSIMDJSON_COMPETITION_YYJSON=ON \
        -G "Unix Makefiles"

    if [ $? -ne 0 ]; then
        echo -e "${RED}CMake configuration failed${NC}"
        exit 1
    fi

    echo -e "${GREEN}✓ CMake configured successfully${NC}"
}

# Function to build Rust/Serde library
build_rust_library() {
    if command -v cargo &> /dev/null; then
        if [ -f "$SCRIPT_DIR/benchmark/static_reflect/serde-benchmark/Cargo.toml" ]; then
            echo -e "${YELLOW}Building Rust/Serde benchmark library...${NC}"
            cd "$SCRIPT_DIR/benchmark/static_reflect/serde-benchmark"
            cargo build --release --lib

            # Copy library to build directory
            if [ -f "target/release/libserde_benchmark.so" ]; then
                mkdir -p "$BUILD_DIR/benchmark/static_reflect"
                cp target/release/libserde_benchmark.so "$BUILD_DIR/benchmark/static_reflect/"
                echo -e "${GREEN}✓ Rust/Serde library built${NC}"
            elif [ -f "target/release/libserde_benchmark.dylib" ]; then
                mkdir -p "$BUILD_DIR/benchmark/static_reflect"
                cp target/release/libserde_benchmark.dylib "$BUILD_DIR/benchmark/static_reflect/libserde_benchmark.so"
                echo -e "${GREEN}✓ Rust/Serde library built${NC}"
            fi
        fi
    fi
}

# Function to build a specific benchmark
build_benchmark() {
    local target="$1"
    local description="$2"

    echo -e "${YELLOW}Building $description...${NC}"
    cd "$BUILD_DIR"

    # First build dependencies
    make simdjson yyjson -j4 2>/dev/null || true

    # Build the target
    make "$target" -j4

    if [ $? -ne 0 ]; then
        echo -e "${RED}Build failed for $target${NC}"
        return 1
    fi

    echo -e "${GREEN}✓ $description built successfully${NC}"
    return 0
}

# Function to run a benchmark
run_benchmark() {
    local executable="$1"
    local dataset="$2"
    local type="$3"

    if [ ! -f "$executable" ]; then
        echo -e "${RED}Executable not found: $executable${NC}"
        return 1
    fi

    echo ""
    echo -e "${BLUE}=== $dataset $type Benchmark ===${NC}"

    # Set library path for Rust/Serde
    export LD_LIBRARY_PATH="$BUILD_DIR/benchmark/static_reflect:$LD_LIBRARY_PATH"
    export DYLD_LIBRARY_PATH="$BUILD_DIR/benchmark/static_reflect:$DYLD_LIBRARY_PATH"

    if [ -n "$FILTER" ]; then
        "$executable" -f "$FILTER"
    else
        "$executable"
    fi
}

# Ensure build directory exists
mkdir -p "$BUILD_DIR"

# Configure CMake if needed
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$CLEAN_BUILD" = true ]; then
    configure_cmake
fi

# Build Rust library if available
build_rust_library

# Twitter Parsing Benchmark
if [ "$RUN_PARSING" = true ] && [ "$RUN_TWITTER" = true ]; then
    TWITTER_PARSING="$BUILD_DIR/benchmark/static_reflect/twitter_benchmark/benchmark_parsing_twitter"

    if needs_rebuild "$TWITTER_PARSING"; then
        build_benchmark "benchmark_parsing_twitter" "Twitter parsing benchmark"
    else
        echo -e "${GREEN}Twitter parsing benchmark already built, skipping...${NC}"
    fi

    run_benchmark "$TWITTER_PARSING" "Twitter" "Parsing"
fi

# Twitter Serialization Benchmark
if [ "$RUN_SERIALIZATION" = true ] && [ "$RUN_TWITTER" = true ]; then
    TWITTER_SERIALIZATION="$BUILD_DIR/benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter"

    if needs_rebuild "$TWITTER_SERIALIZATION"; then
        build_benchmark "benchmark_serialization_twitter" "Twitter serialization benchmark"
    else
        echo -e "${GREEN}Twitter serialization benchmark already built, skipping...${NC}"
    fi

    run_benchmark "$TWITTER_SERIALIZATION" "Twitter" "Serialization"
fi

# CITM Parsing Benchmark
if [ "$RUN_PARSING" = true ] && [ "$RUN_CITM" = true ]; then
    CITM_PARSING="$BUILD_DIR/benchmark/static_reflect/citm_catalog_benchmark/benchmark_parsing_citm"

    if needs_rebuild "$CITM_PARSING"; then
        build_benchmark "benchmark_parsing_citm" "CITM parsing benchmark"
    else
        echo -e "${GREEN}CITM parsing benchmark already built, skipping...${NC}"
    fi

    run_benchmark "$CITM_PARSING" "CITM" "Parsing"
fi

# CITM Serialization Benchmark
if [ "$RUN_SERIALIZATION" = true ] && [ "$RUN_CITM" = true ]; then
    CITM_SERIALIZATION="$BUILD_DIR/benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog"

    if needs_rebuild "$CITM_SERIALIZATION"; then
        build_benchmark "benchmark_serialization_citm_catalog" "CITM serialization benchmark"
    else
        echo -e "${GREEN}CITM serialization benchmark already built, skipping...${NC}"
    fi

    run_benchmark "$CITM_SERIALIZATION" "CITM" "Serialization"
fi

echo ""
echo -e "${BLUE}=====================================${NC}"
echo -e "${BLUE}    Benchmarks Complete${NC}"
echo -e "${BLUE}=====================================${NC}"
echo ""

# Summary
echo "Libraries tested:"
echo "  - simdjson (with C++26 reflection)"
echo "  - simdjson (string_builder API)"
echo "  - nlohmann::json"
echo "  - rapidjson"
echo "  - yyjson"
echo "  - rust/serde"
echo ""
echo "Datasets tested:"
[ "$RUN_TWITTER" = true ] && echo "  - twitter.json (Twitter social media data)"
[ "$RUN_CITM" = true ] && echo "  - citm_catalog.json (CITM ticket catalog)"
echo ""
echo "Benchmark types:"
[ "$RUN_PARSING" = true ] && echo "  - Parsing: JSON → C++ struct performance"
[ "$RUN_SERIALIZATION" = true ] && echo "  - Serialization: C++ struct → JSON performance"
echo ""
echo "Higher MB/s values indicate better performance"