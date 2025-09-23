#!/bin/bash
#
# Comprehensive Ablation Study Script for simdjson
#
# This script performs runtime performance ablation studies on simdjson's JSON processing
# for both serialization and parsing (deserialization)
#
# Features:
# - Smart build detection (skips rebuilding if binaries exist)
# - Support for both Twitter and CITM datasets
# - Tests both serialization and parsing (deserialization)
# - Configurable variants and benchmarks
#
# Usage:
#   ./run_ablation_study.sh [options]
#
# Options:
#   --serialization       Run serialization benchmarks
#   --parsing             Run parsing benchmarks
#   --twitter             Use Twitter dataset
#   --citm                Use CITM catalog dataset
#   --rebuild             Force rebuild even if binaries exist
#   --help                Show this help message
#
# Examples:
#   ./run_ablation_study.sh --serialization --twitter
#   ./run_ablation_study.sh --parsing --citm
#   ./run_ablation_study.sh --serialization --parsing --twitter --citm
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"
ABLATION_DIR="$ROOT_DIR/ablation"
RESULTS_DIR="$ABLATION_DIR/results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Default options
RUN_SERIALIZATION=false
RUN_PARSING=false
RUN_TWITTER=false
RUN_CITM=false
FORCE_REBUILD=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --serialization)
            RUN_SERIALIZATION=true
            shift
            ;;
        --parsing|--deserialization)
            RUN_PARSING=true
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
        --rebuild)
            FORCE_REBUILD=true
            shift
            ;;
        --help)
            grep "^#" "$0" | head -32 | tail -30
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Set defaults if nothing specified
if [ "$RUN_SERIALIZATION" = false ] && [ "$RUN_PARSING" = false ]; then
    RUN_SERIALIZATION=true
    RUN_PARSING=true
fi

