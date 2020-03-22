#ifndef SIMDJSON_HASWELL_BITMANIPULATION_H
#define SIMDJSON_HASWELL_BITMANIPULATION_H

#include "simdjson.h"

#include "haswell/intrinsics.h"

TARGET_HASWELL
namespace simdjson::haswell {

#ifndef _MSC_VER
// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
__attribute__((no_sanitize("undefined")))  // this is deliberate
#endif
really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  return (int)_tzcnt_u64(input_num);
#else
  ////////
  // You might expect the next line to be equivalent to 
  // return (int)_tzcnt_u64(input_num);
  // but the generated code differs and might be less efficient?
  ////////
  return __builtin_ctzll(input_num);
#endif// _MSC_VER
}

/* result might be undefined when input_num is zero */
really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return _blsr_u64(input_num);
}

/* result might be undefined when input_num is zero */
really_inline int leading_zeroes(uint64_t input_num) {
  return static_cast<int>(_lzcnt_u64(input_num));
}

really_inline int count_ones(uint64_t input_num) {
#ifdef _MSC_VER
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num);// Visual Studio wants two underscores
#else
  return _popcnt64(input_num);
#endif
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

#endif // SIMDJSON_HASWELL_BITMANIPULATION_H
