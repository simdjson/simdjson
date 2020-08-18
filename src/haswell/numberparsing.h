#ifndef SIMDJSON_HASWELL_NUMBERPARSING_H
#define SIMDJSON_HASWELL_NUMBERPARSING_H

namespace {
namespace SIMDJSON_IMPLEMENTATION {

static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  // this actually computes *16* values so we are being wasteful.
  const __m128i ascii0 = _mm_set1_epi8('0');
  const __m128i mul_1_10 =
      _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
  const __m128i mul_1_100 = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
  const __m128i mul_1_10000 =
      _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1);
  const __m128i input = _mm_sub_epi8(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(chars)), ascii0);
  const __m128i t1 = _mm_maddubs_epi16(input, mul_1_10);
  const __m128i t2 = _mm_madd_epi16(t1, mul_1_100);
  const __m128i t3 = _mm_packus_epi32(t2, t2);
  const __m128i t4 = _mm_madd_epi16(t3, mul_1_10000);
  return _mm_cvtsi128_si32(
      t4); // only captures the sum of the first 8 digits, drop the rest
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#define SWAR_NUMBER_PARSING

#include "generic/stage2/numberparsing.h"

#endif // SIMDJSON_HASWELL_NUMBERPARSING_H
