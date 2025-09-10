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


**Hardware remarks**: The Intel Ice Lake processor has powerful SIMD support (AVX-512, two 512-bit execution units). The Apple processor runs at higher frequency and cna retire more instructions per cycle, while having weaker SIMD support (ARM NEON, four 128-bit execution units).

## Twitter Dataset Results (631KB)
### Intel Ice Lake
| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson::from()** | 3.90 GB/s | 154.59 μs  | High-level API, uses C++26 reflection |
| **simdjson (reflection)** | 3.75 GB/s | 160.60 μs | C++26 static reflection |
| **simdjson (manual)** | 2.67 GB/s | 225.82 μs | Hand-written parsing code |
| **yyjson** | 1.82 GB/s | 330.94 μs  | C library |
| **Serde (Rust)** | 1.09 GB/s | 551.83 μs    | Via FFI |
| **RapidJSON** | 387 MB/s | 1557.00 μs  | Full extraction |
| **nlohmann/json** | 117 MB/s | 5346.73 μs  | Full extraction |

### Apple Silicon
| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (manual)** | 4.36 GB/s | 138.04 μs | Hand-written parsing code |
| **simdjson::from()** | 4.17 GB/s | 144.45 μs | High-level API, uses C++26 reflection |
| **simdjson (reflection)** | 4.09 GB/s | 147.19 μs | C++26 static reflection |
| **yyjson** | 2.23 GB/s | 269.71 μs | C library |
| **Serde (Rust)** | 1.72 GB/s | 349.75 μs | Via FFI |
| **RapidJSON** | 658 MB/s | 915.14 μs | Full extraction |
| **nlohmann/json** | 172 MB/s | 3501.02 μs | Full extraction |

## CITM Catalog Results (1.7MB)
### Intel Ice Lake

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (manual)** | 2.32 GB/s | 709.51 μs  | Manual parsing |
| **simdjson (reflection)** | 1.85 GB/s | 890.34 μs  | C++26 static reflection |
| **simdjson::from()** | 1.76 GB/s | 890.34 μs | Convenient API, uses C++26 reflection |
| **yyjson** | 1.46 GB/s | 1130.75 μs | Full extraction |
| **RapidJSON** | 552 GB/s | 2986.10 μs | Full extraction |
| **Serde (Rust)** | 279 MB/s | 5903.36 μs  | Cross-language overhead |
| **nlohmann/json** | 107187 MB/s |  15378.63 μs  | Full extraction |

### Apple Silicon

| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (manual)** | 3.01 GB/s | 546.57 μs | Manual parsing |
| **yyjson** | 2.68 GB/s | 614.32 μs | Full extraction |
| **simdjson::from()** | 2.67 GB/s | 617.03 μs | Convenient API, uses C++26 reflection |
| **simdjson (reflection)** | 2.66 GB/s | 620.07 μs | C++26 static reflection |
| **RapidJSON** | 1.22 GB/s | 1354.62 μs | Full extraction |
| **Serde (Rust)** | 535 MB/s | 3081.24 μs | Cross-language overhead |
| **nlohmann/json** | 186 MB/s | 8874.02 μs | Full extraction |

## Key Findings


### Performance Leaders
- On Apple Silicon, **simdjson (manual)** tops both datasets: 4.36 GB/s for Twitter and 3.01 GB/s for CITM.
- On Intel Ice Lake, **simdjson::from()** leads Twitter at 3.90 GB/s, while **simdjson (manual)** leads CITM at 2.32 GB/s.
- simdjson variants consistently dominate the top positions across platforms and datasets, with yyjson as a strong contender especially on Apple Silicon for CITM (2.68 GB/s, nearly matching simdjson::from() at 2.67 GB/s).


### Technology Insights
1. **C++26 Reflection**: simdjson's reflection approach shows variability by platform and dataset, achieving 140% of manual performance on Intel for Twitter (3.75 GB/s vs. 2.67 GB/s) and 94% on Apple Silicon (4.09 GB/s vs. 4.36 GB/s), averaging about 111%; for CITM, it reaches 80% on Intel (1.85 GB/s vs. 2.32 GB/s) and 88% on Apple Silicon (2.66 GB/s vs. 3.01 GB/s), averaging 84%.
2. **Native Performance**: C/C++ libraries (simdjson, yyjson, RapidJSON, nlohmann/json) significantly outperform Rust's Serde, whichranks near the bottom in all cases.
3. **API Trade-offs**: High-level APIs like simdjson::from() incur minimal overhead, often matching or exceeding reflection and manual methods (e.g., leading on Intel Twitter with 3.90 GB/s).
4. **Fair Comparison**: All libraries now extract complete data structures including nested objects

## Methodology
- 3000 iterations for Twitter and CITM dataset
- Fresh parser instance per iteration (realistic usage)
- Full field extraction (no lazy evaluation)
- Warmup phase before timing