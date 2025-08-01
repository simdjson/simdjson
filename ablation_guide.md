# Ablation Study Guide - simdjson C++26 Reflection

This guide explains how to run and analyze ablation studies for the simdjson C++26 reflection-based JSON serialization implementation.

## Prerequisites

1. **Compiler**: Clang with C++26 reflection support (bloomberg/clang-p2996)
2. **Build Tools**: CMake 3.25+, Make
3. **Analysis Tools**: Python 3, bc (basic calculator)
4. **System**: Linux/macOS with sufficient memory for compilation

## Quick Start

### Running the Complete Ablation Study

```bash
# Run both benchmarks with defaults (10 runs Twitter, 20 runs CITM)
./ablation_study.sh

# Run only Twitter benchmark with custom runs
./ablation_study.sh -b twitter -r 20

# Run with compilation time measurement
./ablation_study.sh --compilation-time

# Analyze results
python3 calculate_stats.py
```

## Important: Baseline Performance Verification

**CRITICAL**: Before running any ablation study, verify that your baseline performance is approximately **3,200 MB/s** for the Twitter benchmark. If you see significantly lower numbers (e.g., ~1,600 MB/s), the consteval optimization may not be active.

### Verify Baseline Performance

```bash
cd build
cmake .. -DCMAKE_CXX_COMPILER=clang++ \
         -DSIMDJSON_DEVELOPER_MODE=ON \
         -DSIMDJSON_STATIC_REFLECTION=ON \
         -DBUILD_SHARED_LIBS=OFF \
         -DCMAKE_BUILD_TYPE=Release
make benchmark_serialization_twitter -j4
./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection
```

Expected output:
```
bench_simdjson_static_reflection : 3164.70 MB/s   0.79 Ms/s
```

If you see ~1,600 MB/s instead, try:
1. Clean rebuild: `rm -rf build/*`
2. Verify include files are correct in `json_builder.h`
3. Check that `SIMDJSON_CONSTEVAL` is defined

## Understanding the Ablation Study

### What It Measures

The ablation study systematically disables optimizations to measure their individual contributions:

1. **Baseline**: All optimizations enabled (reference)
2. **No Consteval**: Disables compile-time string processing
3. **No SIMD Escaping**: Disables vectorized string escaping
4. **No Fast Digits**: Disables optimized integer-to-string conversion
5. **No Branch Hints**: Disables CPU branch prediction hints
6. **Linear Growth**: Uses linear instead of exponential buffer growth

### Output Format

Results are saved in CSV format to the `ablation_results` directory:
- `twitter_ablation_results.csv`: Twitter benchmark results
- `citm_ablation_results.csv`: CITM benchmark results
- `ablation_summary.txt`: Human-readable summary

CSV format:
```
Variant,Mean_MB/s,StdDev,CV%,Runs,Impact%,CompileTime_s
baseline,3164.70,36.93,1.17,10,0,44.02
no_consteval,1571.96,26.00,1.65,10,-50.3,40.31
```

## Step-by-Step Process

### 1. Prepare the Environment

```bash
# Navigate to simdjson directory
cd /path/to/simdjson

# Ensure build directory exists
mkdir -p build

# Make scripts executable
chmod +x ablation_study.sh
chmod +x calculate_stats.py
```

### 2. Run the Ablation Study

```bash
# Basic run (both benchmarks with optimal runs)
./ablation_study.sh

# Advanced options
./ablation_study.sh --help

# Run only CITM with custom runs (due to high variance)
./ablation_study.sh -b citm -c 30

# Include compilation time measurements
./ablation_study.sh --compilation-time

# Verbose mode for debugging
./ablation_study.sh --verbose
```

#### Key Options

- `-b, --benchmark`: Choose twitter, citm, or both (default: both)
- `-r, --runs`: Number of runs for Twitter (default: 10)
- `-c, --citm-runs`: Number of runs for CITM (default: 20 due to higher variance)
- `--compilation-time`: Also measure compilation time for each variant
- `-o, --output`: Output directory for results (default: ablation_results)

### 3. Monitor Progress

The script will show progress for each variant:
```
=== Processing variant: baseline ===
Results: Twitter,baseline,3164.70,36.93,10,44.02s compilation

=== Processing variant: no_consteval ===
Results: Twitter,no_consteval,1571.96,26.00,10,40.31s compilation
```

### 4. Analyze Results

```bash
# Process results with statistics
python3 calculate_stats.py

# Or specify a custom results file
python3 calculate_stats.py my_ablation_results.txt
```

