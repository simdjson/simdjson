#ifndef SIMDJSON_HASWELL_BITMASK_H
#define SIMDJSON_HASWELL_BITMASK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/haswell/base.h"
#include "simdjson/haswell/intrinsics.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace haswell {
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
  return (int)_tzcnt_u64(input_num);
#else // SIMDJSON_REGULAR_VISUAL_STUDIO
  ////////
  // You might expect the next line to be equivalent to
  // return (int)_tzcnt_u64(input_num);
  // but the generated code differs and might be less efficient?
  ////////
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return _blsr_u64(input_num);
}

/* result might be undefined when input_num is zero */
simdjson_inline int leading_zeroes(uint64_t input_num) {
  return int(_lzcnt_u64(input_num));
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

#if SIMDJSON_REGULAR_VISUAL_STUDIO
using borrow_t = unsigned char;
#else
using borrow_t = unsigned long long;
#endif

simdjson_inline uint64_t subtract_borrow(const uint64_t value1, const uint64_t value2, borrow_t& borrow) noexcept {
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned __int64 result;
  borrow = _subborrow_u64(borrow, value1, value2, &result);
  return result;
#else
  return __builtin_subcll(value1, value2, borrow, &borrow);
#endif
}

simdjson_inline uint64_t subtract_borrow_out(const uint64_t value1, const uint64_t value2, bool& borrow_out) noexcept {
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned __int64 result;
  borrow_out = _subborrow_u64(0, value1, value2, &result);
  return result;
#else
  unsigned long long result;
  borrow_out = __builtin_usubll_overflow(value1, value2, &result);
  return result;
#endif
}

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_inline uint64_t prefix_xor(const uint64_t bitmask) noexcept {
  // There should be no such thing with a processor supporting avx2
  // but not clmul.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return _mm_cvtsi128_si64(result);
}

} // namespace bitmask
} // namespace haswell
} // namespace simdjson

#endif // SIMDJSON_HASWELL_BITMASK_H
