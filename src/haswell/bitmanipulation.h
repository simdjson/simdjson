#ifndef SIMDJSON_HASWELL_BITMANIPULATION_H
#define SIMDJSON_HASWELL_BITMANIPULATION_H

#include "simdjson/common_defs.h"

#ifdef IS_X86_64
#include "haswell/intrinsics.h"

TARGET_HASWELL
namespace simdjson::haswell {

/* result might be undefined when input_num is zero */
static inline int trailing_zeroes(uint64_t input_num) {
  return static_cast<int>(_tzcnt_u64(input_num));
}

/* result might be undefined when input_num is zero */
static inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return _blsr_u64(input_num);
}

/* result might be undefined when input_num is zero */
static inline int leading_zeroes(uint64_t input_num) {
  return static_cast<int>(_lzcnt_u64(input_num));
}

static inline int hamming(uint64_t input_num) {
  return _popcnt64(input_num);
}
}// namespace simdjson::haswell
UNTARGET_REGION
#endif
#endif //  SIMDJSON_HASWELL_BITMANIPULATION_H