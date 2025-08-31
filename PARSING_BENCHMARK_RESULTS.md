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
| **simdjson (manual)** | 3.89 GB/s | 154.71 μs | Hand-written parsing code |
| **simdjson (reflection)** | 3.72 GB/s | 162.03 μs | C++26 static reflection |
| **simdjson::from()** | 3.71 GB/s | 162.52 μs | High-level API |
| **yyjson** | 3.14 GB/s | 191.93 μs | C library |
| **Serde (Rust)** | 1.73 GB/s | 348.95 μs | Via FFI |
| **RapidJSON** | 662 MB/s | 909.38 μs | SAX parsing |
| **nlohmann/json** | 172 MB/s | 3500.96 μs | Full extraction |

## CITM Catalog Results (1.7MB)

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **yyjson** | 2.68 GB/s | 614.48 μs | Full extraction (fair comparison) |
| **simdjson::from()** | 2.15 GB/s | 766.65 μs | Convenient API |
| **simdjson (reflection)** | 2.14 GB/s | 767.97 μs | Reflection-based |
| **simdjson (manual)** | 1.92 GB/s | 858.46 μs | Manual parsing |
| **RapidJSON** | 1.37 GB/s | 1200.20 μs | DOM parsing |
| **Serde (Rust)** | 577 MB/s | 2854.99 μs | Cross-language overhead |
| **nlohmann/json** | 198 MB/s | 8329.62 μs | Slowest |

## Key Findings

### Performance Leaders
- **simdjson (manual)** leads in Twitter parsing at 3.89 GB/s
- **yyjson** leads in CITM parsing at 2.68 GB/s (with full extraction)
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