#!/bin/bash

# Quick compilation time study
echo "=== Compilation Time Study ==="
echo "Date: $(date)"
echo ""

# Function to measure compilation time
measure_compile_time() {
    local variant_name="$1"
    local cmake_flags="$2"
    
    echo "Testing: $variant_name"
    
    # Clean build
    rm -rf build
    mkdir build
    cd build
    
    # Configure silently
    cmake -DCMAKE_CXX_COMPILER=clang++ \
          -DSIMDJSON_DEVELOPER_MODE=ON \
          -DSIMDJSON_STATIC_REFLECTION=ON \
          -DBUILD_SHARED_LIBS=OFF \
          $cmake_flags \
          .. > /dev/null 2>&1
    
    # Measure compilation time
    start_time=$(date +%s)
    cmake --build . --target benchmark_serialization_twitter > /dev/null 2>&1
    end_time=$(date +%s)
    compile_time=$((end_time - start_time))
    
    echo "  Compilation time: ${compile_time}s"
    
    # Quick performance test
    result=$(./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection)
    echo "  $result"
    echo ""
    
    cd ..
}

# Test variants
measure_compile_time "Baseline" ""
measure_compile_time "No SIMD Escaping" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_SIMD_ESCAPING\""
measure_compile_time "No Consteval" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_CONSTEVAL\""
measure_compile_time "No Fast Digits" "-DCMAKE_CXX_FLAGS=\"-DSIMDJSON_ABLATION_NO_FAST_DIGITS\""

# Cleanup
rm -rf build

echo "=== Study Complete ==="