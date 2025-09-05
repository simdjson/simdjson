#!/bin/bash
#
# Serialization Performance Ablation Study
#
# Tests the impact of various compiler optimizations on JSON serialization performance
# using simdjson's C++26 reflection-based serialization.
#
# Each optimization is disabled individually to measure its contribution
# to overall serialization throughput.
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/build"
ABLATION_DIR="$ROOT_DIR/ablation"
RESULTS_DIR="$ABLATION_DIR/results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  JSON Serialization Ablation Study${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Create results directory
mkdir -p "$RESULTS_DIR"

# Define ablation variants
declare -A variants=(
    ["baseline"]=""
    ["no_consteval"]="-DSIMDJSON_ABLATION_NO_CONSTEVAL"
    ["no_simd_escaping"]="-DSIMDJSON_ABLATION_NO_SIMD_ESCAPING"
    ["no_fast_digits"]="-DSIMDJSON_ABLATION_NO_FAST_DIGITS"
    ["no_branch_hints"]="-DSIMDJSON_ABLATION_NO_BRANCH_HINTS"
    ["linear_growth"]="-DSIMDJSON_ABLATION_LINEAR_GROWTH"
)

# Function to build and test serialization
test_serialization_variant() {
    local variant_name=$1
    local flags=$2

    echo -e "${YELLOW}Testing variant: $variant_name${NC}"

    # Configure and build with CMake
    cd "$BUILD_DIR"

    echo "  Configuring CMake..."
    rm -f CMakeCache.txt
    if ! env CXX=/usr/local/bin/clang++ CC=/usr/local/bin/clang cmake .. \
         -DCMAKE_CXX_FLAGS="$flags -O3" \
         -DSIMDJSON_DEVELOPER_MODE=ON \
         -DSIMDJSON_STATIC_REFLECTION=ON \
         -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1; then
        echo -e "  ${RED}ERROR: CMake configuration failed for $variant_name${NC}"
        return 1
    fi

    echo "  Building serialization benchmarks..."
    if ! make benchmark_serialization_twitter benchmark_serialization_citm_catalog -j4 > /dev/null 2>&1; then
        echo -e "  ${RED}ERROR: Build failed for $variant_name${NC}"
        return 1
    fi

    # Run Twitter serialization benchmark
    echo "  Running Twitter serialization benchmark..."
    twitter_output=$(./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection 2>&1)
    twitter_result=$(echo "$twitter_output" | grep "bench_simdjson_static_reflection" | grep -o '[0-9]*\.[0-9]* MB/s' || echo "FAILED")

    # Run CITM serialization benchmark
    echo "  Running CITM serialization benchmark..."
    citm_output=$(./benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog -f simdjson_static_reflection 2>&1)
    citm_result=$(echo "$citm_output" | grep "bench_simdjson_static_reflection" | grep -o '[0-9]*\.[0-9]* MB/s' || echo "FAILED")

    # Store results
    echo "$variant_name,twitter,$twitter_result" >> "$RESULTS_DIR/serialization_results.csv"
    echo "$variant_name,citm,$citm_result" >> "$RESULTS_DIR/serialization_results.csv"

    # Display results
    echo -e "  ${GREEN}Results:${NC}"
    echo "    Twitter: $twitter_result"
    echo "    CITM:    $citm_result"
    echo ""
}

# Initialize results file
echo "variant,dataset,throughput" > "$RESULTS_DIR/serialization_results.csv"

# Run tests for each variant
for variant in baseline no_consteval no_simd_escaping no_fast_digits no_branch_hints linear_growth; do
    test_serialization_variant "$variant" "${variants[$variant]}"
done

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE} Serialization Ablation Study Complete${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Display summary
echo "Results saved to: $RESULTS_DIR/serialization_results.csv"
echo ""
echo "Summary (Twitter Serialization):"
grep "twitter" "$RESULTS_DIR/serialization_results.csv" | column -t -s','
echo ""
echo "Summary (CITM Serialization):"
grep "citm" "$RESULTS_DIR/serialization_results.csv" | column -t -s','