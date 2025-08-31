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
| **simdjson (manual)** | 3.83 GB/s | 157.43 μs | Hand-written parsing code |
| **simdjson (reflection)** | 3.62 GB/s | 166.30 μs | C++26 static reflection |
| **simdjson::from()** | 3.61 GB/s | 166.93 μs | High-level API |
| **yyjson** | 3.15 GB/s | 191.07 μs | C library |
| **Serde (Rust)** | 1.71 GB/s | 352.45 μs | Via FFI |
| **RapidJSON** | 659 MB/s | 913.41 μs | Full extraction |
| **nlohmann/json** | 172 MB/s | 3507.81 μs | Full extraction |

## CITM Catalog Results (1.7MB)

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **yyjson** | 2.67 GB/s | 616.14 μs | Full extraction |
| **simdjson (reflection)** | 2.19 GB/s | 753.16 μs | Reflection-based |
| **simdjson::from()** | 2.14 GB/s | 769.66 μs | Convenient API |
| **simdjson (manual)** | 1.89 GB/s | 873.39 μs | Manual parsing |
| **RapidJSON** | 1.17 GB/s | 1409.37 μs | Full extraction |
| **Serde (Rust)** | 590 MB/s | 2793.82 μs | Cross-language overhead |
| **nlohmann/json** | 187 MB/s | 8815.76 μs | Full extraction |

## Key Findings

### Performance Leaders
- **simdjson (manual)** leads in Twitter parsing at 3.83 GB/s
- **yyjson** leads in CITM parsing at 2.67 GB/s
- **simdjson (reflection)** provides excellent performance with convenience

### Technology Insights
1. **C++26 Reflection**: simdjson's reflection approach achieves 95% of manual performance on Twitter
2. **Native Performance**: C/C++ libraries significantly outperform cross-language solutions
3. **API Trade-offs**: High-level APIs (simdjson::from) have minimal overhead (<1% vs reflection)
4. **Fair Comparison**: All libraries now extract complete data structures including nested objects

## Methodology
- 1000 iterations for Twitter dataset
- 500 iterations for CITM dataset  
- Fresh parser instance per iteration (realistic usage)
- Full field extraction (no lazy evaluation)
- Warmup phase before timing