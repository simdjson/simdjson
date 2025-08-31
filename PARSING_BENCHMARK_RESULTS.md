# JSON Parsing Benchmark Results

## Executive Summary
Comprehensive benchmarks comparing JSON parsing performance across multiple libraries using two real-world datasets.

## Test Environment
- **Date**: January 2025
- **Compiler**: Clang 21.0.0 with C++26 support  
- **Platform**: Linux (aarch64)
- **Optimization**: `-O3`
- **Datasets**: Twitter (631KB), CITM Catalog (1.7MB)
- **Reflection**: Using C++26 static reflection (P2996) with consteval optimization

## Twitter Dataset Results (631KB)

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (manual)** | 3.96 GB/s | 151.92 μs | Hand-written parsing code |
| **simdjson (reflection)** | 3.87 GB/s | 155.80 μs | C++26 static reflection |
| **simdjson::from()** | 3.86 GB/s | 156.17 μs | High-level API |
| **yyjson** | 3.20 GB/s | 188.36 μs | C library |
| **Serde (Rust)** | 1.72 GB/s | 349.33 μs | Via FFI |
| **RapidJSON** | 666 MB/s | 904.75 μs | SAX parsing |
| **nlohmann/json** | 177 MB/s | 3397.99 μs | Full extraction |

## CITM Catalog Results (1.7MB)

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **yyjson** | 3.64 GB/s | 452.40 μs | Fastest overall |
| **simdjson::from()** | 2.16 GB/s | 761.76 μs | Convenient API |
| **simdjson (reflection)** | 2.15 GB/s | 767.06 μs | Reflection-based |
| **simdjson (manual)** | 1.96 GB/s | 840.36 μs | Manual parsing |
| **RapidJSON** | 1.39 GB/s | 1183.08 μs | DOM parsing |
| **Serde (Rust)** | 592 MB/s | 2781.54 μs | Cross-language overhead |
| **nlohmann/json** | 204 MB/s | 8089.41 μs | Slowest |

## Key Findings

### Performance Leaders
- **simdjson (manual)** leads in Twitter parsing at 3.96 GB/s
- **yyjson** leads in CITM parsing at 3.64 GB/s
- **simdjson (reflection)** provides excellent performance with convenience

### Technology Insights
1. **C++26 Reflection**: simdjson's reflection approach achieves 98% of manual performance on Twitter
2. **Native Performance**: C/C++ libraries significantly outperform cross-language solutions
3. **API Trade-offs**: High-level APIs (simdjson::from) have minimal overhead (<1% on Twitter)

## Methodology
- 1000 iterations for Twitter dataset
- 500 iterations for CITM dataset  
- Fresh parser instance per iteration (realistic usage)
- Full field extraction (no lazy evaluation)
- Warmup phase before timing