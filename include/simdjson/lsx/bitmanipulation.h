#ifndef SIMDJSON_LSX_BITMANIPULATION_H
#define SIMDJSON_LSX_BITMANIPULATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/lsx/base.h"
#include "simdjson/lsx/intrinsics.h"
#include "simdjson/lsx/bitmask.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace lsx {
namespace {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
SIMDJSON_NO_SANITIZE_UNDEFINED
// This function can be used safely even if not all bytes have been
// initialized.
// See issue https://github.com/simdjson/simdjson/issues/1965
SIMDJSON_NO_SANITIZE_MEMORY
simdjson_inline int trailing_zeroes(uint64_t input_num) {
  return __builtin_ctzll(input_num);
}

/* result might be undefined when input_num is zero */
simdjson_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
}

/* result might be undefined when input_num is zero */
simdjson_inline int leading_zeroes(uint64_t input_num) {
  return __builtin_clzll(input_num);
}

/* result might be undefined when input_num is zero */
simdjson_inline int count_ones(uint64_t input_num) {
  return __lsx_vpickve2gr_w(__lsx_vpcnt_d(__m128i(v2u64{input_num, 0})), 0);
}

simdjson_inline bool add_overflow(uint64_t value1, uint64_t value2, uint64_t *result) {
  return __builtin_uaddll_overflow(value1, value2,
                                   reinterpret_cast<unsigned long long *>(result));
}

} // unnamed namespace
} // namespace lsx
} // namespace simdjson

#endif // SIMDJSON_LSX_BITMANIPULATION_H
