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

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   JSON SERIALIZATION Benchmarks${NC}"
echo -e "${BLUE}   (C++ structs → JSON)${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found!${NC}"
    echo "Please build the project first with:"
    echo "  mkdir build && cd build"
    echo "  cmake .. -DSIMDJSON_DEVELOPER_MODE=ON -DSIMDJSON_STATIC_REFLECTION=ON"
    echo "  make"
    exit 1
fi

cd "$BUILD_DIR"

# Check if benchmarks exist
TWITTER_BENCH="benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter"
CITM_BENCH="benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog"

if [ ! -f "$TWITTER_BENCH" ] || [ ! -f "$CITM_BENCH" ]; then
    echo -e "${YELLOW}Benchmarks not found. Building...${NC}"
    make benchmark_serialization_twitter benchmark_serialization_citm_catalog -j4
fi

# Parse command line arguments
FILTER=""
DATASET="all"

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
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -f, --filter LIBS    Run only specified libraries (comma-separated)"
            echo "                       Options: nlohmann,simdjson_static_reflection,simdjson_to,rust,reflect_cpp"
            echo "  -d, --dataset NAME   Run only specified dataset (twitter, citm, or all)"
            echo "  -h, --help          Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                                    # Run all benchmarks"
            echo "  $0 -f simdjson_to,rust                # Compare simdjson::to with Serde"
            echo "  $0 -d twitter                         # Run only Twitter dataset"
            echo "  $0 -f rust -d citm                    # Run only Serde on CITM"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Run Twitter benchmark
if [ "$DATASET" = "all" ] || [ "$DATASET" = "twitter" ]; then
    echo -e "${GREEN}=== Twitter Serialization Benchmark (631KB) ===${NC}"
    echo ""
    if [ -n "$FILTER" ]; then
        ./$TWITTER_BENCH -f "$FILTER"
    else
        ./$TWITTER_BENCH
    fi
    echo ""
fi

# Run CITM benchmark
if [ "$DATASET" = "all" ] || [ "$DATASET" = "citm" ]; then
    echo -e "${GREEN}=== CITM Serialization Benchmark (1.7MB) ===${NC}"
    echo ""
    if [ -n "$FILTER" ]; then
        ./$CITM_BENCH -f "$FILTER"
    else
        ./$CITM_BENCH
    fi
    echo ""
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Serialization Benchmarks Complete${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Key metrics to compare:"
echo "  - Throughput (MB/s) - Higher is better"
echo "  - simdjson_static_reflection uses low-level builder API"
echo "  - simdjson_to uses high-level convenient API"
echo "  - rust uses serde_json::to_string()"
echo ""
echo "Expected performance ranking:"
echo "  1. simdjson_static_reflection (~3.2 GB/s Twitter, ~2.4 GB/s CITM)"
echo "  2. simdjson_to (~2.8 GB/s Twitter, ~2.1 GB/s CITM)"
echo "  3. Serde/rust (~1.7 GB/s Twitter, ~1.2 GB/s CITM)"
echo "  4. reflect-cpp (~1.5 GB/s Twitter, ~1.2 GB/s CITM)"
echo "  5. nlohmann (~0.18 GB/s Twitter, ~0.10 GB/s CITM)"