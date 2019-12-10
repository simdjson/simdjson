#ifndef SIMDJSON_ARM64_BITMANIPULATION_H
#define SIMDJSON_ARM64_BITMANIPULATION_H

#include "simdjson/common_defs.h"
#include "simdjson/portability.h"
#include "arm64/intrinsics.h"

namespace simdjson::arm64 {

/* result might be undefined when input_num is zero */
static inline  int trailing_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  unsigned long ret;
  // Search the mask data from least significant bit (LSB) 
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else
  return __builtin_ctzll(input_num);
#endif// _MSC_VER
}

/* result might be undefined when input_num is zero */
static inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
}

/* result might be undefined when input_num is zero */
static inline int leading_zeroes(uint64_t input_num) {
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
static inline int hamming(uint64_t input_num) {
   return vaddv_u8(vcnt_u8((uint8x8_t)input_num));
}

}// namespace simdjson::arm64
#endif //  SIMDJSON_ARM64_BITMANIPULATION_H