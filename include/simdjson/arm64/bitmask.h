#ifndef SIMDJSON_ARM64_BITMASK_H
#define SIMDJSON_ARM64_BITMASK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/arm64/base.h"
#include "simdjson/arm64/intrinsics.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace arm64 {
namespace bitmask {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
SIMDJSON_NO_SANITIZE_UNDEFINED
// This function can be used safely even if not all bytes have been
// initialized.
// See issue https://github.com/simdjson/simdjson/issues/1965
SIMDJSON_NO_SANITIZE_MEMORY
simdjson_inline int trailing_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
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
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
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

/* result might be undefined when input_num is zero */
simdjson_inline int count_ones(uint64_t input_num) {
   return vaddv_u8(vcnt_u8(vcreate_u8(input_num)));
}


#if defined(__GNUC__) // catches clang and gcc
/**
 * ARM has a fast 64-bit "bit reversal function" that is handy. However,
 * it is not generally available as an intrinsic function under Visual
 * Studio (though this might be changing). Even under clang/gcc, we
 * apparently need to invoke inline assembly.
 */
/*
 * We use SIMDJSON_PREFER_REVERSE_BITS as a hint that algorithms that
 * work well with bit reversal may use it.
 */
#define SIMDJSON_PREFER_REVERSE_BITS 1

/* reverse the bits */
simdjson_inline uint64_t reverse_bits(uint64_t input_num) {
  uint64_t rev_bits;
  __asm("rbit %0, %1" : "=r"(rev_bits) : "r"(input_num));
  return rev_bits;
}

/**
 * Flips bit at index 63 - lz. Thus if you have 'leading_zeroes' leading zeroes,
 * then this will set to zero the leading bit. It is possible for leading_zeroes to be
 * greating or equal to 63 in which case we trigger undefined behavior, but the output
 * of such undefined behavior is never used.
 **/
SIMDJSON_NO_SANITIZE_UNDEFINED
simdjson_inline uint64_t zero_leading_bit(uint64_t rev_bits, int leading_zeroes) {
  return rev_bits ^ (uint64_t(0x8000000000000000) >> leading_zeroes);
}

#endif

simdjson_inline uint64_t add_carry_out(uint64_t value1, uint64_t value2, bool &carry_out) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  uint64_t result = value1 + value2;
  carry_out = result < value1;
  return result;
#else
  unsigned long long result;
  carry_out = __builtin_uaddll_overflow(value1, value2, &result);
  return result;
#endif
}

simdjson_inline uint64_t subtract_borrow(uint64_t value1, uint64_t value2, bool &borrow) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  value2 += borrow;
  uint64_t result = value1 - value2;
  borrow = value1 > value2;
  return result;
#else
  unsigned long long result;
  bool borrow1 = __builtin_usubll_overflow(value1, value2, &result);
  borrow = borrow1 | __builtin_usubll_overflow(result, borrow, &result);
  return result;
#endif
}

simdjson_inline uint64_t subtract_borrow_out(uint64_t value1, uint64_t value2, bool &borrow_out) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  uint64_t result = value1 - value2;
  borrow_out = result > value1;
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
simdjson_inline uint64_t prefix_xor(uint64_t bitmask) {
  /////////////
  // We could do this with PMULL, but it is apparently slow.
  //
  //#ifdef __ARM_FEATURE_CRYPTO // some ARM processors lack this extension
  //return vmull_p64(-1ULL, bitmask);
  //#else
  // Analysis by @sebpop:
  // When diffing the assembly for src/stage1_find_marks.cpp I see that the eors are all spread out
  // in between other vector code, so effectively the extra cycles of the sequence do not matter
  // because the GPR units are idle otherwise and the critical path is on the FP side.
  // Also the PMULL requires two extra fmovs: GPR->FP (3 cycles in N1, 5 cycles in A72 )
  // and FP->GPR (2 cycles on N1 and 5 cycles on A72.)
  ///////////
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
}

} // unnamed namespace
} // namespace arm64
} // namespace simdjson

#endif
