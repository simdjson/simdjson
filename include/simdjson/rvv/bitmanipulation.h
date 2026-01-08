#ifndef SIMDJSON_RVV_BITMANIPULATION_H
#define SIMDJSON_RVV_BITMANIPULATION_H

#include "simdjson/base.h"

namespace simdjson {
namespace rvv {
namespace {

// -------------------------------------------------------------------------
// Bit Manipulation Primitives
// -------------------------------------------------------------------------
// These functions operate on the scalar uint64_t bitmasks we extract from
// the RVV vector unit. They are critical for the "Stage 1" structural loop.
//
// We use compiler builtins (__builtin_*) because:
// 1. They automatically map to RISC-V Zbb instructions (cpop, ctz) if
//    compiled with -march=..._zbb.
// 2. They provide highly optimized software fallbacks if Zbb is missing.
// -------------------------------------------------------------------------

/**
 * Count the number of set bits (1s) in a 64-bit integer.
 */
simdjson_inline int count_ones(uint64_t input_num) {
  return __builtin_popcountll(input_num);
}

/**
 * Find the index of the first set bit (least significant bit).
 * Returns the 0-based index (0-63).
 *
 * PRECONDITION: input_num must NOT be zero.
 * (Behavior is undefined for input_num == 0 by the standard, though often 64 on hardware).
 */
simdjson_inline int trailing_zeroes(uint64_t input_num) {
  return __builtin_ctzll(input_num);
}

/**
 * Clears the lowest set bit in the integer.
 * e.g., 0b...10100 -> 0b...10000
 */
simdjson_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num - 1);
}

/**
 * Returns the integer with only the lowest set bit kept.
 * e.g., 0b...10100 -> 0b...00100
 */
simdjson_inline uint64_t get_lowest_bit(uint64_t input_num) {
  // Standard two's complement isolation trick
  // -input_num is equivalent to (~input_num + 1)
  return input_num & (static_cast<uint64_t>(0) - input_num);
}

} // unnamed namespace
} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_BITMANIPULATION_H