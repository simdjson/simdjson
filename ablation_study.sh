#!/bin/bash

# Unified Ablation Study Script for simdjson C++26 Reflection Serialization
# This script runs performance ablation studies for both Twitter and CITM benchmarks
# with optional compilation time measurement

set -e

# Default configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
RUNS_TWITTER=10
RUNS_CITM=20  # Higher due to higher variance
BENCHMARK="both"
MEASURE_COMPILATION=false
OUTPUT_DIR="$SCRIPT_DIR/ablation_results"
VERBOSE=false

# Ablation variants
declare -A ABLATION_VARIANTS=(
    ["baseline"]=""
    ["no_consteval"]="-DSIMDJSON_ABLATION_NO_CONSTEVAL"
    ["no_simd_escaping"]="-DSIMDJSON_ABLATION_NO_SIMD_ESCAPING"
    ["no_fast_digits"]="-DSIMDJSON_ABLATION_NO_FAST_DIGITS"
    ["no_branch_hints"]="-DSIMDJSON_ABLATION_NO_BRANCH_HINTS"
    ["linear_growth"]="-DSIMDJSON_ABLATION_LINEAR_GROWTH"
)

# Function to display usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Unified ablation study script for simdjson C++26 reflection benchmarks.

Options:
    -b, --benchmark NAME       Benchmark to run: twitter, citm, or both (default: both)
    -r, --runs NUM            Number of runs for Twitter benchmark (default: $RUNS_TWITTER)
    -c, --citm-runs NUM       Number of runs for CITM benchmark (default: $RUNS_CITM)
    --compilation-time        Also measure compilation time for each variant
    -o, --output DIR          Output directory for results (default: ablation_results)
    -v, --verbose             Enable verbose output
    -h, --help                Show this help message

Examples:
    $0                        # Run both benchmarks with defaults
    $0 -b twitter -r 20       # Run only Twitter with 20 iterations
    $0 --compilation-time     # Include compilation time measurements
    $0 -c 30                  # Run CITM with 30 iterations (for high variance)

Notes:
    - CITM defaults to 20 runs due to higher variance (CV ~19%)
    - Twitter defaults to 10 runs due to lower variance (CV ~1%)
    - Compilation time measurement adds significant time to the study
EOF
}

# Function to parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -b|--benchmark)
                BENCHMARK="$2"
                shift 2
                ;;
            -r|--runs)
                RUNS_TWITTER="$2"
                shift 2
                ;;
            -c|--citm-runs)
                RUNS_CITM="$2"
                shift 2
                ;;
            --compilation-time)
                MEASURE_COMPILATION=true
                shift
                ;;
            -o|--output)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Validate benchmark parameter
    case "$BENCHMARK" in
        twitter|citm|both)
            ;;
        *)
            echo "Error: --benchmark must be 'twitter', 'citm', or 'both'"
            exit 1
            ;;
    esac
    
    # Validate runs parameters
    if ! [[ "$RUNS_TWITTER" =~ ^[0-9]+$ ]] || [ "$RUNS_TWITTER" -lt 1 ]; then
        echo "Error: --runs must be a positive integer"
        exit 1
    fi
    
    if ! [[ "$RUNS_CITM" =~ ^[0-9]+$ ]] || [ "$RUNS_CITM" -lt 1 ]; then
        echo "Error: --citm-runs must be a positive integer"
        exit 1
    fi
}

# Function to log messages
log() {
    if [ "$VERBOSE" = true ]; then
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
    fi
}

# Function to ensure build directory exists and is configured
setup_build() {
    if [[ ! -d "$BUILD_DIR" ]]; then
        mkdir -p "$BUILD_DIR"
    fi
    
    cd "$BUILD_DIR"
    
    # Ensure baseline configuration
    log "Setting up build directory with baseline configuration..."
    cmake .. -DCMAKE_CXX_COMPILER=clang++ \
             -DSIMDJSON_DEVELOPER_MODE=ON \
             -DSIMDJSON_STATIC_REFLECTION=ON \
             -DBUILD_SHARED_LIBS=OFF \
             -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
}

