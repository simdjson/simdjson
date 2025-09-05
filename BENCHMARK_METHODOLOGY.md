# Benchmark Methodology

## Overview
This document describes the methodology used for the JSON parsing and serialization benchmarks.

## Test Environment

### Compiler and Flags
- **Compiler**: Clang 21.0.0 with C++26 support
- **Optimization**: `-O3 -march=native`
- **Reflection Support**: `-freflection -fexpansion-statements -stdlib=libc++`
- **Build System**: CMake with unified benchmark executable

### Hardware
Tests were run on Linux (aarch64) with results measured in MB/s throughput.

## Datasets

### Twitter Dataset
- **File**: `jsonexamples/twitter.json`
- **Size**: 631,515 bytes
- **Content**: Array of tweet objects with nested user information
- **Characteristics**: String-heavy (92%), moderate integer content (15%), minimal floats (<0.05%)

### CITM Catalog Dataset
- **File**: `jsonexamples/citm_catalog.json`
- **Size**: 1,727,204 bytes
- **Content**: Event catalog with performances, venues, and pricing
- **Characteristics**: Complex nested structure with maps and arrays

## Benchmark Design

### Iterations
- **Twitter**: 1,000 iterations per benchmark
- **CITM**: 500 iterations per benchmark
- **Warmup**: 10% of main iterations (100 for Twitter, 50 for CITM)

### Memory Management
- **String Builder Reuse**: Serialization benchmarks reuse the same string_builder instance across iterations
- **Parser Instance**: Each parsing iteration uses a fresh parser instance for realistic performance
- **Buffer Clearing**: Buffers are cleared (not deallocated) between iterations to maintain capacity

### Timing Methodology
1. Warmup phase to stabilize caches and branch predictors
2. Timed phase measures wall clock time for all iterations
3. Throughput calculated as: `(data_size * iterations) / total_time`
4. Results reported in MB/s and microseconds per iteration

## Libraries and Versions

### Core Libraries
- **simdjson**: Latest with C++26 reflection support
- **nlohmann/json**: v3.11.2
- **RapidJSON**: v1.1.0
- **yyjson**: v0.8.0

### Optional Libraries
- **Serde (Rust)**: serde_json v1.0 via FFI (parsing and serialization)

## Implementation Details

### Parsing Benchmarks
- All libraries perform full field extraction into C++ structures
- No lazy evaluation or partial parsing
- Validates that all expected fields are present

### Serialization Benchmarks
- Serializes complete C++ structures to JSON strings
- Measures only the serialization time, not structure population
- Output validation ensures correctness

### simdjson Approaches

#### Manual Parsing/Serialization
- Hand-written code for each field
- Explicit error checking
- Maximum control over parsing/serialization order

#### Reflection-Based
- Uses C++26 static reflection
- Automatic field discovery via `std::meta::nonstatic_data_members_of()`
- Compile-time code generation for optimal performance

#### simdjson::from() API
- High-level convenient API
- Type-safe automatic conversion
- Parsing only (no serialization equivalent)

## Running the Benchmarks

### Parsing Benchmarks
```bash
./run_parsing_benchmarks.sh
```

### Serialization Benchmarks
```bash
./run_serialization_benchmarks.sh
```

Both scripts:
1. Build the unified benchmark with all available libraries
2. Compile with appropriate reflection flags
3. Run benchmarks for both datasets
4. Display results in tabular format

## Reproducibility
All benchmarks use deterministic iteration counts and can be reproduced by running the provided scripts. The unified benchmark executable ensures all libraries are tested under identical conditions.