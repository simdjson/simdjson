#ifndef SIMDJSON_ARM64_NUMBERPARSING_DEFS_H
#define SIMDJSON_ARM64_NUMBERPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/arm64/base.h"
#include "simdjson/arm64/intrinsics.h"
#include "simdjson/internal/numberparsing_tables.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <cstring>

#if SIMDJSON_REGULAR_VISUAL_STUDIO && SIMDJSON_IS_ARM64
// __umulh requires intrin.h
#include <intrin.h>
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO && SIMDJSON_IS_ARM64

namespace simdjson {
namespace arm64 {
namespace numberparsing {

// we don't have SSE, so let us use a scalar function
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
#if SIMDJSON_REGULAR_VISUAL_STUDIO || SIMDJSON_IS_32BITS
#if SIMDJSON_IS_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // SIMDJSON_IS_ARM64
#else // SIMDJSON_REGULAR_VISUAL_STUDIO || SIMDJSON_IS_32BITS
  __uint128_t r = (static_cast<__uint128_t>(value1)) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace numberparsing
} // namespace arm64
} // namespace simdjson

#ifndef SIMDJSON_SWAR_NUMBER_PARSING
#if SIMDJSON_IS_BIG_ENDIAN
#define SIMDJSON_SWAR_NUMBER_PARSING 0
#else
#define SIMDJSON_SWAR_NUMBER_PARSING 1
#endif
#endif

#endif // SIMDJSON_ARM64_NUMBERPARSING_DEFS_H
