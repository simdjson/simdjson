#!/bin/bash

# Extended Ablation Study - Additional Performance Optimizations
# Tests micro-optimizations beyond the core algorithmic changes

set -e

RESULTS_FILE="extended_ablation_results.md"

echo "# Extended simdjson Reflection Ablation Study" > $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "**Focus**: Micro-optimizations and implementation details" >> $RESULTS_FILE
echo "**Date:** $(date)" >> $RESULTS_FILE
echo "**Compiler:** $(clang++ --version | head -1)" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# Function to run single benchmark
run_benchmark() {
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
    
    # Configure and build
    cmake -DCMAKE_CXX_COMPILER=clang++ \
          -DSIMDJSON_DEVELOPER_MODE=ON \
          -DSIMDJSON_STATIC_REFLECTION=ON \
          -DBUILD_SHARED_LIBS=OFF \
          $cmake_flags \
          .. > /dev/null 2>&1
    
    if cmake --build . --target benchmark_serialization_twitter > /dev/null 2>&1; then
        echo "**Results:**" >> ../$RESULTS_FILE
        echo "\`\`\`" >> ../$RESULTS_FILE
        
        # Run benchmark 3 times
        for i in {1..3}; do
            ./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection >> ../$RESULTS_FILE
        done
        
        echo "\`\`\`" >> ../$RESULTS_FILE
    else
        echo "**Status:** Build failed" >> ../$RESULTS_FILE
        echo "\`\`\`" >> ../$RESULTS_FILE
        echo "Build error - variant may not be compatible" >> ../$RESULTS_FILE
        echo "\`\`\`" >> ../$RESULTS_FILE
    fi
    
    echo "" >> ../$RESULTS_FILE
    cd ..
}

# Test additional micro-optimizations
echo "Testing additional performance optimizations beyond core algorithmic changes..." 

run_benchmark "Baseline (Reference)" "" "Full optimizations enabled for comparison."

run_benchmark "No Branch Prediction Hints" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_BRANCH_HINTS\"" "Disables __builtin_expect hints to measure branch prediction impact."

run_benchmark "Linear Buffer Growth" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_LINEAR_GROWTH\"" "Uses linear buffer growth instead of exponential (2x) growth strategy."

run_benchmark "No String Escape Fast Path" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_ESCAPE_FAST_PATH\"" "Forces character-by-character escaping, disables memcpy fast path for non-escaped strings."

# Combined variants
run_benchmark "Multiple Micro-optimizations Disabled" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_BRANCH_HINTS -DSIMDJSON_ABLATION_LINEAR_GROWTH -DSIMDJSON_ABLATION_NO_ESCAPE_FAST_PATH\"" "Worst case: disables multiple micro-optimizations simultaneously."

# Cleanup
rm -rf build

echo "---" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE 
echo "**Extended ablation study completed.**" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "This study focuses on micro-optimizations like branch prediction, buffer growth strategies, and fast paths that complement the major algorithmic optimizations (consteval, SIMD)." >> $RESULTS_FILE

echo "Extended ablation results saved to: $RESULTS_FILE"