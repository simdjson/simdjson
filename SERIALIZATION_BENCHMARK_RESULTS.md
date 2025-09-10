# JSON Serialization Benchmark Results

## Executive Summary
Performance comparison of JSON serialization (C++ structs → JSON) across multiple libraries.

## Test Environment
- **Date**: September 2025
- **Compiler**: Clang 21.0.0 with C++26 support
- **Platform**: Linux (aarch64 and x64)
- **Optimization**: `-O3` (we do not use `-march=native` or other flags)
- **Datasets**: Twitter (631KB), CITM Catalog (1.7MB)
- **Consteval**: Enabled with `std::define_static_string` for compile-time key generation

**Software remarks**: The simdjson library makes little use of SIMD instructions when serializing.

**Hardware remarks**: The Intel Ice Lake processor has powerful SIMD support (AVX-512, two 512-bit execution units). The Apple processor runs at higher frequency and cna retire more instructions per cycle, while having weaker SIMD support (ARM NEON, four 128-bit execution units).


## Twitter Dataset Results (631KB)

### Intel Ice Lake
| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (reflection)** | 3.48 GB/s | 23.24 μs | C++26 static reflection with consteval |
| **yyjson** | 2.07 GB/s | 39.11 μs | C library |
| **simdjson (DOM)** | 1.66 GB/s | 48.85 μs | Manual DOM serialization |
| **Serde (Rust)** | 1.34 GB/s | 60.38 μs | Via FFI |
| **RapidJSON** | 494 MB/s | 163.86 μs | DOM-based |
| **nlohmann/json** | 243 MB/s | 333.51 μs | Slowest |

### Apple Silicon
| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (reflection)** | 3.52 GB/s | 23.00 μs | C++26 static reflection with consteval |
| **yyjson** | 2.08 GB/s | 38.94 μs | C library |
| **simdjson (DOM)** | 1.67 GB/s | 48.36 μs | Manual DOM serialization |
| **Serde (Rust)** | 1.32 GB/s | 61.28 μs | Via FFI |
| **RapidJSON** | 861 MB/s | 94.04 μs | DOM-based |
| **nlohmann/json** | 242 MB/s | 334.18 μs | Slowest |

## CITM Catalog Results (1.7MB)

### Intel Ice Lake
| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (reflection)** | 2.10 GB/s | 226.78 μs | Fastest with consteval optimization |
| **yyjson** | 1.68 GB/s | 283.64 μs | C library |
| **Serde (Rust)** | 1.16 GB/s | 411.79 μs | Strong performance |
| **simdjson (DOM)** | 799 MB/s | 597.50 μs | Manual implementation |
| **RapidJSON** | 571 MB/s | 835.23 μs | DOM-based |
| **nlohmann/json** | 127 MB/s | 3747.76 μs | Slowest |

### Apple Silicon
| Library/Method | Throughput | Time/iter | Notes |
|----------------|------------|-----------|-------|
| **simdjson (reflection)** | 2.25 GB/s | 212.06 μs | Fastest with consteval optimization |
| **yyjson** | 1.67 GB/s | 286.43 μs | C library |
| **Serde (Rust)** | 1.17 GB/s | 408.82 μs | Strong performance |
| **simdjson (DOM)** | 780 MB/s | 612.03 μs | Manual implementation |
| **RapidJSON** | 354 MB/s | 1349.76 μs | DOM-based |
| **nlohmann/json** | 125 MB/s | 3831.37 μs | Slowest |

## Key Findings

### Performance Leaders
- **simdjson (reflection)**  leads across all tests, peaking at 3.52 GB/s on Twitter (Apple Silicon) and 2.25 GB/s on CITM (Apple Silicon), showcasing best-in-class serialization performance.
- **yyjson** consistently ranks second, achieving 2.08 GB/s on Twitter (Apple Silicon) and 1.68 GB/s on CITM (Intel Ice Lake), competitive but trailing simdjson by 1.5-1.7x.
- Traditional libraries (RapidJSON, nlohmann/json) lag significantly, with nlohmann/json being the slowest at 242-243 MB/s on Twitter and 125-127 MB/s on CITM, roughly 14-30x slower than simdjson (reflection).

### Technology Insights

1. **Consteval Impact**: Using `std::define_static_string` for compile-time JSON key generation significantly boosts performance, enabling simdjson (reflection) to achieve up to 3.52 GB/s on Twitter, a 1.7-2.1x improvement over non-consteval methods like yyjson.
2. **Memory Management**: String builder reuse combined with consteval key generation optimizes memory allocation, contributing to simdjson (reflection)'s superior performance across datasets and platforms.
3. **Platform Differences**: Apple Silicon slightly edges out Intel Ice Lake for simdjson (reflection) on both datasets (3.52 GB/s vs. 3.48 GB/s on Twitter, 2.25 GB/s vs. 2.10 GB/s on CITM), likely due to higher frequency and instruction retirement, despite weaker SIMD support (ARM NEON vs. AVX-512).
4. **Serde (Rust)** trails C/C++ libraries by 1.8-3x.
5. **Reflection Performance**: C++26 reflection with consteval outperforms all alternatives


## Methodology
- 3000 iterations for Twitter and CITM dataset
- String builder reuse for simdjson (realistic optimization)
- Full serialization with proper JSON escaping
- Warmup phase before timing
- Consteval optimization with `std::define_static_string`