# Function to build with specific ablation flags
build_variant() {
    local variant="$1"
    local flags="$2"
    local measure_time="$3"
    
    log "Building variant: $variant"
    
    cd "$BUILD_DIR"
    
    # Configure with ablation flags
    cmake .. -DCMAKE_CXX_FLAGS="$flags" > /dev/null 2>&1
    
    if [ "$measure_time" = true ]; then
        # Measure compilation time
        local start_time=$(date +%s.%N)
        make benchmark_serialization_twitter benchmark_serialization_citm_catalog -j4 > /dev/null 2>&1 || true
        local end_time=$(date +%s.%N)
        local compilation_time=$(echo "$end_time - $start_time" | bc -l)
        echo "$compilation_time"
    else
        make benchmark_serialization_twitter benchmark_serialization_citm_catalog -j4 > /dev/null 2>&1 || true
        echo "0"
    fi
}

# Function to run Twitter benchmark
run_twitter_benchmark() {
    local variant="$1"
    local runs="$2"
    local results=()
    
    # Check if benchmark exists
    if [[ ! -x "$BUILD_DIR/benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter" ]]; then
        log "Twitter benchmark not found, skipping..."
        return 1
    fi
    
    log "Running Twitter benchmark for $variant ($runs runs)..."
    
    for ((i=1; i<=$runs; i++)); do
        local output=$($BUILD_DIR/benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection 2>&1 | grep "bench_simdjson_static_reflection" || true)
        
        if [[ -n "$output" ]]; then
            local throughput=$(echo "$output" | grep -o '[0-9]*\.[0-9]* MB/s' | grep -o '[0-9]*\.[0-9]*' | head -1)
            if [[ -n "$throughput" ]]; then
                results+=("$throughput")
                log "  Run $i: $throughput MB/s"
            fi
        fi
    done
    
    # Return results as comma-separated string
    printf '%s\n' "${results[@]}" | paste -sd,
}

# Function to run CITM benchmark
run_citm_benchmark() {
    local variant="$1"
    local runs="$2"
    local results=()
    
    # Create and compile CITM test program
    log "Compiling CITM test program..."
    
    cat > "$BUILD_DIR/citm_test.cpp" << 'EOF'
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <cmath>
#include <map>
#include <string>
#include <simdjson.h>

struct Event {
    int64_t id;
    std::string name;
    std::string description;
    int64_t topic;
    std::vector<int64_t> relatedIds;
    bool operator==(const Event&) const = default;
};

struct Venue {
    int64_t id;
    std::string name;
    std::string address;
    int64_t capacity;
    std::vector<std::string> features;
    bool operator==(const Venue&) const = default;
};

struct Catalog {
    std::map<std::string, Event> events;
    std::map<std::string, Venue> venues;
    std::vector<int64_t> activeIds;
    std::string catalogName;
    std::string lastUpdate;
    bool operator==(const Catalog&) const = default;
};

Catalog create_test_catalog() {
    Catalog catalog;
    catalog.catalogName = "CITM Test Catalog";
    catalog.lastUpdate = "2025-07-31T20:00:00Z";
    
    for (int i = 0; i < 200; i++) {
        Event event;
        event.id = 10000 + i;
        event.name = "Event " + std::to_string(i);
        event.description = "This is a description for event number " + std::to_string(i) + " with some additional text";
        event.topic = 100 + (i % 20);
        event.relatedIds = {i*2, i*2+1, i*2+2, i*2+3};
        catalog.events["evt_" + std::to_string(i)] = event;
        catalog.activeIds.push_back(event.id);
    }
    
    for (int i = 0; i < 50; i++) {
        Venue venue;
        venue.id = 20000 + i;
        venue.name = "Venue " + std::to_string(i);
        venue.address = "123 Main St, City " + std::to_string(i);
        venue.capacity = 1000 + i * 50;
        venue.features = {"Feature A", "Feature B", "Feature C"};
        catalog.venues["ven_" + std::to_string(i)] = venue;
    }
    
    return catalog;
}

int main() {
    Catalog catalog = create_test_catalog();
    
    // Warm up
    for (int i = 0; i < 50; i++) {
        simdjson::arm64::builder::string_builder sb;
        simdjson::arm64::builder::append(sb, catalog);
        std::string_view json;
        sb.view().get(json);
    }
    
    // Single run measurement
    simdjson::arm64::builder::string_builder sb;
    
    auto start = std::chrono::high_resolution_clock::now();
    simdjson::arm64::builder::append(sb, catalog);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::string_view json;
    sb.view().get(json);
    
    double elapsed = std::chrono::duration<double>(end - start).count();
    double throughput = (json.size() / 1024.0 / 1024.0) / elapsed;
    
    std::cout << throughput << std::endl;
    
    return 0;
}
EOF

    local flags="${ABLATION_VARIANTS[$variant]}"
    clang++ -std=c++26 -freflection -fexpansion-statements -stdlib=libc++ -O3 \
        -DSIMDJSON_STATIC_REFLECTION=1 $flags \
        -I../include -I.. "$BUILD_DIR/citm_test.cpp" -o "$BUILD_DIR/citm_test" -L. -lsimdjson 2>/dev/null
    
    log "Running CITM benchmark for $variant ($runs runs)..."
    
    for ((i=1; i<=$runs; i++)); do
        local throughput=$($BUILD_DIR/citm_test 2>/dev/null)
        if [[ -n "$throughput" ]]; then
            results+=("$throughput")
            log "  Run $i: $throughput MB/s"
        fi
    done
    
    # Clean up
    rm -f "$BUILD_DIR/citm_test" "$BUILD_DIR/citm_test.cpp"
    
    # Return results as comma-separated string
    printf '%s\n' "${results[@]}" | paste -sd,
}

