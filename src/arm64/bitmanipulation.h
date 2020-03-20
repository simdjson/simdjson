#ifndef SIMDJSON_ARM64_BITMANIPULATION_H
#define SIMDJSON_ARM64_BITMANIPULATION_H

#include "simdjson.h"
#include "arm64/intrinsics.h"

namespace simdjson::arm64 {

#ifndef _MSC_VER
// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out. 
__attribute__((no_sanitize("undefined"))) // this is deliberate
#endif // _MSC_VER
/* result might be undefined when input_num is zero */
really_inline int trailing_zeroes(uint64_t input_num) {

#ifdef _MSC_VER
  unsigned long ret;
  // Search the mask data from least significant bit (LSB) 
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else
  return __builtin_ctzll(input_num);
#endif // _MSC_VER

} // namespace simdjson::arm64

/* result might be undefined when input_num is zero */
really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
}

/* result might be undefined when input_num is zero */
really_inline int leading_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB) 
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif// _MSC_VER
}

/* result might be undefined when input_num is zero */
really_inline int count_ones(uint64_t input_num) {
   return vaddv_u8(vcnt_u8((uint8x8_t)input_num));
}

really_inline bool add_overflow(uint64_t value1, uint64_t value2, uint64_t *result) {
#ifdef _MSC_VER
  // todo: this might fail under visual studio for ARM
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

#ifdef _MSC_VER
#pragma intrinsic(_umul128) // todo: this might fail under visual studio for ARM
#endif

really_inline bool mul_overflow(uint64_t value1, uint64_t value2, uint64_t *result) {
#ifdef _MSC_VER
  // todo: this might fail under visual studio for ARM
  uint64_t high;
  *result = _umul128(value1, value2, &high);
  return high;
#else
  return __builtin_umulll_overflow(value1, value2, (unsigned long long *)result);
#endif
}

} // namespace simdjson::arm64

#endif // SIMDJSON_ARM64_BITMANIPULATION_H
