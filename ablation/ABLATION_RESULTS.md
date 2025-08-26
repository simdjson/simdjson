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

## Running the Study

```bash
cd /path/to/simdjson
./ablation/run_ablation_study.sh

# With compilation time analysis
./ablation/run_ablation_study.sh --enable_compilation
```

Results are saved to `ablation/results/` (gitignored).