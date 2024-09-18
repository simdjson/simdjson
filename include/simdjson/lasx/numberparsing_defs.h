#ifndef SIMDJSON_LASX_NUMBERPARSING_DEFS_H
#define SIMDJSON_LASX_NUMBERPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/lasx/base.h"
#include "simdjson/lasx/intrinsics.h"
#include "simdjson/internal/numberparsing_tables.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <cstring>

namespace simdjson {
namespace lasx {
namespace numberparsing {

// we don't have appropriate instructions, so let us use a scalar function
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
/** @private */
static simdjson_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  uint64_t val;
  std::memcpy(&val, chars, sizeof(uint64_t));
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return uint32_t((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}

simdjson_inline internal::value128 full_multiplication(uint64_t value1, uint64_t value2) {
  internal::value128 answer;
  __uint128_t r = (static_cast<__uint128_t>(value1)) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
  return answer;
}

} // namespace numberparsing
} // namespace lasx
} // namespace simdjson

#ifndef SIMDJSON_SWAR_NUMBER_PARSING
#if SIMDJSON_IS_BIG_ENDIAN
#define SIMDJSON_SWAR_NUMBER_PARSING 0
#else
#define SIMDJSON_SWAR_NUMBER_PARSING 1
#endif
#endif

#endif // SIMDJSON_LASX_NUMBERPARSING_DEFS_H
