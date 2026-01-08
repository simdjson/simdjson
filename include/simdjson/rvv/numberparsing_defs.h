#ifndef SIMDJSON_RVV_NUMBERPARSING_DEFS_H
#define SIMDJSON_RVV_NUMBERPARSING_DEFS_H

#include "simdjson/base.h"
#include <cstring> // for memcpy

namespace simdjson {
namespace rvv {
namespace numberparsing {

// -------------------------------------------------------------------------
// Fast Number Parsing Primitives (Scalar SWAR Optimized)
// -------------------------------------------------------------------------
// While this is the RVV backend, setting up a vector pipeline for just 8 bytes
// (64 bits) is inefficient compared to scalar 64-bit ALU operations.
// We use proven SWAR (SIMD Within A Register) techniques here.
// -------------------------------------------------------------------------

/**
 * Quickly checks if the next 8 bytes are all ASCII digits ('0'-'9').
 *
 * Uses a bit-hack to verify the range [0x30, 0x39] for all 8 bytes in parallel
 * using standard 64-bit integer registers.
 */
simdjson_inline bool is_made_of_eight_digits_quickly(const uint8_t *chars) {
  uint64_t val;
  // Use memcpy to avoid strict aliasing violations; compiler optimizes this to a single 'ld'
  std::memcpy(&val, chars, sizeof(uint64_t));

  // The '0' character is 0x30. '9' is 0x39.
  // We need to check if every byte is in the range [0x30, 0x39].

  // Logic:
  // 1. (val & 0xF0...) == 0x30...
  //    Verifies that the high nibble of every byte is 3.
  //    This accepts 0x30-0x3F (digits + :;<=>? ).
  //
  // 2. ((val + 0x06...) & 0xF0...) == 0x30...
  //    Verifies that the byte + 6 does not overflow out of the 0x30 range.
  //    - '9' (0x39) + 0x06 = 0x3F (high nibble still 3). OK.
  //    - ':' (0x3A) + 0x06 = 0x40 (high nibble becomes 4). FAIL.

  return ((val & 0xF0F0F0F0F0F0F0F0ULL) == 0x3030303030303030ULL) &&
         (((val + 0x0606060606060606ULL) & 0xF0F0F0F0F0F0F0F0ULL) == 0x3030303030303030ULL);
}

/**
 * Parse 8 digits into a 32-bit integer (e.g., "12345678" -> 12345678).
 *
 * PRECONDITION: The input MUST be 8 valid ASCII digits.
 * Uses a standard divide-and-conquer multiplication approach to merge digits.
 */
simdjson_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  uint64_t val;
  std::memcpy(&val, chars, sizeof(uint64_t));

  // 1. Convert ASCII '0'-'9' (0x30-0x39) to Integers 0-9.
  val = val - 0x3030303030303030ULL;

  // 2. Horizontal aggregation using multiplication.
  //    We process pairs, then quads, then the octet.
  //    Input is Little Endian: d0 d1 d2 ... (where d0 is the first char '1')
  //    We want: d0*10^7 + d1*10^6 ...

  // Step A: Merge pairs (d0,d1) -> d0*10 + d1
  // Formula: (val * 10) + (val >> 8)  (masked to keep relevant bytes)
  const uint64_t mask_pairs = 0x00FF00FF00FF00FFULL;
  const uint64_t mul_10     = 0x000A000A000A000AULL; // Multiplier 10 in every other slot

  uint64_t pairs = ((val * mul_10) + (val >> 8)) & mask_pairs;
  // Now we have 4 results (16-bits apart): [d6d7] [d4d5] [d2d3] [d0d1]
  // (Note: d0d1 means d0*10+d1, e.g. "12" -> 12)

  // Step B: Merge quads [d0d1] and [d2d3] -> [d0d1]*100 + [d2d3]
  const uint64_t mul_100    = 0x0064006400640064ULL; // Multiplier 100
  uint64_t quads = ((pairs * mul_100) + (pairs >> 16));

  // Mask out the garbage in upper bits of 32-bit chunks
  quads &= 0x0000FFFF0000FFFFULL;

  // Step C: Merge halves -> result
  const uint64_t mul_10000  = 0x2710271027102710ULL; // Multiplier 10000
  uint64_t final = ((quads * mul_10000) + (quads >> 32));

  // The result is in the lower 32 bits.
  return static_cast<uint32_t>(final);
}

} // namespace numberparsing
} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_NUMBERPARSING_DEFS_H
