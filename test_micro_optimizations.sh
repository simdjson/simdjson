#!/bin/bash

# Micro-optimizations ablation study
# Tests newly implemented low-hanging fruit optimizations

set -e

RESULTS_FILE="micro_optimizations_results.md"

echo "# Micro-optimizations Ablation Study" > $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "**Focus**: Low-hanging fruit optimizations and micro-performance improvements" >> $RESULTS_FILE
echo "**Date:** $(date)" >> $RESULTS_FILE
echo "**Compiler:** $(clang++ --version | head -1)" >> $RESULTS_FILE
echo "**Platform:** $(uname -m)" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# Function to run benchmark with timing
run_optimization_test() {
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
    
    # Configure
    cmake -DCMAKE_CXX_COMPILER=clang++ \
          -DSIMDJSON_DEVELOPER_MODE=ON \
          -DSIMDJSON_STATIC_REFLECTION=ON \
          -DBUILD_SHARED_LIBS=OFF \
          $cmake_flags \
          .. > /dev/null 2>&1
    
    # Measure compilation time
    echo "**Compilation:**" >> ../$RESULTS_FILE
    start_time=$(date +%s.%N)
    
    if cmake --build . --target benchmark_serialization_twitter > /dev/null 2>&1; then
        end_time=$(date +%s.%N)
        compile_time=$(python3 -c "print(f'{$end_time - $start_time:.2f}')")
        
        echo "\`\`\`" >> ../$RESULTS_FILE
        echo "Compilation time: ${compile_time}s" >> ../$RESULTS_FILE
        echo "\`\`\`" >> ../$RESULTS_FILE
        echo "" >> ../$RESULTS_FILE
        
        # Run performance benchmark (3 times)
        echo "**Performance Results:**" >> ../$RESULTS_FILE
        echo "\`\`\`" >> ../$RESULTS_FILE
        
        total_throughput=0
        for i in {1..3}; do
            result=$(./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection)
            echo "$result" >> ../$RESULTS_FILE
            throughput=$(echo "$result" | grep "bench_simdjson_static_reflection" | awk '{print $3}' | sed 's/MB\/s//')
            total_throughput=$(python3 -c "print($total_throughput + $throughput)")
        done
        
        avg_throughput=$(python3 -c "print(f'{$total_throughput / 3:.2f}')")
        echo "" >> ../$RESULTS_FILE
        echo "Average throughput: ${avg_throughput} MB/s" >> ../$RESULTS_FILE
        echo "\`\`\`" >> ../$RESULTS_FILE
        
    else
        echo "\`\`\`" >> ../$RESULTS_FILE
        echo "Build failed - optimization may not be compatible" >> ../$RESULTS_FILE
        echo "\`\`\`" >> ../$RESULTS_FILE
    fi
    
    echo "" >> ../$RESULTS_FILE
    cd ..
}

echo "Testing micro-optimizations..."

# Baseline
run_optimization_test "Baseline (All Optimizations)" "" "Reference implementation with all optimizations enabled."

# Test individual micro-optimizations disabled
run_optimization_test "No Inline Optimizations" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_INLINE_OPTIMIZATIONS\"" "Disables manual inlining, branch hint improvements, and fast-path optimizations."

run_optimization_test "No Memory Prefetching" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_PREFETCH\"" "Disables __builtin_prefetch hints in SIMD string processing and memory access patterns."

run_optimization_test "No Constant Folding" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_CONSTANT_FOLDING\"" "Disables compile-time optimizations like field count pre-computation and enum lookup table generation."

# Combined worst case
run_optimization_test "All Micro-optimizations Disabled" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_INLINE_OPTIMIZATIONS -DSIMDJSON_ABLATION_NO_PREFETCH -DSIMDJSON_ABLATION_NO_CONSTANT_FOLDING\"" "Worst case: disables all newly implemented micro-optimizations."

# Test with major optimizations disabled to see micro-optimization impact
run_optimization_test "No Consteval + All Micro-optimizations" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_CONSTEVAL\"" "Tests micro-optimizations when major algorithmic optimization (consteval) is disabled."

run_optimization_test "No Consteval + No Micro-optimizations" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_CONSTEVAL -DSIMDJSON_ABLATION_NO_INLINE_OPTIMIZATIONS -DSIMDJSON_ABLATION_NO_PREFETCH -DSIMDJSON_ABLATION_NO_CONSTANT_FOLDING\"" "Double worst case: major optimization disabled AND micro-optimizations disabled."

# Cleanup
rm -rf build

echo "---" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "**Micro-optimization study completed.**" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "This study measures the impact of low-hanging fruit optimizations:" >> $RESULTS_FILE
echo "- **Inline optimizations**: Manual inlining, branch hints, fast-path improvements" >> $RESULTS_FILE  
echo "- **Memory prefetching**: Strategic \\`__builtin_prefetch\\` usage in hot loops" >> $RESULTS_FILE
echo "- **Constant folding**: Compile-time computations for field counts and enum tables" >> $RESULTS_FILE

echo "Micro-optimization results saved to: $RESULTS_FILE"