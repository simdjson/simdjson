#ifndef SIMDJSON_WESTMERE_NUMBERPARSING_H
#define SIMDJSON_WESTMERE_NUMBERPARSING_H

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
/**
 * We could use a scalar version of parse_eight_digits_unrolled that compiles
 * to something like
 *
 *        and     rax, qword ptr [rdi]
 *        imul    rax, rax, 2561
 *        shr     rax, 8
 *        movabs  rcx, 71777214294589695
 *        and     rcx, rax
 *        imul    rax, rcx, 6553601
 *        shr     rax, 16
 *        movabs  rcx, 281470681808895
 *        and     rcx, rax
 *        movabs  rax, 42949672960001
 *        imul    rax, rcx
 *        shr     rax, 32
 *
 * The vectorized sequence below is favorable, as it compiles to
 *       vmovq   xmm0, qword ptr [rdi]           # xmm0 = mem[0],zero
 *       vpaddb  xmm0, xmm0, xmmword ptr [rip + .LCPI0_0]
 *       vpmaddubsw      xmm0, xmm0, xmmword ptr [rip + .LCPI0_1]
 *       vpmaddwd        xmm0, xmm0, xmmword ptr [rip + .LCPI0_2]
 *       vpackusdw       xmm0, xmm0, xmm0
 *       vpmaddwd        xmm0, xmm0, xmmword ptr [rip + .LCPI0_3]
 *       vmovd   eax, xmm0
 * even though it does twice the work and looks far more complicated.
 */
static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  // this actually computes *16* values so we are being wasteful.
  const __m128i ascii0 = _mm_set1_epi8('0');
  const __m128i mul_1_10 =
      _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
  const __m128i mul_1_100 = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
  const __m128i mul_1_10000 =
      _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1);
  // _mm_loadu_si128(reinterpret_cast<const __m128i *>(chars)) is faster
  // but _mm_loadu_si64(chars) avoids buffer overflows.
  const __m128i input = _mm_sub_epi8(
      _mm_loadl_epi64(reinterpret_cast<const __m128i *>(chars)), ascii0);
  const __m128i t1 = _mm_maddubs_epi16(input, mul_1_10);
  const __m128i t2 = _mm_madd_epi16(t1, mul_1_100);
  const __m128i t3 = _mm_packus_epi32(t2, t2);
  const __m128i t4 = _mm_madd_epi16(t3, mul_1_10000);
  return _mm_cvtsi128_si32(
      t4); // only captures the sum of the first 8 digits, drop the rest
}


} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#define SIMDJSON_SWAR_NUMBER_PARSING 1

#include "simdjson/generic/numberparsing.h"

#endif //  SIMDJSON_WESTMERE_NUMBERPARSING_H
