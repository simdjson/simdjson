#!/bin/bash

# Ablation Study for simdjson C++26 Reflection
# Builds with each ablation flag and measures performance impact

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/build"
RUNS_TWITTER=10
RUNS_CITM=10
OUTPUT_DIR="$SCRIPT_DIR/results"

# Ablation variants to test
declare -A ABLATION_VARIANTS=(
    ["baseline"]=""
    ["no_consteval"]="-DSIMDJSON_ABLATION_NO_CONSTEVAL"
    ["no_simd_escaping"]="-DSIMDJSON_ABLATION_NO_SIMD_ESCAPING"
    ["no_fast_digits"]="-DSIMDJSON_ABLATION_NO_FAST_DIGITS"
    ["no_branch_hints"]="-DSIMDJSON_ABLATION_NO_BRANCH_HINTS"
    ["linear_growth"]="-DSIMDJSON_ABLATION_LINEAR_GROWTH"
)

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -r|--runs-twitter)
            RUNS_TWITTER="$2"
            shift 2
            ;;
        -c|--runs-citm)
            RUNS_CITM="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "  -r, --runs-twitter NUM  Number of runs for Twitter (default: 10)"
            echo "  -c, --runs-citm NUM     Number of runs for CITM (default: 10)"
            echo "  -h, --help              Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Build variant with specific flags
build_variant() {
    local variant="$1"
    local flags="$2"
    
    echo "  Building with flags: $flags"
    
    # Make sure build directory exists
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Always reconfigure to ensure correct flags are applied
    echo "  Configuring CMake..."
    rm -f CMakeCache.txt
    if ! env CXX=/usr/local/bin/clang++ CC=/usr/local/bin/clang cmake .. \
             -DCMAKE_CXX_FLAGS="$flags" \
             -DSIMDJSON_DEVELOPER_MODE=ON \
             -DSIMDJSON_STATIC_REFLECTION=ON \
             -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1; then
        echo "  ERROR: CMake configuration failed!"
        return 1
    fi
    
    # Build the benchmarks - clean and rebuild to ensure correct flags
    echo "  Building benchmarks..."
    make clean > /dev/null 2>&1
    if ! make benchmark_serialization_twitter benchmark_serialization_citm_catalog -j4 > /dev/null 2>&1; then
        echo "  ERROR: Build failed!"
        return 1
    fi
    
    echo "  Build completed successfully"
    return 0
}

