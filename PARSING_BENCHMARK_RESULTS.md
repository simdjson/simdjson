# JSON Parsing Benchmark Results

## Executive Summary
Comprehensive benchmarks comparing JSON parsing performance across multiple libraries using two real-world datasets.

## Test Environment
- **Date**: September 2025
- **Compiler**: Clang 21.0.0 with C++26 support
- **Platform**: Linux (aarch64 and x64)
- **Optimization**: `-O3`
- **Datasets**: Twitter (631KB), CITM Catalog (1.7MB)
- **Reflection**: Using C++26 static reflection (P2996) with consteval optimization

## Twitter Dataset Results (631KB)
### Intel Ice Lake
| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (manual)** | 2.24 GB/s | 268.58 μs | Hand-written parsing code |
| **simdjson (reflection)** | 2.06 GB/s | 290.68 μs | C++26 static reflection |
| **simdjson::from()** | 2.07 GB/s | 284.19 μs | High-level API |
| **yyjson** | 1.82 GB/s | 330.94 μs  | C library |
| **Serde (Rust)** | 1.09 GB/s | 551.83 μs    | Via FFI |
| **RapidJSON** | 387 MB/s | 1557.00 μs  | Full extraction |
| **nlohmann/json** | 117 MB/s | 5346.73 μs  | Full extraction |

### Apple Silicon
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
### Intel Ice Lake

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **yyjson** | 1.34 GB/s | 1229.51 μs  | Full extraction |
| **simdjson (reflection)** | 1.43 GB/s | 1150.54 μs  | Reflection-based |
| **simdjson::from()** | 1.37 GB/s | 1203.19 μs | Convenient API |
| **simdjson (manual)** | 1.32 GB/s | 1247.85 μs  | Manual parsing |
| **RapidJSON** | 552 GB/s | 2986.10 μs | Full extraction |
| **Serde (Rust)** | 279 MB/s | 5903.36 μs  | Cross-language overhead |
| **nlohmann/json** | 107187 MB/s |  15378.63 μs  | Full extraction |

### Apple Silicon

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
- **yyjson** leads in CITM parsing on Apple Silicon, being 20% than simdjson but no faster on x64;
- **simdjson (reflection)** provides excellent performance with convenience

### Technology Insights
1. **C++26 Reflection**: simdjson's reflection approach achieves 95% of manual performance on Twitter
2. **Native Performance**: C/C++ libraries significantly outperform Rust Serde
3. **API Trade-offs**: High-level APIs (simdjson::from) have minimal overhead
4. **Fair Comparison**: All libraries now extract complete data structures including nested objects

## Methodology
- 1000 iterations for Twitter dataset
- 500 iterations for CITM dataset
- Fresh parser instance per iteration (realistic usage)
- Full field extraction (no lazy evaluation)
- Warmup phase before timing