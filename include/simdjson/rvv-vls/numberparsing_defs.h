#ifndef SIMDJSON_RVV_VLS_NUMBERPARSING_DEFS_H
#define SIMDJSON_RVV_VLS_NUMBERPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv-vls/base.h"
#include "simdjson/internal/numberparsing_tables.h"
#include "simdjson/internal/numberparsing.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <cstring>

#ifdef JSON_TEST_NUMBERS // for unit testing
void found_invalid_number(const uint8_t *buf);
void found_integer(int64_t result, const uint8_t *buf);
void found_unsigned_integer(uint64_t result, const uint8_t *buf);
void found_float(double result, const uint8_t *buf);
#endif

namespace simdjson {
namespace rvv_vls {
namespace numberparsing {

/** @private */
static simdjson_inline uint32_t parse_eight_digits_unrolled(const char *chars) {
  uint64_t val;
#if __riscv_misaligned_fast
  memcpy(&val, chars, sizeof(uint64_t));
#else
  val = __riscv_vmv_x(__riscv_vreinterpret_u64m1(__riscv_vlmul_ext_u8m1(__riscv_vle8_v_u8mf2((uint8_t*)chars, 8))));
#endif
  return internal::parse_eight_digits_swar(val);
}

/** @private */
static simdjson_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  return parse_eight_digits_unrolled(reinterpret_cast<const char *>(chars));
}

/** @private */
simdjson_inline internal::value128 full_multiplication(uint64_t value1, uint64_t value2) {
  internal::value128 answer;
  __uint128_t r = (static_cast<__uint128_t>(value1)) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
  return answer;
}

} // namespace numberparsing
} // namespace rvv_vls
} // namespace simdjson

#define SIMDJSON_SWAR_NUMBER_PARSING 1

#endif // SIMDJSON_RVV_VLS_NUMBERPARSING_DEFS_H
