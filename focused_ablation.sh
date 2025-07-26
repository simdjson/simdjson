#!/bin/bash

# Focused ablation study - key optimizations with compilation times
# Francisco Thiesen - simdjson reflection performance analysis

set -e

RESULTS_FILE="focused_ablation_results.md"

echo "# Extended simdjson Reflection Ablation Study" > $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "**Date:** $(date)" >> $RESULTS_FILE
echo "**Compiler:** $(clang++ --version | head -1)" >> $RESULTS_FILE
echo "**Platform:** $(uname -m)" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# Function to measure compilation time and run benchmark
measure_variant() {
    local variant_name="$1"
    local cmake_flags="$2"
    local description="$3"
    
    echo "## $variant_name" >> $RESULTS_FILE
    echo "" >> $RESULTS_FILE
    echo "$description" >> $RESULTS_FILE
    echo "" >> $RESULTS_FILE
    
    # Clean build
    rm -rf build
    mkdir build
    cd build
    
    # Configure (silent)
    cmake -DCMAKE_CXX_COMPILER=clang++ \
          -DSIMDJSON_DEVELOPER_MODE=ON \
          -DSIMDJSON_STATIC_REFLECTION=ON \
          -DBUILD_SHARED_LIBS=OFF \
          $cmake_flags \
          .. > /dev/null 2>&1
    
    # Measure compilation time
    echo "**Compilation Time:**" >> ../$RESULTS_FILE
    start_time=$(date +%s.%N)
    cmake --build . --target benchmark_serialization_twitter > /dev/null 2>&1
    end_time=$(date +%s.%N)
    compile_time=$(python3 -c "print(f'{$end_time - $start_time:.2f}')")
    
    echo "\`\`\`" >> ../$RESULTS_FILE
    echo "Compilation time: ${compile_time}s" >> ../$RESULTS_FILE
    echo "\`\`\`" >> ../$RESULTS_FILE
    echo "" >> ../$RESULTS_FILE
    
    # Run performance benchmark (3 times for stability)
    echo "**Performance Results:**" >> ../$RESULTS_FILE
    echo "\`\`\`" >> ../$RESULTS_FILE
    
    total_throughput=0
    for i in {1..3}; do
        result=$(./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection)
        echo "$result" >> ../$RESULTS_FILE
        # Extract throughput for averaging
        throughput=$(echo "$result" | grep "bench_simdjson_static_reflection" | awk '{print $3}' | sed 's/MB\/s//')
        total_throughput=$(python3 -c "print($total_throughput + $throughput)")
    done
    
    avg_throughput=$(python3 -c "print(f'{$total_throughput / 3:.2f}')")
    echo "" >> ../$RESULTS_FILE
    echo "Average throughput: ${avg_throughput} MB/s" >> ../$RESULTS_FILE
    echo "\`\`\`" >> ../$RESULTS_FILE
    echo "" >> ../$RESULTS_FILE
    
    cd ..
}

# Test key variants
measure_variant "Baseline (Full Optimizations)" "" "All optimizations enabled - our performance target."

measure_variant "No SIMD Escaping" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_SIMD_ESCAPING\"" "Disables vectorized string character checking, forces scalar implementation."

measure_variant "No Consteval" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_CONSTEVAL\"" "Disables compile-time string processing - most critical optimization."

measure_variant "No Fast Digits" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_FAST_DIGITS\"" "Uses std::to_string() instead of optimized digit counting."

measure_variant "Combined: No SIMD + No Consteval" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_SIMD_ESCAPING -DSIMDJSON_ABLATION_NO_CONSTEVAL\"" "Worst case: disables both key optimizations."

# Cleanup
rm -rf build

echo "---" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "**Study completed successfully.**" >> $RESULTS_FILE

echo "Results saved to: $RESULTS_FILE"