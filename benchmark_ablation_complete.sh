#!/bin/bash

# Complete ablation study with compilation time measurement
# Francisco Thiesen - simdjson reflection performance analysis

set -e

RESULTS_FILE="ablation_complete_results.txt"
BUILD_DIR="build_ablation"

echo "=== simdjson Reflection Ablation Study ===" | tee $RESULTS_FILE
echo "Date: $(date)" | tee -a $RESULTS_FILE
echo "Compiler: $(clang++ --version | head -1)" | tee -a $RESULTS_FILE
echo "Platform: $(uname -a)" | tee -a $RESULTS_FILE
echo "" | tee -a $RESULTS_FILE

# Function to measure compilation time and run benchmark
measure_variant() {
    local variant_name="$1"
    local cmake_flags="$2"
    
    echo "=== Testing $variant_name ===" | tee -a $RESULTS_FILE
    
    # Clean build
    rm -rf $BUILD_DIR
    mkdir $BUILD_DIR
    cd $BUILD_DIR
    
    # Configure
    echo "Configuring..." | tee -a ../$RESULTS_FILE
    cmake -DCMAKE_CXX_COMPILER=clang++ \
          -DSIMDJSON_DEVELOPER_MODE=ON \
          -DSIMDJSON_STATIC_REFLECTION=ON \
          -DBUILD_SHARED_LIBS=OFF \
          $cmake_flags \
          .. > cmake_output.log 2>&1
    
    # Measure compilation time
    echo "Measuring compilation time..." | tee -a ../$RESULTS_FILE
    start_time=$(date +%s.%N)
    cmake --build . --target benchmark_serialization_twitter > build_output.log 2>&1
    end_time=$(date +%s.%N)
    compile_time=$(echo "$end_time - $start_time" | bc)
    
    echo "Compilation time: ${compile_time}s" | tee -a ../$RESULTS_FILE
    
    # Run performance benchmark (3 times for stability)
    echo "Running performance benchmarks..." | tee -a ../$RESULTS_FILE
    for i in {1..3}; do
        echo "Run $i:" | tee -a ../$RESULTS_FILE
        ./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection | tee -a ../$RESULTS_FILE
    done
    
    echo "" | tee -a ../$RESULTS_FILE
    cd ..
}

# Test all variants
measure_variant "Baseline (Full Optimizations)" ""

measure_variant "No SIMD Escaping" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_SIMD_ESCAPING\""

measure_variant "No Consteval" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_CONSTEVAL\""

measure_variant "No Fast Digits" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_FAST_DIGITS\""

measure_variant "No Lookup Tables" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_LOOKUP_TABLES\""

measure_variant "No SIMD + No Consteval" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_SIMD_ESCAPING -DSIMDJSON_ABLATION_NO_CONSTEVAL\""

# Cleanup
rm -rf $BUILD_DIR

echo "=== Ablation Study Complete ===" | tee -a $RESULTS_FILE
echo "Results saved to: $RESULTS_FILE" | tee -a $RESULTS_FILE