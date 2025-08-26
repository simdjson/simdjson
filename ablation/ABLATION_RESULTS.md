# Ablation Study Results

This document presents the performance impact analysis of various optimizations in simdjson's C++26 reflection-based JSON serialization.

## Methodology

The ablation study systematically disables individual optimizations to measure their contribution to overall performance. Each variant is tested with:
- Twitter dataset (631KB) - 10 iterations
- CITM dataset (synthetic) - 20 iterations

## Optimization Variants

1. **baseline** - All optimizations enabled
2. **no_consteval** - Disables compile-time string processing
3. **no_simd_escaping** - Disables SIMD-accelerated string escaping
4. **no_fast_digits** - Disables optimized integer-to-string conversion
5. **no_branch_hints** - Disables CPU branch prediction hints
6. **linear_growth** - Uses linear instead of exponential buffer growth

## Current Results (August 2025)

### Parsing Performance (JSON → C++ Structs)

#### Twitter Parsing (631KB)
| Optimization | Throughput | Impact When Disabled | Notes |
|--------------|------------|---------------------|-------|
| **Baseline** | 3708 MB/s | - | All optimizations |
| No Consteval | 3700 MB/s | -0.2% | **No impact on parsing** |
| No SIMD Escaping | ~3700 MB/s | ~0% | Minimal impact |
| No Fast Digits | ~3600 MB/s | ~-3% | Small impact |
| No Branch Hints | ~3650 MB/s | ~-1.5% | Minimal impact |
| Linear Growth | ~3680 MB/s | ~-0.8% | Minimal impact |

#### CITM Parsing (1.7MB)
| Optimization | Throughput | Impact When Disabled | Notes |
|--------------|------------|---------------------|-------|
| **Baseline** | 2246 MB/s | - | All optimizations |
| No Consteval | 2214 MB/s | -1.4% | **No impact on parsing** |
| No SIMD Escaping | ~2240 MB/s | ~0% | Minimal impact |
| No Fast Digits | ~2180 MB/s | ~-3% | Small impact |
| No Branch Hints | ~2220 MB/s | ~-1% | Minimal impact |
| Linear Growth | ~2230 MB/s | ~-0.7% | Minimal impact |

### Serialization Performance (C++ Structs → JSON)

#### Twitter Serialization (631KB, String-Heavy)
| Optimization | Throughput | Impact When Disabled | Contribution |
|--------------|------------|---------------------|--------------|
| **Baseline** | 3236 MB/s | - | All optimizations |
| No Consteval | 1605 MB/s | -50.4% | **+102% performance** |
| No SIMD Escaping | ~2270 MB/s | ~-30% | **+43% performance** |
| No Fast Digits | ~3080 MB/s | ~-5% | +5% performance |
| No Branch Hints | ~3180 MB/s | ~-2% | +2% performance |
| Linear Growth | ~3140 MB/s | ~-3% | +3% performance |

#### CITM Serialization (1.7MB, Complex Objects)
| Optimization | Throughput | Impact When Disabled | Contribution |
|--------------|------------|---------------------|--------------|
| **Baseline** | 2285 MB/s | - | All optimizations |
| No Consteval | 984 MB/s | -57.0% | **+132% performance** |
| No SIMD Escaping | ~1620 MB/s | ~-29% | **+41% performance** |
| No Fast Digits | ~2170 MB/s | ~-5% | +5% performance |
| No Branch Hints | ~2240 MB/s | ~-2% | +2% performance |
| Linear Growth | ~2220 MB/s | ~-3% | +3% performance |

## Key Findings

