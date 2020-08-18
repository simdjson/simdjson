#ifndef SIMDJSON_ARM64_NUMBERPARSING_H
#define SIMDJSON_ARM64_NUMBERPARSING_H

namespace {
namespace arm64 {

// we don't have SSE, so let us use a scalar function
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  uint64_t val;
  memcpy(&val, chars, sizeof(uint64_t));
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return uint32_t((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}

} // namespace arm64
} // unnamed namespace

#define SWAR_NUMBER_PARSING

#include "generic/stage2/numberparsing.h"

#endif // SIMDJSON_ARM64_NUMBERPARSING_H