if [ "$RUN_TWITTER" = false ] && [ "$RUN_CITM" = false ]; then
    RUN_TWITTER=true
    RUN_CITM=true
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Comprehensive Ablation Study${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo -e "${CYAN}Configuration:${NC}"
echo "  Serialization:    $([ "$RUN_SERIALIZATION" = true ] && echo "YES" || echo "NO")"
echo "  Parsing:          $([ "$RUN_PARSING" = true ] && echo "YES" || echo "NO")"
echo "  Twitter:          $([ "$RUN_TWITTER" = true ] && echo "YES" || echo "NO")"
echo "  CITM:             $([ "$RUN_CITM" = true ] && echo "YES" || echo "NO")"
echo "  Force rebuild:    $([ "$FORCE_REBUILD" = true ] && echo "YES" || echo "NO")"
echo ""

# Create results directory
mkdir -p "$RESULTS_DIR"

# Define ablation variants with descriptions
declare -A variants=(
    ["baseline_ref"]=""
    ["no_consteval"]="-DSIMDJSON_ABLATION_NO_CONSTEVAL"
    ["no_simd_escaping"]="-DSIMDJSON_ABLATION_NO_SIMD_ESCAPING"
    ["no_fast_digits"]="-DSIMDJSON_ABLATION_NO_FAST_DIGITS"
    ["no_branch_hints"]="-DSIMDJSON_ABLATION_NO_BRANCH_HINTS"
    ["linear_growth"]="-DSIMDJSON_ABLATION_LINEAR_GROWTH"
)

declare -A variant_descriptions=(
    ["baseline_ref"]="Baseline with all optimizations"
    ["no_consteval"]="Without consteval optimizations"
    ["no_simd_escaping"]="Without SIMD string escaping"
    ["no_fast_digits"]="Without fast digit conversion"
    ["no_branch_hints"]="Without branch prediction hints"
    ["linear_growth"]="Using linear buffer growth"
)

# Function to check if binary exists
binary_exists() {
    local build_dir=$1
    local binary_path=$2
    [ -f "$build_dir/$binary_path" ]
}


# Function to build variant (for runtime tests)
build_variant() {
    local variant_name=$1
    local flags=$2
    local build_dir="$ROOT_DIR/build_ablation_$variant_name"

    # Check which binaries we need
    local need_twitter_ser=false
    local need_twitter_par=false
    local need_citm_ser=false
    local need_citm_par=false

    if [ "$RUN_SERIALIZATION" = true ] && [ "$RUN_TWITTER" = true ]; then
        if [ "$FORCE_REBUILD" = true ] || ! binary_exists "$build_dir" "benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter"; then
            need_twitter_ser=true
        fi
    fi

    if [ "$RUN_PARSING" = true ] && [ "$RUN_TWITTER" = true ]; then
        if [ "$FORCE_REBUILD" = true ] || ! binary_exists "$build_dir" "benchmark/static_reflect/twitter_benchmark/benchmark_parsing_twitter"; then
            need_twitter_par=true
        fi
    fi

    if [ "$RUN_SERIALIZATION" = true ] && [ "$RUN_CITM" = true ]; then
        if [ "$FORCE_REBUILD" = true ] || ! binary_exists "$build_dir" "benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog"; then
            need_citm_ser=true
        fi
    fi

    if [ "$RUN_PARSING" = true ] && [ "$RUN_CITM" = true ]; then
        if [ "$FORCE_REBUILD" = true ] || ! binary_exists "$build_dir" "benchmark/static_reflect/citm_catalog_benchmark/benchmark_parsing_citm"; then
            need_citm_par=true
        fi
    fi

    # Skip build if nothing needed
    if [ "$need_twitter_ser" = false ] && [ "$need_twitter_par" = false ] && \
       [ "$need_citm_ser" = false ] && [ "$need_citm_par" = false ]; then
        echo -e "  ${GREEN}All required binaries exist for $variant_name, skipping build${NC}"
        return 0
    fi

    echo -e "  ${YELLOW}Building variant: $variant_name${NC}"

    # Create build directory
    mkdir -p "$build_dir"
    cd "$build_dir"

    # Configure with CMake if needed
    if [ ! -f "CMakeCache.txt" ] || [ "$FORCE_REBUILD" = true ]; then
        # Clean CMake cache if force rebuild
        if [ "$FORCE_REBUILD" = true ] && [ -f "CMakeCache.txt" ]; then
            rm -rf CMakeCache.txt CMakeFiles/
        fi
        echo "    Configuring CMake..."
        if ! env CXX=/usr/local/bin/clang++ CC=/usr/local/bin/clang CXXFLAGS="-std=c++26 -freflection $flags -O3 -march=native" cmake "$ROOT_DIR" \
             -DCMAKE_BUILD_TYPE=Release \
             -DSIMDJSON_DEVELOPER_MODE=ON \
             -DSIMDJSON_STATIC_REFLECTION=ON \
             -DSIMDJSON_RUST_VERSION=ON > /dev/null 2>&1; then
            echo -e "    ${RED}ERROR: CMake configuration failed for $variant_name${NC}"
            return 1
        fi
    fi

    # Build only what we need
    local targets=""
    [ "$need_twitter_ser" = true ] && targets="$targets benchmark_serialization_twitter"
    [ "$need_twitter_par" = true ] && targets="$targets benchmark_parsing_twitter"
    [ "$need_citm_ser" = true ] && targets="$targets benchmark_serialization_citm_catalog"
    [ "$need_citm_par" = true ] && targets="$targets benchmark_parsing_citm"

    if [ -n "$targets" ]; then
        echo "    Building: $targets"
        if ! make $targets -j8 > /dev/null 2>&1; then
            echo -e "    ${RED}ERROR: Build failed for $variant_name${NC}"
            return 1
        fi
    fi

    echo -e "    ${GREEN}Build complete${NC}"
}

# Function to run benchmark
run_benchmark() {
    local variant_name=$1
    local build_dir="$ROOT_DIR/build_ablation_$variant_name"
    local benchmark_type=$2  # serialization or parsing
    local dataset=$3          # twitter or citm

    local binary_path=""
    local filter="simdjson"

    if [ "$benchmark_type" = "serialization" ]; then
        if [ "$dataset" = "twitter" ]; then
            binary_path="benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter"
            filter="simdjson_static_reflection"
        else
            binary_path="benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog"
            filter="simdjson_static_reflection"
        fi
    else
        if [ "$dataset" = "twitter" ]; then
            binary_path="benchmark/static_reflect/twitter_benchmark/benchmark_parsing_twitter"
            filter="simdjson_static_reflection"
        else
            binary_path="benchmark/static_reflect/citm_catalog_benchmark/benchmark_parsing_citm"
            filter="simdjson_static_reflection"
        fi
    fi

    if ! binary_exists "$build_dir" "$binary_path"; then
        echo "BINARY_NOT_FOUND"
        return
    fi

    # Run benchmark (3 times and take median)
    local results=()
    for i in {1..3}; do
        local output=$("$build_dir/$binary_path" -f "$filter" 2>&1)
        local result=$(echo "$output" | grep "bench_simdjson_static_reflection" | head -1 | grep -o '[0-9]*\.[0-9]* MB/s' | grep -o '[0-9]*\.[0-9]*' || echo "0")
        results+=($result)
    done

    # Sort and get median
    IFS=$'\n' sorted=($(sort -n <<<"${results[*]}")); unset IFS
    echo "${sorted[1]}"
}

# Initialize results files with timestamp
timestamp=$(date +"%Y%m%d_%H%M%S")
runtime_results_file="$RESULTS_DIR/runtime_ablation_${timestamp}.csv"

# Store baseline results for comparison
declare -A baseline_results

# Run runtime performance ablation if requested
if [ "$RUN_SERIALIZATION" = true ] || [ "$RUN_PARSING" = true ]; then
    echo -e "${CYAN}========================================${NC}"
    echo -e "${CYAN}     Runtime Performance Ablation${NC}"
    echo -e "${CYAN}========================================${NC}"
    echo ""

    echo "variant,description,benchmark_type,dataset,throughput_mbps,relative_to_baseline" > "$runtime_results_file"

    for variant in baseline_ref no_consteval no_simd_escaping no_fast_digits no_branch_hints linear_growth; do
        echo -e "${YELLOW}Testing: ${variant_descriptions[$variant]}${NC}"

        # Build variant if needed
        build_variant "$variant" "${variants[$variant]}"

        # Run benchmarks
        if [ "$RUN_SERIALIZATION" = true ]; then
            if [ "$RUN_TWITTER" = true ]; then
                echo "  Running Twitter serialization..."
                result=$(run_benchmark "$variant" "serialization" "twitter")
                key="serialization_twitter"
                if [ "$variant" = "baseline_ref" ]; then
                    baseline_results[$key]=$result
                fi
                relative=$(echo "$result ${baseline_results[$key]:-$result}" | awk '{printf "%.2f", $1 * 100 / $2}')
                echo "$variant,${variant_descriptions[$variant]},serialization,twitter,$result,$relative%" >> "$runtime_results_file"
                echo -e "    Result: ${GREEN}$result MB/s${NC} (${relative}% of baseline)"
            fi

            if [ "$RUN_CITM" = true ]; then
                echo "  Running CITM serialization..."
                result=$(run_benchmark "$variant" "serialization" "citm")
                key="serialization_citm"
                if [ "$variant" = "baseline_ref" ]; then
                    baseline_results[$key]=$result
                fi
                relative=$(echo "$result ${baseline_results[$key]:-$result}" | awk '{printf "%.2f", $1 * 100 / $2}')
                echo "$variant,${variant_descriptions[$variant]},serialization,citm,$result,$relative%" >> "$runtime_results_file"
                echo -e "    Result: ${GREEN}$result MB/s${NC} (${relative}% of baseline)"
            fi
        fi

        if [ "$RUN_PARSING" = true ]; then
            if [ "$RUN_TWITTER" = true ]; then
                echo "  Running Twitter parsing..."
                result=$(run_benchmark "$variant" "parsing" "twitter")
                key="parsing_twitter"
                if [ "$variant" = "baseline_ref" ]; then
                    baseline_results[$key]=$result
                fi
                relative=$(echo "$result ${baseline_results[$key]:-$result}" | awk '{printf "%.2f", $1 * 100 / $2}')
                echo "$variant,${variant_descriptions[$variant]},parsing,twitter,$result,$relative%" >> "$runtime_results_file"
                echo -e "    Result: ${GREEN}$result MB/s${NC} (${relative}% of baseline)"
            fi

            if [ "$RUN_CITM" = true ]; then
                echo "  Running CITM parsing..."
                result=$(run_benchmark "$variant" "parsing" "citm")
                key="parsing_citm"
                if [ "$variant" = "baseline_ref" ]; then
                    baseline_results[$key]=$result
                fi
                relative=$(echo "$result ${baseline_results[$key]:-$result}" | awk '{printf "%.2f", $1 * 100 / $2}')
                echo "$variant,${variant_descriptions[$variant]},parsing,citm,$result,$relative%" >> "$runtime_results_file"
                echo -e "    Result: ${GREEN}$result MB/s${NC} (${relative}% of baseline)"
            fi
        fi
        echo ""
    done
fi

# Display final summary
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}      Ablation Study Complete${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Display runtime performance summary
if [ "$RUN_SERIALIZATION" = true ] || [ "$RUN_PARSING" = true ]; then
    echo -e "${CYAN}Runtime Performance Impact Summary:${NC}"
    echo -e "${CYAN}Results saved to: $runtime_results_file${NC}"
    echo ""

    if [ "$RUN_SERIALIZATION" = true ]; then
        echo -e "${YELLOW}SERIALIZATION:${NC}"
        if [ "$RUN_TWITTER" = true ]; then
            echo "  Twitter:"
            grep "serialization,twitter" "$runtime_results_file" | tail -n +2 | while IFS=, read -r variant desc type dataset throughput relative; do
                printf "    %-20s %8s MB/s  %8s\n" "$variant:" "$throughput" "$relative"
            done
            echo ""
        fi
        if [ "$RUN_CITM" = true ]; then
            echo "  CITM:"
            grep "serialization,citm" "$runtime_results_file" | tail -n +2 | while IFS=, read -r variant desc type dataset throughput relative; do
                printf "    %-20s %8s MB/s  %8s\n" "$variant:" "$throughput" "$relative"
            done
            echo ""
        fi
    fi

    if [ "$RUN_PARSING" = true ]; then
        echo -e "${YELLOW}PARSING (DESERIALIZATION):${NC}"
        if [ "$RUN_TWITTER" = true ]; then
            echo "  Twitter:"
            grep "parsing,twitter" "$runtime_results_file" | tail -n +2 | while IFS=, read -r variant desc type dataset throughput relative; do
                printf "    %-20s %8s MB/s  %8s\n" "$variant:" "$throughput" "$relative"
            done
            echo ""
        fi
        if [ "$RUN_CITM" = true ]; then
            echo "  CITM:"
            grep "parsing,citm" "$runtime_results_file" | tail -n +2 | while IFS=, read -r variant desc type dataset throughput relative; do
                printf "    %-20s %8s MB/s  %8s\n" "$variant:" "$throughput" "$relative"
            done
            echo ""
        fi
    fi
fi

echo -e "${GREEN}Analysis complete!${NC}"
echo ""
echo "Note: All build directories (build_ablation_*) can be safely deleted to save space."