#ifndef SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H
#define SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "haswell/architecture.h"
#include "haswell/simd_input.h"
#include "haswell/simdutf8check.h"
#include "simdjson/stage1_find_marks.h"

TARGET_HASWELL
namespace simdjson::haswell {

static really_inline uint64_t compute_quote_mask(uint64_t quote_bits) {
  // There should be no such thing with a processing supporting avx2
  // but not clmul.
  uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFFu), 0));
  return quote_mask;
}

static really_inline void find_whitespace_and_structurals(simd_input<ARCHITECTURE> in,
  uint64_t &whitespace, uint64_t &structurals) {

  #ifdef SIMDJSON_NAIVE_STRUCTURAL
  // You should never need this naive approach, but it can be useful
  // for research purposes
  const __m256i mask_open_brace = _mm256_set1_epi8(0x7b);
  __m256i struct_lo = _mm256_cmpeq_epi8(in.lo, mask_open_brace);
  __m256i struct_hi = _mm256_cmpeq_epi8(in.hi, mask_open_brace);
  const __m256i mask_close_brace = _mm256_set1_epi8(0x7d);
  struct_lo = _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_close_brace));
  struct_hi = _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_close_brace));
  const __m256i mask_open_bracket = _mm256_set1_epi8(0x5b);
  struct_lo = _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_open_bracket));
  struct_hi = _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_open_bracket));
  const __m256i mask_close_bracket = _mm256_set1_epi8(0x5d);
  struct_lo = _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_close_bracket));
  struct_hi = _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_close_bracket));
  const __m256i mask_column = _mm256_set1_epi8(0x3a);
  struct_lo = _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_column));
  struct_hi = _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_column));
  const __m256i mask_comma = _mm256_set1_epi8(0x2c);
  struct_lo = _mm256_or_si256(struct_lo, _mm256_cmpeq_epi8(in.lo, mask_comma));
  struct_hi = _mm256_or_si256(struct_hi, _mm256_cmpeq_epi8(in.hi, mask_comma));
  uint64_t structural_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(struct_lo));
  uint64_t structural_res_1 = _mm256_movemask_epi8(struct_hi);
  structurals = (structural_res_0 | (structural_res_1 << 32));

  const __m256i mask_space = _mm256_set1_epi8(0x20);
  __m256i space_lo = _mm256_cmpeq_epi8(in.lo, mask_space);
  __m256i space_hi = _mm256_cmpeq_epi8(in.hi, mask_space);
  const __m256i mask_linefeed = _mm256_set1_epi8(0x0a);
  space_lo = _mm256_or_si256(space_lo, _mm256_cmpeq_epi8(in.lo, mask_linefeed));
  space_hi = _mm256_or_si256(space_hi, _mm256_cmpeq_epi8(in.hi, mask_linefeed));
  const __m256i mask_tab = _mm256_set1_epi8(0x09);
  space_lo = _mm256_or_si256(space_lo, _mm256_cmpeq_epi8(in.lo, mask_tab));
  space_hi = _mm256_or_si256(space_hi, _mm256_cmpeq_epi8(in.hi, mask_tab));
  const __m256i mask_carriage = _mm256_set1_epi8(0x0d);
  space_lo = _mm256_or_si256(space_lo, _mm256_cmpeq_epi8(in.lo, mask_carriage));
  space_hi = _mm256_or_si256(space_hi, _mm256_cmpeq_epi8(in.hi, mask_carriage));

  uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(space_lo));
  uint64_t ws_res_1 = _mm256_movemask_epi8(space_hi);
  whitespace = (ws_res_0 | (ws_res_1 << 32));
  // end of naive approach

  #else  // SIMDJSON_NAIVE_STRUCTURAL
  // clang-format off
  const __m256i structural_table =
      _mm256_setr_epi8(44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123,
                        44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
  const __m256i white_table = _mm256_setr_epi8(
      32, 100, 100, 100, 17, 100, 113, 2, 100, 9, 10, 112, 100, 13, 100, 100,
      32, 100, 100, 100, 17, 100, 113, 2, 100, 9, 10, 112, 100, 13, 100, 100);
  // clang-format on
  const __m256i struct_offset = _mm256_set1_epi8(0xd4u);
  const __m256i struct_mask = _mm256_set1_epi8(32);

  __m256i lo_white = _mm256_cmpeq_epi8(in.lo, _mm256_shuffle_epi8(white_table, in.lo));
  __m256i hi_white = _mm256_cmpeq_epi8(in.hi, _mm256_shuffle_epi8(white_table, in.hi));
  uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(lo_white));
  uint64_t ws_res_1 = _mm256_movemask_epi8(hi_white);
  whitespace = (ws_res_0 | (ws_res_1 << 32));
  __m256i lo_struct_r1 = _mm256_add_epi8(struct_offset, in.lo);
  __m256i hi_struct_r1 = _mm256_add_epi8(struct_offset, in.hi);
  __m256i lo_struct_r2 = _mm256_or_si256(in.lo, struct_mask);
  __m256i hi_struct_r2 = _mm256_or_si256(in.hi, struct_mask);
  __m256i lo_struct_r3 = _mm256_shuffle_epi8(structural_table, lo_struct_r1);
  __m256i hi_struct_r3 = _mm256_shuffle_epi8(structural_table, hi_struct_r1);
  __m256i lo_struct = _mm256_cmpeq_epi8(lo_struct_r2, lo_struct_r3);
  __m256i hi_struct = _mm256_cmpeq_epi8(hi_struct_r2, hi_struct_r3);

  uint64_t structural_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(lo_struct));
  uint64_t structural_res_1 = _mm256_movemask_epi8(hi_struct);
  structurals = (structural_res_0 | (structural_res_1 << 32));
  #endif // else SIMDJSON_NAIVE_STRUCTURAL
}

// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
static really_inline void flatten_bits(uint32_t *base_ptr, uint32_t &base, uint32_t idx, uint64_t bits) {
  // In some instances, the next branch is expensive because it is mispredicted.
  // Unfortunately, in other cases,
  // it helps tremendously.
  if (bits == 0)
      return;
  uint32_t cnt = _mm_popcnt_u64(bits);
  uint32_t next_base = base + cnt;
  idx -= 64;
  base_ptr += base;
  {
      base_ptr[0] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[1] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[2] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[3] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[4] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[5] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[6] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[7] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr += 8;
  }
  // We hope that the next branch is easily predicted.
  if (cnt > 8) {
      base_ptr[0] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[1] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[2] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[3] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[4] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[5] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[6] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr[7] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr += 8;
  }
  if (cnt > 16) { // unluckly: we rarely get here
      // since it means having one structural or pseudo-structral element
      // every 4 characters (possible with inputs like "","","",...).
      do {
      base_ptr[0] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
      base_ptr++;
      } while (bits != 0);
  }
  base = next_base;
}

#include "generic/stage1_find_marks.h"

} // namespace haswell
UNTARGET_REGION

TARGET_HASWELL
namespace simdjson {

template <>
int find_structural_bits<Architecture::HASWELL>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  return haswell::find_structural_bits(buf, len, pj);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H
