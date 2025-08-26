# Unified Benchmark Results

This document presents the performance comparison between simdjson's parsing methods and other popular JSON libraries.

## Overview

The benchmark compares:
- **simdjson manual**: Traditional field-by-field extraction
- **simdjson reflection**: C++26 static reflection-based parsing
- **simdjson::from()**: Convenience API
- **nlohmann/json**: Popular C++ JSON library
- **RapidJSON**: High-performance C++ JSON parser

All libraries perform full field extraction for fair comparison. simdjson uses parser reuse (as designed for optimal performance).

## Benchmark Results

### Twitter Dataset (631KB)

| Library/Method | Throughput | vs Manual | Code Complexity |
|----------------|------------|-----------|-----------------|
| simdjson (manual) | ~3,800 MB/s | 100% | ~30 lines |
| simdjson (reflection) | ~3,600 MB/s | 95% | 1 line |
| simdjson::from() | ~3,600 MB/s | 95% | 1 line |
| RapidJSON | ~665 MB/s | 17.5% | ~35 lines |
| nlohmann/json | ~170 MB/s | 4.5% | ~30 lines |

### CITM Catalog Dataset (1.7MB)

| Library/Method | Throughput | vs Manual | Code Complexity |
|----------------|------------|-----------|-----------------|
| simdjson (manual) | ~2,900 MB/s | 100% | ~100 lines |
| simdjson (reflection) | ~2,100 MB/s | 72% | 1 line |
| simdjson::from() | ~2,100 MB/s | 72% | 1 line |
| RapidJSON | ~1,370 MB/s | 47% | ~120 lines |
| nlohmann/json | ~195 MB/s | 7% | ~90 lines |

## Key Findings

1. **Reflection achieves 72-95% of manual performance** with 99% less code
2. **simdjson dominates**: 
   - 5-22x faster than nlohmann/json
   - 2-6x faster than RapidJSON
3. **simdjson::from() matches reflection performance** - identical implementation
4. **Parser reuse is critical** for simdjson's performance

## Code Comparison

### Manual Parsing (30+ lines)
```cpp
for (auto tweet_obj : doc["statuses"]) {
    TwitterTweet tweet;
    tweet.created_at = std::string(std::string_view(tweet_obj["created_at"]));
    tweet.id = uint64_t(tweet_obj["id"]);
    // ... many more fields
}
```

### Reflection/from() (1 line)
```cpp
doc.get(data);  // or simdjson::from(json)
```

## Running the Benchmark

```bash
cd /path/to/simdjson
./benchmark/build_unified_benchmark.sh
./benchmark/unified_benchmark
```

The benchmark loads real JSON files from `jsonexamples/` directory.