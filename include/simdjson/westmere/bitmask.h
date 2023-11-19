#ifndef SIMDJSON_WESTMERE_BITMASK_H
#define SIMDJSON_WESTMERE_BITMASK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/westmere/base.h"
#include "simdjson/westmere/intrinsics.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace westmere {
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
#else // SIMDJSON_REGULAR_VISUAL_STUDIO
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
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
#endif// SIMDJSON_REGULAR_VISUAL_STUDIO
}

#if SIMDJSON_REGULAR_VISUAL_STUDIO
simdjson_inline unsigned __int64 count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows in this kernel
  return __popcnt64(input_num);// Visual Studio wants two underscores
}
#else
simdjson_inline long long int count_ones(uint64_t input_num) {
  return _popcnt64(input_num);
}
#endif


simdjson_inline uint64_t add_carry_out(const uint64_t value1, const uint64_t value2, bool& carry_out) noexcept {
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned __int64 result;
  carry_out = _addcarry_u64(0, value1, value2, &result);
  return result;
#else
  unsigned long long result;
  carry_out = __builtin_uaddll_overflow(value1, value2, &result);
  return result;
#endif
}

simdjson_inline uint64_t subtract_borrow(const uint64_t value1, const uint64_t value2, bool& borrow) noexcept {
  unsigned long long result;
  bool borrow1 = __builtin_usubll_overflow(value1, value2, &result);
  borrow = borrow1 | __builtin_usubll_overflow(result, borrow, &result);
  return result;
}

simdjson_inline uint64_t subtract_borrow_out(const uint64_t value1, const int64_t value2, bool& borrow_out) noexcept {
  unsigned long long result;
  borrow_out = __builtin_usubll_overflow(value1, value2, &result); // 2 (one to set )
  return result;
}

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_inline uint64_t prefix_xor(const uint64_t bitmask) {
  // There should be no such thing with a processing supporting avx2
  // but not clmul.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return _mm_cvtsi128_si64(result);
}

} // unnamed namespace
} // namespace westmere
} // namespace simdjson

#endif // SIMDJSON_WESTMERE_BITMASK_H
