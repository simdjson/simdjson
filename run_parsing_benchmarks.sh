#!/bin/bash
#
# Top-level script to run all JSON PARSING benchmarks
# Tests: JSON → C++ structs performance
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
echo -e "${BLUE}   JSON PARSING Benchmarks${NC}"
echo -e "${BLUE}   (JSON → C++ structs)${NC}"
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
            echo "                       Options: simdjson_manual,simdjson_reflection,simdjson_from,nlohmann,rapidjson,serde"
            echo "  -d, --dataset NAME   Run only specified dataset (twitter, citm, or all)"
            echo "  -t, --min-time SECS  Minimum benchmark time per test (default: auto)"
            echo "  -h, --help          Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                                    # Run all benchmarks"
            echo "  $0 -f simdjson_reflection,serde       # Compare simdjson reflection with Serde"
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

# Run the unified benchmark
echo -e "${GREEN}Running Unified Parsing Benchmark${NC}"
echo ""

# Set library path for Serde if it exists
if [ -f "benchmark/static_reflect/serde-benchmark/target/release/libserde_benchmark.so" ]; then
    export LD_LIBRARY_PATH="$SCRIPT_DIR/benchmark/static_reflect/serde-benchmark/target/release:$LD_LIBRARY_PATH"
fi

# The unified benchmark handles both datasets internally
# Pass --parsing flag to run parsing benchmarks (this is the default)
./benchmark/unified_benchmark --parsing

echo ""
echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}   Parsing Benchmarks Complete${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""
echo "Key metrics to compare:"
echo "  - Throughput (MB/s) - Higher is better"
echo "  - simdjson (manual) uses hand-written parsing code"
echo "  - simdjson (reflection) uses C++26 static reflection"
echo "  - simdjson::from() uses high-level convenient API"
echo "  - Serde (Rust) uses serde_json::from_str()"
echo ""
echo "Expected performance ranking:"
echo "  1. simdjson (manual) (~3.6 GB/s Twitter, ~2.7 GB/s CITM)"
echo "  2. simdjson (reflection) (~3.4 GB/s Twitter, ~2.5 GB/s CITM)"
echo "  3. simdjson::from() (~3.2 GB/s Twitter, ~2.3 GB/s CITM)"
echo "  4. Serde (Rust) (~1.9 GB/s Twitter, ~1.4 GB/s CITM)"
echo "  5. RapidJSON (~0.5 GB/s Twitter, ~0.4 GB/s CITM)"
echo "  6. nlohmann (~0.25 GB/s Twitter, ~0.15 GB/s CITM)"