# JSON Serialization Benchmark Results

## Executive Summary
Performance comparison of JSON serialization (C++ structs → JSON) across multiple libraries.

## Test Environment
- **Date**: January 2025
- **Compiler**: Clang 21.0.0 with C++26 support
- **Platform**: Linux (aarch64)
- **Optimization**: `-O3`
- **Datasets**: Twitter (631KB), CITM Catalog (1.7MB)
- **Consteval**: Enabled with `std::define_static_string` for compile-time key generation

## Twitter Dataset Results (631KB)

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (reflection)** | 3.49 GB/s | 23.18 μs | C++26 static reflection with consteval |
| **yyjson** | 2.10 GB/s | 38.62 μs | C library |
| **simdjson (DOM)** | 1.69 GB/s | 47.95 μs | Manual DOM serialization |
| **Serde (Rust)** | 1.34 GB/s | 60.40 μs | Via FFI |
| **RapidJSON** | 546 MB/s | 148.38 μs | DOM-based |
| **nlohmann/json** | 242 MB/s | 334.10 μs | Slowest |

## CITM Catalog Results (1.7MB)

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (reflection)** | 2.26 GB/s | 210.84 μs | Fastest with consteval optimization |
| **yyjson** | 1.65 GB/s | 289.18 μs | C library |
| **Serde (Rust)** | 1.19 GB/s | 400.57 μs | Strong performance |
| **simdjson (DOM)** | 800 MB/s | 596.26 μs | Manual implementation |
| **RapidJSON** | 567 MB/s | 841.96 μs | DOM-based |
| **nlohmann/json** | 129 MB/s | 3695.05 μs | Slowest |

## Key Findings

### Performance Leaders
- **simdjson (reflection)** dominates with 3.49 GB/s on Twitter (best-in-class)
- **simdjson (reflection)** achieves 2.26 GB/s on CITM (fastest overall)
- **Consteval optimization** provides significant speedup by pre-computing JSON keys at compile-time

### Technology Insights
1. **Consteval Impact**: Pre-computing JSON keys at compile-time provides major performance gains
2. **Reflection Performance**: C++26 reflection with consteval outperforms all alternatives
3. **Memory Management**: String builder reuse + consteval keys = optimal performance

## Methodology
- 1000 iterations for Twitter dataset
- 500 iterations for CITM dataset
- String builder reuse for simdjson (realistic optimization)
- Full serialization with proper JSON escaping
- Warmup phase before timing
- Consteval optimization with `std::define_static_string`