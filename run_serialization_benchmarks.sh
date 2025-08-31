#!/bin/bash
#
# Top-level script to run all JSON SERIALIZATION benchmarks
# Tests: C++ structs → JSON performance
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

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}   JSON SERIALIZATION Benchmarks${NC}"
echo -e "${BLUE}   (C++ structs → JSON)${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""

# Always rebuild unified benchmark to ensure it's up to date
echo -e "${YELLOW}Building unified benchmark with all available libraries...${NC}"

# First check and build Serde if needed
if [ ! -f "benchmark/static_reflect/serde-benchmark/target/release/libserde_benchmark.so" ] && \
   [ ! -f "benchmark/static_reflect/serde-benchmark/target/release/libserde_benchmark.a" ]; then
    echo -e "${YELLOW}Building Serde benchmark library...${NC}"
    if [ -d "benchmark/static_reflect/serde-benchmark" ]; then
        cd benchmark/static_reflect/serde-benchmark
        cargo build --release
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Serde benchmark built successfully${NC}"
        else
            echo -e "${YELLOW}⚠ Warning: Serde benchmark build failed - will skip Serde tests${NC}"
        fi
        cd ../../..
    else
        echo -e "${YELLOW}⚠ Serde benchmark directory not found - will skip Serde tests${NC}"
    fi
fi

# Build unified benchmark
./build_unified_benchmark.sh
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to build unified benchmark${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Unified benchmark built successfully${NC}"

# Parse command line arguments
FILTER=""
DATASET="all"
MIN_TIME=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -f|--filter)
            FILTER="$2"
            shift 2
            ;;
        -d|--dataset)
            DATASET="$2"
            shift 2
            ;;
        -t|--min-time)
            MIN_TIME="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -f, --filter LIBS    Run only specified libraries (comma-separated)"
            echo "                       Options: simdjson,simdjson_reflection,nlohmann,rapidjson,yyjson,serde"
            echo "  -d, --dataset NAME   Run only specified dataset (twitter, citm, or all)"
            echo "  -t, --min-time SECS  Minimum benchmark time per test (default: auto)"
            echo "  -h, --help          Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                                    # Run all benchmarks"
            echo "  $0 -f simdjson_reflection,yyjson      # Compare simdjson reflection with yyjson"
            echo "  $0 -d twitter                         # Run only Twitter dataset"
            echo "  $0 -f serde -d citm                   # Run only Serde on CITM"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Run the unified benchmark with serialization flag
echo -e "${GREEN}Running Unified Serialization Benchmark${NC}"
echo ""

# Set library path for Serde if it exists
if [ -f "benchmark/static_reflect/serde-benchmark/target/release/libserde_benchmark.so" ]; then
    export LD_LIBRARY_PATH="$SCRIPT_DIR/benchmark/static_reflect/serde-benchmark/target/release:$LD_LIBRARY_PATH"
fi

# The unified benchmark handles both datasets internally
# Pass --serialization flag to run serialization benchmarks
./benchmark/unified_benchmark --serialization

echo ""
echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}   Serialization Benchmarks Complete${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""
echo "Key metrics to compare:"
echo "  - Throughput (MB/s) - Higher is better"
echo "  - simdjson uses DOM-based serialization"
echo "  - simdjson (reflection) uses C++26 static reflection"
echo "  - yyjson uses optimized C serialization"
echo "  - Serde (Rust) uses serde_json::to_string()"
echo ""
echo "Expected performance ranking:"
echo "  1. yyjson (~3.0 GB/s Twitter, ~2.2 GB/s CITM)"
echo "  2. simdjson (reflection) (~2.8 GB/s Twitter, ~2.0 GB/s CITM)"
echo "  3. simdjson (DOM) (~2.5 GB/s Twitter, ~1.8 GB/s CITM)"
echo "  4. RapidJSON (~1.5 GB/s Twitter, ~1.2 GB/s CITM)"
echo "  5. Serde (Rust) (~1.2 GB/s Twitter, ~0.9 GB/s CITM)"
echo "  6. nlohmann (~0.4 GB/s Twitter, ~0.3 GB/s CITM)"