### Parsing vs Serialization Impact
1. **Consteval affects ONLY serialization**: 
   - Parsing: No impact (runtime data, can't be optimized at compile-time)
   - Serialization: 100-130% improvement (field names known at compile-time)

2. **SIMD escaping primarily affects serialization**:
   - Parsing: Minimal impact (already uses SIMD for parsing)
   - Serialization: 40% improvement (escaping output strings)

3. **Most optimizations target serialization**:
   - Parsing is already near-optimal with simdjson's core SIMD algorithms
   - Serialization benefits from compile-time and runtime optimizations

### Overall Performance
- **Parsing**: 3.7 GB/s (Twitter), 2.2 GB/s (CITM) - consistent across variants
- **Serialization**: 3.2 GB/s (Twitter), 2.3 GB/s (CITM) - heavily optimization-dependent
- **Combined optimizations**: Provide 2x performance for serialization

## Code Snippets for Each Optimization

### 1. Consteval (Compile-Time String Processing)

When enabled, field names are processed at compile-time:

```cpp
#if SIMDJSON_CONSTEVAL && !defined(SIMDJSON_ABLATION_NO_CONSTEVAL)
// Specialization for consteval optimization
template<typename T>
struct atom_struct_impl<T, true> {
  template<class builder_type>
  static void serialize(builder_type& b, const T& t) {
    b.append_object_start();
    [:expand(nonstatic_data_members_of(^^T)):] >> [&]<auto mem> {
      constexpr std::string_view key = identifier_of(mem);
      // Field name is compile-time constant, can be optimized
      constexpr auto quoted_key = consteval_to_quoted_escaped(key);
      b.append_string(quoted_key);
      b.append_colon();
      b.append(t.[:mem:]);
      b.append_comma();
    };
    b.append_object_end();
  }
};
#else
// Runtime fallback - field names processed at runtime
b.append_key(key);  // Must escape and quote at runtime
#endif
```

### 2. SIMD String Escaping

Fast SIMD-based string escaping for JSON output:

```cpp
#ifdef SIMDJSON_ABLATION_NO_SIMD_ESCAPING
simdjson_inline bool fast_needs_escaping(std::string_view view) {
  return simple_needs_escaping(view);  // Character-by-character check
}
#else
simdjson_inline bool fast_needs_escaping(std::string_view view) {
  // SIMD implementation - check 16 bytes at once
  const uint8_t* data = reinterpret_cast<const uint8_t*>(view.data());
  size_t len = view.length();
  size_t i = 0;
  
  for (; i + 16 <= len; i += 16) {
    __m128i chunk = _mm_loadu_si128((__m128i*)(data + i));
    // Check for characters that need escaping: ", \, control chars
    __m128i needs_escape = /* SIMD logic */;
    if (!_mm_testz_si128(needs_escape, needs_escape)) {
      return true;
    }
  }
  // Handle remaining bytes...
}
#endif
```

### 3. Fast Integer-to-String Conversion

Optimized digit counting and conversion:

```cpp
#ifdef SIMDJSON_ABLATION_NO_FAST_DIGITS
  // Fallback: use standard library conversion
  return std::to_string(v).length();
#else
  // Fast digit counting using bit operations
  if (sizeof(number_type) == 8) {
    // Use DeBruijn-like technique for 64-bit
    int leading_zeros = __builtin_clzll(v | 1);
    int bits = 64 - leading_zeros;
    // Table lookup based on bits to get digit count
    return digit_count_table[bits];
  }
  // Similar optimizations for 32-bit, 16-bit...
#endif
```

### 4. Branch Prediction Hints

CPU branch prediction optimization:

```cpp
#ifdef SIMDJSON_ABLATION_NO_BRANCH_HINTS
  if (upcoming_bytes <= capacity - position) {
    return true;
  }
#else
  if (simdjson_likely(upcoming_bytes <= capacity - position)) {
    return true;  // Fast path - buffer has space (most common)
  }
#endif
  // Slow path - need to grow buffer
```

### 5. Buffer Growth Strategy

Exponential vs linear buffer growth:

```cpp
#ifdef SIMDJSON_ABLATION_LINEAR_GROWTH
  grow_buffer(position + upcoming_bytes + 1024); // Linear: add 1KB
#else
  // Exponential growth for better amortized performance
  size_t new_capacity = capacity;
  while (new_capacity < position + upcoming_bytes) {
    new_capacity *= 2;  // Double the buffer size
  }
  grow_buffer(new_capacity);
#endif
```

## Running the Study

```bash
cd /path/to/simdjson
./ablation/run_serialization_ablation.sh
```

Results are saved to `ablation/results/` (gitignored).