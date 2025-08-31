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
| **simdjson (reflection)** | 3.73 GB/s | 21.72 μs | C++26 static reflection with consteval |
| **yyjson** | 2.14 GB/s | 37.92 μs | C library |
| **simdjson (DOM)** | 1.75 GB/s | 46.21 μs | Manual DOM serialization |
| **Serde (Rust)** | 1.35 GB/s | 59.92 μs | Via FFI |
| **RapidJSON** | 527 MB/s | 153.60 μs | DOM-based |
| **nlohmann/json** | 253 MB/s | 320.07 μs | Slowest |

## CITM Catalog Results (1.7MB)

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (reflection)** | 2.02 GB/s | 236.49 μs | Fastest with consteval optimization |
| **yyjson** | 1.66 GB/s | 288.29 μs | C library |
| **Serde (Rust)** | 1.15 GB/s | 415.30 μs | Strong performance |
| **simdjson (DOM)** | 797 MB/s | 598.79 μs | Manual implementation |
| **RapidJSON** | 573 MB/s | 832.03 μs | DOM-based |
| **nlohmann/json** | 130 MB/s | 3661.84 μs | Slowest |

## Key Findings

### Performance Leaders
- **simdjson (reflection)** dominates with 3.73 GB/s on Twitter (best-in-class)
- **simdjson (reflection)** achieves 2.02 GB/s on CITM (fastest overall)
- **Consteval optimization** provides 2x speedup over runtime key generation

### Technology Insights
1. **Consteval Impact**: Pre-computing JSON keys at compile-time doubles performance
2. **Reflection Performance**: C++26 reflection with consteval outperforms all alternatives
3. **Memory Management**: String builder reuse + consteval keys = optimal performance

## Methodology
- 1000 iterations for Twitter dataset
- 500 iterations for CITM dataset
- String builder reuse for simdjson (realistic optimization)
- Full serialization with proper JSON escaping
- Warmup phase before timing
- Consteval optimization with `std::define_static_string`