Output will show:
- Mean throughput for each variant
- Standard deviation and coefficient of variation
- Performance impact relative to baseline
- Compilation time differences

Example output:
```
================================================================================
Twitter Benchmark Results
================================================================================

Variant                   Mean (MB/s)  StdDev     CV (%)   Impact       Compile (s)
------------------------- ------------ ---------- -------- ------------ ------------
**Baseline**              3164.70      ±36.93     1.17     Reference    44.02
No Consteval              1571.96      ±26.00     1.65     -50.3%       40.31
No Simd Escaping          2285.77      ±33.34     1.46     -27.8%       41.51
```

## Troubleshooting

### Issue: Low Baseline Performance

If baseline is ~1,600 MB/s instead of ~3,200 MB/s:

1. **Clean rebuild**:
   ```bash
   cd build
   rm -rf *
   cmake .. # with proper flags
   make benchmark_serialization_twitter -j4
   ```

2. **Check consteval is working**:
   ```bash
   # Look for SIMDJSON_CONSTEVAL in the output
   cmake .. -DCMAKE_BUILD_TYPE=Release -DSIMDJSON_STATIC_REFLECTION=ON -DCMAKE_VERBOSE_MAKEFILE=ON
   ```

3. **Verify includes**: Check that `json_builder.h` includes `json_string_builder-inl.h`

### Issue: CITM Benchmark Fails

The CITM benchmark has been fixed using `std::define_static_string`. If you still encounter issues, check `citm_issue.md` for details.

### Issue: Script Permissions

```bash
chmod +x ablation_study.sh
chmod +x calculate_stats.py
```

### Issue: Missing Dependencies

```bash
# Install bc (basic calculator)
sudo apt-get install bc  # Ubuntu/Debian
brew install bc          # macOS
```

## Manual Testing

To test individual optimization variants manually:

```bash
cd build

# Test specific variant
cmake .. -DCMAKE_CXX_FLAGS="-DSIMDJSON_ABLATION_NO_CONSTEVAL" -DCMAKE_BUILD_TYPE=Release
make benchmark_serialization_twitter -j4
./benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter -f simdjson_static_reflection
```

## Understanding Results

### Performance Tiers

1. **Critical Optimizations (>25% impact)**:
   - Consteval: ~50% performance improvement
   - SIMD Escaping: ~28% performance improvement

2. **Moderate Optimizations (5-10% impact)**:
   - Fast Digits: ~7% performance improvement

3. **Minor Optimizations (<5% impact)**:
   - Branch Hints: ~2% performance improvement
   - Buffer Growth Strategy: ~2% performance improvement

### Compilation Time

Interestingly, optimizations generally *reduce* compilation time:
- Baseline: ~44 seconds
- With optimizations disabled: ~40-42 seconds

This suggests that compile-time computation (consteval) actually speeds up overall compilation.

## Advanced Usage

### Running Specific Variants Only

Modify the `ABLATION_VARIANTS` array in `ablation_study.sh`:

```bash
declare -A ABLATION_VARIANTS=(
    ["baseline"]=""
    ["no_consteval"]="-DSIMDJSON_ABLATION_NO_CONSTEVAL"
    # Add or remove variants as needed
)
```

### Custom Benchmarks

To add a new benchmark:

1. Add benchmark path to the script
2. Update the benchmark selection logic
3. Ensure the benchmark follows the expected output format

### Integration with CI/CD

```yaml
# Example GitHub Actions workflow
- name: Run Ablation Study
  run: |
    ./ablation_study.sh -r 5 -c 10 -o ci_results
    python3 calculate_stats.py ci_results > ablation_summary.txt

- name: Upload Results
  uses: actions/upload-artifact@v3
  with:
    name: ablation-results
    path: |
      ci_ablation_results.txt
      ablation_summary.txt
```

## Best Practices

1. **Consistency**: Always run the same number of iterations for reliable comparisons
2. **Clean State**: Start with a clean build directory for each full study
3. **System Load**: Run on a quiet system to minimize variance
4. **Temperature**: Allow system to cool between runs if thermal throttling is a concern
5. **Documentation**: Record system specs and compiler versions with results

## Further Reading

- `ablation_results.md`: Detailed analysis of optimization impacts
- `citm_issue.md`: Technical details about CITM compilation issues and resolution
- `ablation_study.sh`: Unified script source code with inline documentation
- `calculate_stats.py`: Statistical analysis implementation