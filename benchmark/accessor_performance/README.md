# Accessor Performance Benchmarks (C++26)

These benchmarks compare the performance of runtime vs compile-time JSON accessors.
For the comparison to be meaningful, you must build simdjson with support for
C++26 reflexion. See the `p2996` repository in the main project directory.

## Files

- `accessor_benchmark.h` - Common benchmark framework and test data
- `runtime_accessors.h` - Runtime `at_path()` benchmarks
- `compile_time_accessors.h` - Compile-time `at_path_compiled()` benchmarks (requires C++26 reflection)

## Benchmarks

Each benchmark measures parsing + single field access:

1. **accessor_simple** - Simple field: `.name`
2. **accessor_nested** - Nested field: `.address.city`
3. **accessor_deep** - Deep nested field: `.address.coordinates.lat`

## Building (Linux/macOS)

```bash
cmake -B build -D SIMDJSON_STATIC_REFLECTION=ON -DSIMDJSON_DEVELOPER_MODE=ON
cmake --build build --target=bench_ondemand
```

The `SIMDJSON_STATIC_REFLECTION` will be made unnecessary once mainstream compilers
begin supporting C++26 sufficiently well.

## Running (Linux/macOS)


```bash
# Run all accessor benchmarks
./build/bench_ondemand --benchmark_filter="accessor"
```

## Results

We find that compile-time accessors show performance improvements that scale with path depth:
- Simple fields: ~1.2x faster
- Nested fields: ~1.5x faster
- Deep nested fields: ~1.8x faster

The speedup comes from eliminating runtime path parsing and conversion overhead.
