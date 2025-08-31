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
| **simdjson (reflection)** | 3.47 GB/s | 23.32 μs | C++26 static reflection with consteval |
| **yyjson** | 2.06 GB/s | 39.25 μs | C library |
| **simdjson (DOM)** | 1.63 GB/s | 49.59 μs | Manual DOM serialization |
| **Serde (Rust)** | 1.32 GB/s | 61.39 μs | Via FFI |
| **RapidJSON** | 493 MB/s | 164.20 μs | DOM-based |
| **nlohmann/json** | 243 MB/s | 333.23 μs | Slowest |

## CITM Catalog Results (1.7MB)

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (reflection)** | 2.25 GB/s | 212.09 μs | Fastest with consteval optimization |
| **yyjson** | 1.69 GB/s | 283.10 μs | C library |
| **Serde (Rust)** | 1.18 GB/s | 405.61 μs | Strong performance |
| **simdjson (DOM)** | 786 MB/s | 607.35 μs | Manual implementation |
| **RapidJSON** | 568 MB/s | 840.74 μs | DOM-based |
| **nlohmann/json** | 130 MB/s | 3668.45 μs | Slowest |

## Key Findings

### Performance Leaders
- **simdjson (reflection)** dominates with 3.47 GB/s on Twitter (best-in-class)
- **simdjson (reflection)** achieves 2.25 GB/s on CITM (fastest overall)
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