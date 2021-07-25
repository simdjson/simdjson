#ifndef SIMDJSON_PPC64_NUMBERPARSING_H
#define SIMDJSON_PPC64_NUMBERPARSING_H

#include <byteswap.h>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

// we don't have appropriate instructions, so let us use a scalar function
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
static simdjson_really_inline uint32_t
parse_eight_digits_unrolled(const uint8_t *chars) {
  uint64_t val;
  std::memcpy(&val, chars, sizeof(uint64_t));
#ifdef __BIG_ENDIAN__
  val = bswap_64(val);
#endif
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return uint32_t((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#define SIMDJSON_SWAR_NUMBER_PARSING 1

#include "simdjson/generic/numberparsing.h"

#endif // SIMDJSON_PPC64_NUMBERPARSING_H
