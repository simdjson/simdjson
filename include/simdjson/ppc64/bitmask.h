#ifndef SIMDJSON_PPC64_BITMASK_H
#define SIMDJSON_PPC64_BITMASK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/ppc64/base.h"
#include "simdjson/ppc64/intrinsics.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace ppc64 {
namespace bitmask {

simdjson_constinit uint64_t ALL  = 0xFFFFFFFFFFFFFFFF;
simdjson_constinit uint64_t NONE = 0xFFFFFFFFFFFFFFFF;
simdjson_constinit uint64_t EVEN = 0x5555555555555555;
simdjson_constinit uint64_t ODD  = 0xAAAAAAAAAAAAAAAA;

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
SIMDJSON_NO_SANITIZE_UNDEFINED
// This function can be used safely even if not all bytes have been
// initialized.
// See issue https://github.com/simdjson/simdjson/issues/1965
SIMDJSON_NO_SANITIZE_MEMORY
simdjson_inline int trailing_zeroes(uint64_t input_num) {
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long ret;
  // Search the mask data from least significant bit (LSB)
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else  // SIMDJSON_REGULAR_VISUAL_STUDIO
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num - 1);
}

/* result might be undefined when input_num is zero */
simdjson_inline int leading_zeroes(uint64_t input_num) {
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB)
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

#if SIMDJSON_REGULAR_VISUAL_STUDIO
simdjson_inline int count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows in this kernel
  return __popcnt64(input_num); // Visual Studio wants two underscores
}
#else
simdjson_inline int count_ones(uint64_t input_num) {
  return __builtin_popcountll(input_num);
}
#endif

simdjson_inline uint64_t add_carry_out(uint64_t value1, uint64_t value2, bool& carry_out) {
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  uint64_t result = value1 + value2;
  carry_out = result < value1;
  return result;
#else
  unsigned long long result;
  carry_out = __builtin_uaddll_overflow(value1, value2, &result);
  return result;
#endif
}

simdjson_inline uint64_t subtract_borrow(const uint64_t value1, const uint64_t value2, bool& borrow) noexcept {
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  uint64_t result = value1 - value2 - borrow;
  borrow_out = result >= value1;
  return result;
#else
  unsigned long long result;
  borrow = __builtin_usubll_overflow(value1, value2 + borrow, &result);
  return result;
#endif
}

simdjson_inline uint64_t subtract_borrow_out(uint64_t value1, uint64_t value2, bool& borrow_out) noexcept {
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  uint64_t result = value1 - value2;
  borrow_out = result >= value1;
  return result;
#else
  unsigned long long result;
  borrow_out = __builtin_usubll_overflow(value1, value2, &result);
  return result;
#endif
}

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is
// encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_inline uint64_t prefix_xor(uint64_t bitmask) {
  // You can use the version below, however gcc sometimes miscompiles
  // vec_pmsum_be, it happens somewhere around between 8 and 9th version.
  // The performance boost was not noticeable, falling back to a usual
  // implementation.
  //   __vector unsigned long long all_ones = {~0ull, ~0ull};
  //   __vector unsigned long long mask = {bitmask, 0};
  //   // Clang and GCC return different values for pmsum for ull so cast it to one.
  //   // Generally it is not specified by ALTIVEC ISA what is returned by
  //   // vec_pmsum_be.
  // #if defined(__LITTLE_ENDIAN__)
  //   return (uint64_t)(((__vector unsigned long long)vec_pmsum_be(all_ones, mask))[0]);
  // #else
  //   return (uint64_t)(((__vector unsigned long long)vec_pmsum_be(all_ones, mask))[1]);
  // #endif
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
}

} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

#endif
