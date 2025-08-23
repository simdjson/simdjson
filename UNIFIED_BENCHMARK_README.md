# Unified JSON Performance Benchmark

## Overview

This is a comprehensive benchmark comparing JSON parsing and serialization performance across multiple libraries using real-world datasets. The benchmark demonstrates the performance of simdjson's new C++26 reflection-based parsing compared to traditional manual parsing and other popular JSON libraries.

## What's Included

### Single Benchmark File
- `benchmark_comparison_unified.cpp` - Complete benchmark implementation
- `build_unified_benchmark.sh` - Build script with automatic dependency detection
- `UNIFIED_BENCHMARK_RESULTS.md` - Detailed results and analysis

### Libraries Tested
1. **simdjson** (3 variants):
   - Manual parsing (traditional field-by-field extraction)
   - Reflection-based parsing (using C++26 static reflection)
   - `simdjson::from()` convenience API

2. **nlohmann/json** (optional):
   - Parse-only mode: Builds internal DOM tree, no extraction to C++ structs
   - With field extraction: Parses AND extracts values into C++ structs

3. **RapidJSON** (optional):
   - Parse-only mode: Builds internal DOM tree, no extraction to C++ structs
   - With field extraction: Parses AND extracts values into C++ structs

### Important: Parse-only vs With Extraction

- **Parse-only**: The JSON is parsed into the library's internal representation (DOM tree). The JSON is validated and can be queried, but values are NOT extracted into user-defined C++ structures. This is what you get with `nlohmann::json::parse()` or `rapidjson::Document::Parse()`.

- **With extraction**: The JSON is parsed AND values are extracted into C++ structs (like `TwitterData` or `CITMCatalog`). This involves iterating through the DOM and copying values into the target structures.

- **simdjson difference**: simdjson doesn't have a separate DOM phase - it always extracts values directly during parsing (on-demand). There's no intermediate DOM tree, which is one of it's innovations.

### Datasets
1. **Twitter** (631KB):
   - Real dataset with complete tweet and user extraction
   - All fields parsed including nested user objects

2. **CITM Catalog** (1.7MB):
   - Real-world event catalog data
   - Full parsing including nested performances, prices, and seat categories

## Building

### Requirements
- Clang with C++26 and P2996 reflection support
- simdjson library
- Optional: nlohmann/json, RapidJSON

### Quick Build
```bash
./build_unified_benchmark.sh
```

### Manual Build
```bash
clang++ -std=c++26 -freflection \
    -DSIMDJSON_STATIC_REFLECTION=1 \
    -DSIMDJSON_EXCEPTIONS=1 \
    -DSIMDJSON_ABLATION_NO_CONSTEVAL \
    -DHAS_NLOHMANN -DHAS_RAPIDJSON \
    -I./include \
    -I./build20/_deps/nlohmann_json-src/include \
    -I./build20/_deps/rapidjson-src/include \
    -O3 benchmark_comparison_unified.cpp \
    singleheader/simdjson.cpp \
    -o benchmark_comparison_unified
```

## Running

```bash
./benchmark_comparison_unified
```

## Results Summary (with steady_clock timing, full extraction)

### Twitter Dataset (631KB) - Complete Extraction
| Method | Throughput | vs Manual | Code Lines |
|--------|------------|-----------|------------|
| simdjson manual | 3,843 MB/s | 100% | ~30 |
| simdjson reflection | 3,653 MB/s | 95.1% | 1 |
| simdjson::from() | 3,678 MB/s | 95.7% | 1 |
| RapidJSON (extraction) | 656 MB/s | 17.1% | ~35 |
| nlohmann (extraction) | 172 MB/s | 4.5% | ~30 |

### CITM Catalog (1.7MB) - Full Nested Structure
| Method | Throughput | vs Manual | Code Lines |
|--------|------------|-----------|------------|
| simdjson manual | 2,251 MB/s | 100% | ~100 |
| simdjson reflection | 2,257 MB/s | ~100% | 1 |
| simdjson::from() | 2,296 MB/s | ~100% | 1 |
| RapidJSON (extraction) | 1,196 MB/s | 53.2% | ~120 |
| nlohmann (extraction) | 192 MB/s | 8.5% | ~90 |

*Note: All three simdjson methods show equivalent performance on CITM (within margin of error)*

## Key Findings

1. **Reflection Performance**: 
   - Twitter: 95% of manual parsing speed
   - CITM: Equivalent to manual parsing (all within margin of error)
   
2. **Speed Advantage Over Other Libraries**:
   - **vs RapidJSON**: 5.6x faster on Twitter, 1.9x on CITM (full extraction)
   - **vs nlohmann**: 21.2x faster on Twitter, 11.8x on CITM (full extraction)
   
3. **All Libraries Parse Complete Datasets**:
   - Full field extraction for fair comparison
   - Complex nested structures included (CITM performances, prices, seat categories)
   
4. **Code Reduction**: 99% less code (1 line vs 100+ lines for CITM)

5. **Parser Reuse Pattern**:
   - simdjson uses parser reuse (as designed for optimal performance)
   - Other libraries create new parsers each iteration (their typical usage)

## Data Structures

### Twitter
```cpp
struct TwitterUser {
    uint64_t id;
    std::string screen_name;
    std::string name;
    // ... more fields
};

struct TwitterTweet {
    std::string created_at;
    uint64_t id;
    std::string text;
    TwitterUser user;
    // ... more fields
};
```

### CITM Catalog
```cpp
struct CITMEvent {
    uint64_t id;
    std::string name;
    std::optional<std::string> description;
    std::vector<uint64_t> subTopicIds;
    std::vector<uint64_t> topicIds;
    // ... more fields
};

struct CITMPerformance {
    uint64_t id;
    std::vector<CITMPrice> prices;
    std::vector<CITMSeatCategory> seatCategories;
    // ... deeply nested structures
};

struct CITMCatalog {
    std::map<std::string, CITMEvent> events;
    std::vector<CITMPerformance> performances;
};
```

## Notes

- The benchmark loads real data files from `jsonexamples/` directory
- All libraries parse the complete JSON structure with full field extraction
- Results may vary based on CPU architecture and compiler optimizations
- The `-DSIMDJSON_ABLATION_NO_CONSTEVAL` flag is currently needed due to a compiler issue