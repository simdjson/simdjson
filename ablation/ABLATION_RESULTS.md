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

## Expected Results

### Twitter Serialization (String-Heavy)
| Optimization | Impact When Disabled | Contribution |
|--------------|---------------------|--------------|
| Consteval | -50% | **+100% performance** |
| SIMD Escaping | -28% | **+39% performance** |
| Fast Digits | -7% | +7% performance |
| Branch Hints | -2% | +2% performance |
| Buffer Growth | -2% | +2% performance |

### CITM Serialization (Mixed Data)
| Optimization | Impact When Disabled | Contribution |
|--------------|---------------------|--------------|
| Consteval | -37% | **+59% performance** |
| SIMD Escaping | -1% | +1% performance |
| Fast Digits | ±0% | Minimal impact |
| Branch Hints | -1% | +1% performance |
| Buffer Growth | -1% | +1% performance |

## Key Findings

1. **Consteval is critical**: Provides 59-100% performance improvement by processing string escaping at compile time
2. **SIMD benefits string-heavy workloads**: Up to 39% improvement on Twitter, minimal on CITM
3. **Optimizations compound**: Combined effect provides ~2x overall performance

## Running the Study

```bash
cd /path/to/simdjson
./ablation/run_ablation_study.sh

# With compilation time analysis
./ablation/run_ablation_study.sh --enable_compilation
```

Results are saved to `ablation/results/` (gitignored).