# Function to calculate statistics
calculate_stats() {
    local data="$1"
    
    python3 -c "
import sys
import statistics

values = [float(x) for x in '$data'.split(',') if x]
if not values:
    print('0,0,0,0')
else:
    mean = statistics.mean(values)
    stdev = statistics.stdev(values) if len(values) > 1 else 0
    cv = (stdev / mean * 100) if mean > 0 else 0
    count = len(values)
    print(f'{mean:.2f},{stdev:.2f},{cv:.2f},{count}')
"
}

# Function to calculate performance impact
calculate_impact() {
    local baseline="$1"
    local variant="$2"
    
    if (( $(echo "$baseline > 0" | bc -l) )); then
        echo "scale=1; (($variant - $baseline) / $baseline) * 100" | bc -l
    else
        echo "0"
    fi
}

# Main function
main() {
    parse_args "$@"
    
    # Create output directory
    mkdir -p "$OUTPUT_DIR"
    
    echo "=== simdjson C++26 Reflection Ablation Study ==="
    echo "Date: $(date)"
    echo "Benchmarks: $BENCHMARK"
    echo "Twitter runs: $RUNS_TWITTER"
    echo "CITM runs: $RUNS_CITM"
    echo "Measure compilation: $MEASURE_COMPILATION"
    echo ""
    
    # Setup build directory
    setup_build
    
    # Initialize results files
    local twitter_results="$OUTPUT_DIR/twitter_ablation_results.csv"
    local citm_results="$OUTPUT_DIR/citm_ablation_results.csv"
    local summary_file="$OUTPUT_DIR/ablation_summary.txt"
    
    # Headers
    echo "Variant,Mean_MB/s,StdDev,CV%,Runs,Impact%,CompileTime_s" > "$twitter_results"
    echo "Variant,Mean_MB/s,StdDev,CV%,Runs,Impact%,CompileTime_s" > "$citm_results"
    
    # Baseline values for impact calculation
    local twitter_baseline=0
    local citm_baseline=0
    
    # Process each variant
    for variant in baseline no_consteval no_simd_escaping no_fast_digits no_branch_hints linear_growth; do
        echo "Processing variant: $variant"
        
        # Build variant
        local compile_time=$(build_variant "$variant" "${ABLATION_VARIANTS[$variant]}" "$MEASURE_COMPILATION")
        
        # Run Twitter benchmark if requested
        if [[ "$BENCHMARK" == "twitter" ]] || [[ "$BENCHMARK" == "both" ]]; then
            local twitter_data=$(run_twitter_benchmark "$variant" "$RUNS_TWITTER")
            if [[ -n "$twitter_data" ]]; then
                local twitter_stats=$(calculate_stats "$twitter_data")
                IFS=',' read -r mean stdev cv count <<< "$twitter_stats"
                
                if [[ "$variant" == "baseline" ]]; then
                    twitter_baseline=$mean
                    impact="0"
                else
                    impact=$(calculate_impact "$twitter_baseline" "$mean")
                fi
                
                echo "$variant,$mean,$stdev,$cv,$count,$impact,$compile_time" >> "$twitter_results"
                echo "  Twitter: $mean MB/s (±$stdev, CV: $cv%, Impact: ${impact}%)"
            fi
        fi
        
        # Run CITM benchmark if requested
        if [[ "$BENCHMARK" == "citm" ]] || [[ "$BENCHMARK" == "both" ]]; then
            local citm_data=$(run_citm_benchmark "$variant" "$RUNS_CITM")
            if [[ -n "$citm_data" ]]; then
                local citm_stats=$(calculate_stats "$citm_data")
                IFS=',' read -r mean stdev cv count <<< "$citm_stats"
                
                if [[ "$variant" == "baseline" ]]; then
                    citm_baseline=$mean
                    impact="0"
                else
                    impact=$(calculate_impact "$citm_baseline" "$mean")
                fi
                
                echo "$variant,$mean,$stdev,$cv,$count,$impact,$compile_time" >> "$citm_results"
                echo "  CITM: $mean MB/s (±$stdev, CV: $cv%, Impact: ${impact}%)"
            fi
        fi
        
        echo ""
    done
    
    # Generate summary report
    {
        echo "=== Ablation Study Summary ==="
        echo "Generated: $(date)"
        echo ""
        
        if [[ -f "$twitter_results" ]] && [[ $(wc -l < "$twitter_results") -gt 1 ]]; then
            echo "Twitter Benchmark Results:"
            echo "-------------------------"
            # Use awk instead of column for better compatibility
            awk -F',' 'NR==1{printf "%-20s %12s %10s %8s %8s %10s %12s\n", $1, $2, $3, $4, $5, $6, $7} NR>1{printf "%-20s %12.2f %10.2f %8.2f %8d %10.1f %12.2f\n", $1, $2, $3, $4, $5, $6, $7}' "$twitter_results"
            echo ""
        fi
        
        if [[ -f "$citm_results" ]] && [[ $(wc -l < "$citm_results") -gt 1 ]]; then
            echo "CITM Benchmark Results:"
            echo "-----------------------"
            # Use awk instead of column for better compatibility
            awk -F',' 'NR==1{printf "%-20s %12s %10s %8s %8s %10s %12s\n", $1, $2, $3, $4, $5, $6, $7} NR>1{printf "%-20s %12.2f %10.2f %8.2f %8d %10.1f %12.2f\n", $1, $2, $3, $4, $5, $6, $7}' "$citm_results"
            echo ""
        fi
        
        echo "Results saved to: $OUTPUT_DIR"
    } > "$summary_file"
    
    cat "$summary_file"
    
    echo ""
    echo "Detailed results saved to:"
    echo "  - $twitter_results"
    echo "  - $citm_results"
    echo "  - $summary_file"
}

# Check dependencies
if ! command -v bc &> /dev/null; then
    echo "Error: 'bc' calculator is required but not installed"
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo "Error: 'python3' is required but not installed"
    exit 1
fi

# Run main function
main "$@"