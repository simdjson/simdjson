# Accessor Performance Benchmarks

These benchmarks compare the performance of runtime vs compile-time JSON accessors.

## Files

- `accessor_benchmark.h` - Common benchmark framework and test data
- `runtime_accessors.h` - Runtime `at_path()` benchmarks
- `compile_time_accessors.h` - Compile-time `at_path_compiled()` benchmarks (requires C++26 reflection)

## Benchmarks

Each benchmark measures parsing + single field access:

1. **accessor_simple** - Simple field: `.name`
2. **accessor_nested** - Nested field: `.address.city`
3. **accessor_deep** - Deep nested field: `.address.coordinates.lat`

## Running

```bash
# Run all accessor benchmarks
./bench_ondemand --benchmark_filter="accessor"

# Run with repetitions for more stable results
./bench_ondemand --benchmark_filter="accessor" --benchmark_repetitions=5
```

## Results

Compile-time accessors show performance improvements that scale with path depth:
- Simple fields: ~1.2x faster
- Nested fields: ~1.5x faster
- Deep nested fields: ~1.8x faster

The speedup comes from eliminating runtime path parsing and conversion overhead.
