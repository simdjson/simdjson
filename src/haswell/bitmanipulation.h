#ifndef SIMDJSON_HASWELL_BITMANIPULATION_H
#define SIMDJSON_HASWELL_BITMANIPULATION_H

#include "simdjson/common_defs.h"

#ifdef IS_X86_64
#include "haswell/intrinsics.h"

TARGET_HASWELL
namespace simdjson::haswell {

/* result might be undefined when input_num is zero */
really_inline int trailing_zeroes(uint64_t input_num) {
  return static_cast<int>(_tzcnt_u64(input_num));
}

/* result might be undefined when input_num is zero */
really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return _blsr_u64(input_num);
}

/* result might be undefined when input_num is zero */
really_inline int leading_zeroes(uint64_t input_num) {
  return static_cast<int>(_lzcnt_u64(input_num));
}

really_inline int hamming(uint64_t input_num) {
  return _popcnt64(input_num);
}

really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef _MSC_VER
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

#ifdef _MSC_VER
#pragma intrinsic(_umul128)
#endif
really_inline bool mul_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef _MSC_VER
  uint64_t high;
  *result = _umul128(value1, value2, &high);
  return high;
#else
  return __builtin_umulll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}
}// namespace simdjson::haswell
UNTARGET_REGION
#endif
#endif //  SIMDJSON_HASWELL_BITMANIPULATION_H