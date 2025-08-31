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
| **simdjson (manual)** | 4.09 GB/s | 147.13 μs | Hand-written parsing code |
| **simdjson::from()** | 3.90 GB/s | 154.31 μs | High-level API |
| **simdjson (reflection)** | 3.87 GB/s | 155.56 μs | C++26 static reflection |
| **yyjson** | 3.21 GB/s | 187.45 μs | C library |
| **Serde (Rust)** | 1.80 GB/s | 334.14 μs | Via FFI |
| **RapidJSON** | 681 MB/s | 884.26 μs | SAX parsing |
| **nlohmann/json** | 180 MB/s | 3353.70 μs | Full extraction |

## CITM Catalog Results (1.7MB)

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **yyjson** | 3.68 GB/s | 448.06 μs | Fastest overall |
| **simdjson (reflection)** | 2.23 GB/s | 737.46 μs | Reflection-based |
| **simdjson::from()** | 2.22 GB/s | 741.29 μs | Convenient API |
| **simdjson (manual)** | 2.00 GB/s | 821.61 μs | Manual parsing |
| **RapidJSON** | 1.36 GB/s | 1214.03 μs | DOM parsing |
| **Serde (Rust)** | 600 MB/s | 2745.33 μs | Cross-language overhead |
| **nlohmann/json** | 210 MB/s | 7850.07 μs | Slowest |

## Key Findings

### Performance Leaders
- **yyjson** leads in CITM parsing at 3.68 GB/s
- **simdjson (manual)** leads in Twitter parsing at 4.09 GB/s
- **simdjson (reflection)** provides excellent performance with convenience

### Technology Insights
1. **C++26 Reflection**: simdjson's reflection approach achieves 95% of manual performance on Twitter
2. **Native Performance**: C/C++ libraries significantly outperform cross-language solutions
3. **API Trade-offs**: High-level APIs (simdjson::from) have minimal overhead (~5%)

## Methodology
- 1000 iterations for Twitter dataset
- 500 iterations for CITM dataset  
- Fresh parser instance per iteration (realistic usage)
- Full field extraction (no lazy evaluation)
- Warmup phase before timing