# Run Twitter benchmark
run_twitter_benchmark() {
    local runs="$1"
    local results=()
    
    cd "$BUILD_DIR"
    local benchmark="./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter"
    
    if [[ -x "$benchmark" ]]; then
        for ((i=1; i<=$runs; i++)); do
            local output=$($benchmark -f simdjson_static_reflection 2>&1 | grep "bench_simdjson_static_reflection")
            if [[ -n "$output" ]]; then
                local throughput=$(echo "$output" | grep -o '[0-9]*\.[0-9]* MB/s' | grep -o '[0-9]*\.[0-9]*' | head -1)
                if [[ -n "$throughput" ]]; then
                    results+=("$throughput")
                fi
            fi
        done
    fi
    
    if [[ ${#results[@]} -gt 0 ]]; then
        printf '%s\n' "${results[@]}" | paste -sd,
    fi
}

# Run CITM benchmark
run_citm_benchmark() {
    local runs="$1"
    local results=()
    
    cd "$BUILD_DIR"
    local benchmark="./benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog"
    
    if [[ -x "$benchmark" ]]; then
        for ((i=1; i<=$runs; i++)); do
            local output=$($benchmark -f simdjson_static_reflection 2>&1 | grep "bench_simdjson_static_reflection")
            if [[ -n "$output" ]]; then
                local throughput=$(echo "$output" | grep -o '[0-9]*\.[0-9]* MB/s' | grep -o '[0-9]*\.[0-9]*' | head -1)
                if [[ -n "$throughput" ]]; then
                    results+=("$throughput")
                fi
            fi
        done
    fi
    
    if [[ ${#results[@]} -gt 0 ]]; then
        printf '%s\n' "${results[@]}" | paste -sd,
    fi
}

# Calculate statistics
calculate_stats() {
    local data="$1"
    
    if [[ -z "$data" ]]; then
        echo "0,0,0,0"
        return
    fi
    
    python3 -c "
import statistics
values = [float(x) for x in '$data'.split(',') if x]
if not values:
    print('0,0,0,0')
else:
    mean = statistics.mean(values)
    stdev = statistics.stdev(values) if len(values) > 1 else 0
    cv = (stdev / mean * 100) if mean > 0 else 0
    print(f'{mean:.2f},{stdev:.2f},{cv:.2f},{len(values)}')"
}

# Main execution
main() {
    echo "=== simdjson C++26 Reflection Ablation Study ==="
    echo "Date: $(date)"
    echo "Configuration:"
    echo "  Twitter runs: $RUNS_TWITTER"
    echo "  CITM runs: $RUNS_CITM"
    echo ""
    
    # Setup output directory
    mkdir -p "$OUTPUT_DIR"
    
    # Results files
    local results_file="$OUTPUT_DIR/ablation_results.csv"
    local summary_file="$OUTPUT_DIR/ablation_summary.txt"
    
    # CSV header
    echo "Variant,Twitter_MB/s,Twitter_StdDev,Twitter_CV%,CITM_MB/s,CITM_StdDev,CITM_CV%,Twitter_Impact%,CITM_Impact%" > "$results_file"
    
    # Storage for baseline values
    local twitter_baseline=0
    local citm_baseline=0
    
    echo "Running ablation study..."
    echo "========================"
    echo ""
    
    # Test each variant
    for variant in baseline no_consteval no_simd_escaping no_fast_digits no_branch_hints linear_growth; do
        echo "Testing variant: $variant"
        
        # Build with variant flags
        if ! build_variant "$variant" "${ABLATION_VARIANTS[$variant]}"; then
            echo "  Skipping variant due to build failure"
            echo ""
            continue
        fi
        
        # Run Twitter benchmark
        echo "  Running Twitter benchmark..."
        local twitter_data=$(run_twitter_benchmark "$RUNS_TWITTER")
        local twitter_stats=$(calculate_stats "$twitter_data")
        IFS=',' read -r t_mean t_stdev t_cv t_count <<< "$twitter_stats"
        
        # Run CITM benchmark
        echo "  Running CITM benchmark..."
        local citm_data=$(run_citm_benchmark "$RUNS_CITM")
        local citm_stats=$(calculate_stats "$citm_data")
        IFS=',' read -r c_mean c_stdev c_cv c_count <<< "$citm_stats"
        
        # Calculate impact
        if [[ "$variant" == "baseline" ]]; then
            twitter_baseline=$t_mean
            citm_baseline=$c_mean
            t_impact="0"
            c_impact="0"
        else
            if (( $(echo "$twitter_baseline > 0" | bc -l) )); then
                t_impact=$(echo "scale=1; (($t_mean - $twitter_baseline) / $twitter_baseline) * 100" | bc -l)
            else
                t_impact="0"
            fi
            if (( $(echo "$citm_baseline > 0" | bc -l) )); then
                c_impact=$(echo "scale=1; (($c_mean - $citm_baseline) / $citm_baseline) * 100" | bc -l)
            else
                c_impact="0"
            fi
        fi
        
        # Save to CSV
        echo "$variant,$t_mean,$t_stdev,$t_cv,$c_mean,$c_stdev,$c_cv,$t_impact,$c_impact" >> "$results_file"
        
        # Display results
        printf "  Twitter: %7.1f MB/s (±%.1f, CV: %.1f%%)" $t_mean $t_stdev $t_cv
        if [[ "$variant" != "baseline" ]]; then
            printf " [%+.1f%%]" $t_impact
        fi
        echo ""
        
        printf "  CITM:    %7.1f MB/s (±%.1f, CV: %.1f%%)" $c_mean $c_stdev $c_cv
        if [[ "$variant" != "baseline" ]]; then
            printf " [%+.1f%%]" $c_impact
        fi
        echo ""
        echo ""
    done
    
    # Generate summary
    {
        echo "=== Ablation Study Summary ==="
        echo "Generated: $(date)"
        echo ""
        echo "Baseline Performance:"
        echo "  Twitter: $(printf '%.1f' $twitter_baseline) MB/s"
        echo "  CITM:    $(printf '%.1f' $citm_baseline) MB/s"
        echo ""
        echo "Optimization Impact (when DISABLED):"
        echo ""
        
        # Process each variant for summary
        tail -n +2 "$results_file" | while IFS=',' read -r variant t_mean t_stdev t_cv c_mean c_stdev c_cv t_impact c_impact; do
            if [[ "$variant" != "baseline" ]]; then
                opt_name="${variant/no_/}"
                opt_name="${opt_name//_/ }"
                
                # Convert negative impact to positive contribution
                t_contrib=$(echo "scale=1; -1 * $t_impact" | bc -l 2>/dev/null || echo "0")
                c_contrib=$(echo "scale=1; -1 * $c_impact" | bc -l 2>/dev/null || echo "0")
                
                printf "%-20s Twitter: %+6.1f%%   CITM: %+6.1f%%\n" "$opt_name:" "$t_contrib" "$c_contrib"
            fi
        done
        
        echo ""
        echo "Note: Positive values show the performance gain from having the optimization enabled."
        echo "Results saved to: $OUTPUT_DIR"
    } | tee "$summary_file"
    
    echo ""
    echo "Detailed results saved to:"
    echo "  $results_file"
    echo "  $summary_file"
}

# Check dependencies
if ! command -v bc &>/dev/null; then
    echo "Error: bc is required but not installed"
    exit 1
fi

if ! command -v python3 &>/dev/null; then
    echo "Error: python3 is required but not installed"
    exit 1
fi

if ! command -v cmake &>/dev/null; then
    echo "Error: cmake is required but not installed"
    exit 1
fi

if ! command -v /usr/local/bin/clang++ &>/dev/null; then
    echo "Error: clang++ with C++26 support required at /usr/local/bin/clang++"
    exit 1
fi

# Run main
main