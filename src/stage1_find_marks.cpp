#include <cassert>
#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"
#include "simdjson/portability.h"


#ifdef __AVX2__

#ifndef SIMDJSON_SKIPUTF8VALIDATION
#define SIMDJSON_UTF8VALIDATE

#endif
#else
// currently we don't UTF8 validate for ARM
// also we assume that if you're not __AVX2__ 
// you're ARM, which is a bit dumb. TODO: Fix...
#ifdef __ARM_NEON
#include <arm_neon.h>
#else
#warning It appears that neither ARM NEON nor AVX2 are detected.
#endif // __ARM_NEON
#endif // __AVX2__

// It seems that many parsers do UTF-8 validation.
// RapidJSON does not do it by default, but a flag
// allows it.
#ifdef SIMDJSON_UTF8VALIDATE
#include "simdjson/simdutf8check.h"
#endif

#define TRANSPOSE

struct simd_input {
#ifdef __AVX2__
  __m256i lo;
  __m256i hi;
#elif defined(__ARM_NEON)
#ifndef TRANSPOSE
  uint8x16_t i0;
  uint8x16_t i1;
  uint8x16_t i2;
  uint8x16_t i3;
#else
  uint8x16x4_t i;
#endif
#else
#warning It appears that neither ARM NEON nor AVX2 are detected.
#endif
};

really_inline simd_input fill_input(const uint8_t * ptr) {
  struct simd_input in;
#ifdef __AVX2__
  in.lo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 0));
  in.hi = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 32));
#elif defined(__ARM_NEON)
#ifndef TRANSPOSE
  in.i0 = vld1q_u8(ptr + 0);
  in.i1 = vld1q_u8(ptr + 16);
  in.i2 = vld1q_u8(ptr + 32);
  in.i3 = vld1q_u8(ptr + 48);
#else
  in.i = vld4q_u8(ptr);
#endif
#else
#warning It appears that neither ARM NEON nor AVX2 are detected.
#endif
  return in;
}

#ifdef SIMDJSON_UTF8VALIDATE
really_inline void check_utf8(simd_input in,
                              __m256i &has_error,
                              struct avx_processed_utf_bytes &previous) {
  __m256i highbit = _mm256_set1_epi8(0x80);
  if ((_mm256_testz_si256(_mm256_or_si256(in.lo, in.hi), highbit)) == 1) {
    // it is ascii, we just check continuation
    has_error = _mm256_or_si256(
        _mm256_cmpgt_epi8(
            previous.carried_continuations,
            _mm256_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                             9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1)),
        has_error);
  } else {
    // it is not ascii so we have to do heavy work
    previous = avxcheckUTF8Bytes(in.lo, &previous, &has_error);
    previous = avxcheckUTF8Bytes(in.hi, &previous, &has_error);
  }
}
#endif

#ifdef __ARM_NEON
uint16_t neonmovemask(uint8x16_t input) {
  const uint8x16_t bitmask = { 0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                               0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  uint8x16_t minput = vandq_u8(input, bitmask);
  uint8x16_t tmp = vpaddq_u8(minput, minput);
  tmp = vpaddq_u8(tmp, tmp);
  tmp = vpaddq_u8(tmp, tmp);
  return vgetq_lane_u16(vreinterpretq_u16_u8(tmp), 0);
}

really_inline
uint64_t neonmovemask_bulk(uint8x16_t p0, uint8x16_t p1, uint8x16_t p2, uint8x16_t p3) {
#ifndef TRANSPOSE
  const uint8x16_t bitmask = { 0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                               0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  uint8x16_t t0 = vandq_u8(p0, bitmask);
  uint8x16_t t1 = vandq_u8(p1, bitmask);
  uint8x16_t t2 = vandq_u8(p2, bitmask);
  uint8x16_t t3 = vandq_u8(p3, bitmask);
  uint8x16_t sum0 = vpaddq_u8(t0, t1);
  uint8x16_t sum1 = vpaddq_u8(t2, t3);
  sum0 = vpaddq_u8(sum0, sum1);
  sum0 = vpaddq_u8(sum0, sum0);
  return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
#else
  const uint8x16_t bitmask1 = { 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10,
                                0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10};
  const uint8x16_t bitmask2 = { 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20,
                                0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20};
  const uint8x16_t bitmask3 = { 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x04, 0x40,
                                0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x04, 0x40};
  const uint8x16_t bitmask4 = { 0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80,
                                0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80};
#if 0
  uint8x16_t t0 = vandq_u8(p0, bitmask1);
  uint8x16_t t1 = vandq_u8(p1, bitmask2);
  uint8x16_t t2 = vandq_u8(p2, bitmask3);
  uint8x16_t t3 = vandq_u8(p3, bitmask4);
  uint8x16_t tmp = vorrq_u8(vorrq_u8(t0, t1), vorrq_u8(t2, t3));
#else
  uint8x16_t t0 = vandq_u8(p0, bitmask1);
  uint8x16_t t1 = vbslq_u8(bitmask2, p1, t0);
  uint8x16_t t2 = vbslq_u8(bitmask3, p2, t1);
  uint8x16_t tmp = vbslq_u8(bitmask4, p3, t2);
#endif
  uint8x16_t sum = vpaddq_u8(tmp, tmp);
  return vgetq_lane_u64(vreinterpretq_u64_u8(sum), 0);
#endif
}
#endif

// a straightforward comparison of a mask against input. 5 uops; would be
// cheaper in AVX512.
really_inline uint64_t cmp_mask_against_input(simd_input in, uint8_t m) {
#ifdef __AVX2__
  const __m256i mask = _mm256_set1_epi8(m);
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(in.lo, mask);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(in.hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
#elif defined(__ARM_NEON)
  const uint8x16_t mask = vmovq_n_u8(m); 
  uint8x16_t cmp_res_0 = vceqq_u8(in.i.val[0], mask); 
  uint8x16_t cmp_res_1 = vceqq_u8(in.i.val[1], mask); 
  uint8x16_t cmp_res_2 = vceqq_u8(in.i.val[2], mask); 
  uint8x16_t cmp_res_3 = vceqq_u8(in.i.val[3], mask); 
  return neonmovemask_bulk(cmp_res_0, cmp_res_1, cmp_res_2, cmp_res_3);
#else
#warning It appears that neither ARM NEON nor AVX2 are detected.
#endif
}

// find all values less than or equal than the content of maxval (using unsigned arithmetic) 
really_inline uint64_t unsigned_lteq_against_input(simd_input in, uint8_t m) {
#ifdef __AVX2__
  const __m256i maxval = _mm256_set1_epi8(m);
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(_mm256_max_epu8(maxval,in.lo),maxval);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(_mm256_max_epu8(maxval,in.hi),maxval);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
#elif defined(__ARM_NEON)
  const uint8x16_t mask = vmovq_n_u8(m); 
  uint8x16_t cmp_res_0 = vcleq_u8(in.i.val[0], mask); 
  uint8x16_t cmp_res_1 = vcleq_u8(in.i.val[1], mask); 
  uint8x16_t cmp_res_2 = vcleq_u8(in.i.val[2], mask); 
  uint8x16_t cmp_res_3 = vcleq_u8(in.i.val[3], mask); 
  return neonmovemask_bulk(cmp_res_0, cmp_res_1, cmp_res_2, cmp_res_3);
#else
#warning It appears that neither ARM NEON nor AVX2 are detected.
#endif
}

// return a bitvector indicating where we have characters that end an odd-length
// sequence of backslashes (and thus change the behavior of the next character
// to follow). A even-length sequence of backslashes, and, for that matter, the
// largest even-length prefix of our odd-length sequence of backslashes, simply
// modify the behavior of the backslashes themselves.
// We also update the prev_iter_ends_odd_backslash reference parameter to
// indicate whether we end an iteration on an odd-length sequence of
// backslashes, which modifies our subsequent search for odd-length
// sequences of backslashes in an obvious way.
really_inline uint64_t
find_odd_backslash_sequences(simd_input in,
                             uint64_t &prev_iter_ends_odd_backslash) {
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint64_t bs_bits = cmp_mask_against_input(in, '\\');
  uint64_t start_edges = bs_bits & ~(bs_bits << 1);
  // flip lowest if we have an odd-length run at the end of the prior
  // iteration
  uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
  uint64_t even_starts = start_edges & even_start_mask;
  uint64_t odd_starts = start_edges & ~even_start_mask;
  uint64_t even_carries = bs_bits + even_starts;

  uint64_t odd_carries;
  // must record the carry-out of our odd-carries out of bit 63; this
  // indicates whether the sense of any edge going to the next iteration
  // should be flipped
  bool iter_ends_odd_backslash =
      add_overflow(bs_bits, odd_starts, &odd_carries);

  odd_carries |=
      prev_iter_ends_odd_backslash;  // push in bit zero as a potential end
                                     // if we had an odd-numbered run at the
                                     // end of the previous iteration
  prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
  uint64_t even_carry_ends = even_carries & ~bs_bits;
  uint64_t odd_carry_ends = odd_carries & ~bs_bits;
  uint64_t even_start_odd_end = even_carry_ends & odd_bits;
  uint64_t odd_start_even_end = odd_carry_ends & even_bits;
  uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
  return odd_ends;
}

// return both the quote mask (which is a half-open mask that covers the first
// quote
// in an unescaped quote pair and everything in the quote pair) and the quote
// bits, which are the simple
// unescaped quoted bits. We also update the prev_iter_inside_quote value to
// tell the next iteration
// whether we finished the final iteration inside a quote pair; if so, this
// inverts our behavior of
// whether we're inside quotes for the next iteration.
// Note that we don't do any error checking to see if we have backslash
// sequences outside quotes; these
// backslash sequences (of any length) will be detected elsewhere.
really_inline uint64_t find_quote_mask_and_bits(simd_input in, uint64_t odd_ends,
    uint64_t &prev_iter_inside_quote, uint64_t &quote_bits, uint64_t &error_mask) {
  quote_bits = cmp_mask_against_input(in, '"');
  quote_bits = quote_bits & ~odd_ends;
  // remove from the valid quoted region the unescapted characters.
#ifdef __AVX2__
  uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
#elif defined(__ARM_NEON)
  uint64_t quote_mask = vmull_p64( -1ULL, quote_bits);
#else
#warning It appears that neither ARM NEON nor AVX2 are detected.
#endif
  quote_mask ^= prev_iter_inside_quote;
  // All Unicode characters may be placed within the
  // quotation marks, except for the characters that MUST be escaped:
  // quotation mark, reverse solidus, and the control characters (U+0000
  //through U+001F).
  // https://tools.ietf.org/html/rfc8259
  uint64_t unescaped = unsigned_lteq_against_input(in, 0x1F);
  error_mask |= quote_mask & unescaped;
  // right shift of a signed value expected to be well-defined and standard
  // compliant as of C++20,
  // John Regher from Utah U. says this is fine code
  prev_iter_inside_quote =
      static_cast<uint64_t>(static_cast<int64_t>(quote_mask) >> 63);
  return quote_mask;
}

really_inline void find_whitespace_and_structurals(simd_input in,
                                                   uint64_t &whitespace,
                                                   uint64_t &structurals) {
  // do a 'shufti' to detect structural JSON characters
  // they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
  // these go into the first 3 buckets of the comparison (1/2/4)

  // we are also interested in the four whitespace characters
  // space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d
  // these go into the next 2 buckets of the comparison (8/16)
#ifdef __AVX2__
  const __m256i low_nibble_mask = _mm256_setr_epi8(
      16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0, 
      16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
  const __m256i high_nibble_mask = _mm256_setr_epi8(
      8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0, 
      8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0);

  __m256i structural_shufti_mask = _mm256_set1_epi8(0x7);
  __m256i whitespace_shufti_mask = _mm256_set1_epi8(0x18);

  __m256i v_lo = _mm256_and_si256(
      _mm256_shuffle_epi8(low_nibble_mask, in.lo),
      _mm256_shuffle_epi8(high_nibble_mask,
                          _mm256_and_si256(_mm256_srli_epi32(in.lo, 4),
                                           _mm256_set1_epi8(0x7f))));

  __m256i v_hi = _mm256_and_si256(
      _mm256_shuffle_epi8(low_nibble_mask, in.hi),
      _mm256_shuffle_epi8(high_nibble_mask,
                          _mm256_and_si256(_mm256_srli_epi32(in.hi, 4),
                                           _mm256_set1_epi8(0x7f))));
  __m256i tmp_lo = _mm256_cmpeq_epi8(
      _mm256_and_si256(v_lo, structural_shufti_mask), _mm256_set1_epi8(0));
  __m256i tmp_hi = _mm256_cmpeq_epi8(
      _mm256_and_si256(v_hi, structural_shufti_mask), _mm256_set1_epi8(0));

  uint64_t structural_res_0 =
      static_cast<uint32_t>(_mm256_movemask_epi8(tmp_lo));
  uint64_t structural_res_1 = _mm256_movemask_epi8(tmp_hi);
  structurals = ~(structural_res_0 | (structural_res_1 << 32));

  __m256i tmp_ws_lo = _mm256_cmpeq_epi8(
      _mm256_and_si256(v_lo, whitespace_shufti_mask), _mm256_set1_epi8(0));
  __m256i tmp_ws_hi = _mm256_cmpeq_epi8(
      _mm256_and_si256(v_hi, whitespace_shufti_mask), _mm256_set1_epi8(0));

  uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(tmp_ws_lo));
  uint64_t ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
  whitespace = ~(ws_res_0 | (ws_res_1 << 32));
#elif defined(__ARM_NEON)
#ifndef FUNKY_BAD_TABLE
  const uint8x16_t low_nibble_mask = (uint8x16_t){ 
      16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0};
  const uint8x16_t high_nibble_mask = (uint8x16_t){ 
      8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0};
  const uint8x16_t structural_shufti_mask = vmovq_n_u8(0x7); 
  const uint8x16_t whitespace_shufti_mask = vmovq_n_u8(0x18); 
  const uint8x16_t low_nib_and_mask = vmovq_n_u8(0xf); 

  uint8x16_t nib_0_lo = vandq_u8(in.i.val[0], low_nib_and_mask);
  uint8x16_t nib_0_hi = vshrq_n_u8(in.i.val[0], 4);
  uint8x16_t shuf_0_lo = vqtbl1q_u8(low_nibble_mask, nib_0_lo);
  uint8x16_t shuf_0_hi = vqtbl1q_u8(high_nibble_mask, nib_0_hi);
  uint8x16_t v_0 = vandq_u8(shuf_0_lo, shuf_0_hi);

  uint8x16_t nib_1_lo = vandq_u8(in.i.val[1], low_nib_and_mask);
  uint8x16_t nib_1_hi = vshrq_n_u8(in.i.val[1], 4);
  uint8x16_t shuf_1_lo = vqtbl1q_u8(low_nibble_mask, nib_1_lo);
  uint8x16_t shuf_1_hi = vqtbl1q_u8(high_nibble_mask, nib_1_hi);
  uint8x16_t v_1 = vandq_u8(shuf_1_lo, shuf_1_hi);

  uint8x16_t nib_2_lo = vandq_u8(in.i.val[2], low_nib_and_mask);
  uint8x16_t nib_2_hi = vshrq_n_u8(in.i.val[2], 4);
  uint8x16_t shuf_2_lo = vqtbl1q_u8(low_nibble_mask, nib_2_lo);
  uint8x16_t shuf_2_hi = vqtbl1q_u8(high_nibble_mask, nib_2_hi);
  uint8x16_t v_2 = vandq_u8(shuf_2_lo, shuf_2_hi);

  uint8x16_t nib_3_lo = vandq_u8(in.i.val[3], low_nib_and_mask);
  uint8x16_t nib_3_hi = vshrq_n_u8(in.i.val[3], 4);
  uint8x16_t shuf_3_lo = vqtbl1q_u8(low_nibble_mask, nib_3_lo);
  uint8x16_t shuf_3_hi = vqtbl1q_u8(high_nibble_mask, nib_3_hi);
  uint8x16_t v_3 = vandq_u8(shuf_3_lo, shuf_3_hi);

  uint8x16_t tmp_0 = vtstq_u8(v_0, structural_shufti_mask);
  uint8x16_t tmp_1 = vtstq_u8(v_1, structural_shufti_mask);
  uint8x16_t tmp_2 = vtstq_u8(v_2, structural_shufti_mask);
  uint8x16_t tmp_3 = vtstq_u8(v_3, structural_shufti_mask);
  structurals = neonmovemask_bulk(tmp_0, tmp_1, tmp_2, tmp_3);

  uint8x16_t tmp_ws_0 = vtstq_u8(v_0, whitespace_shufti_mask);
  uint8x16_t tmp_ws_1 = vtstq_u8(v_1, whitespace_shufti_mask);
  uint8x16_t tmp_ws_2 = vtstq_u8(v_2, whitespace_shufti_mask);
  uint8x16_t tmp_ws_3 = vtstq_u8(v_3, whitespace_shufti_mask);
  whitespace = neonmovemask_bulk(tmp_ws_0, tmp_ws_1, tmp_ws_2, tmp_ws_3);
#else
  // I think this one is garbage. In order to save the expense
  // of another shuffle, I use an equally expensive shift, and 
  // this gets glued to the end of the dependency chain. Seems a bit
  // slower for no good reason.
  //
  // need to use a weird arrangement. Bytes in this bitvector
  // are in conventional order, but bits are reversed as we are
  // using a signed left shift (that is a +ve value from 0..7) to
  // shift upwards to 0x80 in the bit. So we need to reverse bits.
  
  // note no structural/whitespace has the high bit on
  // so it's OK to put the high 5 bits into our TBL shuffle
  //

  // structurals are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
  // or in 5 bit, 3 bit form thats
  // (15,3) (15, 5) (7,2) (11,3) (11,5) (5,4) 
  // bit-reversing (subtract low 3 bits from 7) yields:
  // (15,4) (15, 2) (7,5) (11,4) (11,2) (5,3) 
  
  const uint8x16_t structural_bitvec = (uint8x16_t){ 
      0, 0, 0, 0, 
      0, 8, 0, 32, 
      0, 0, 0, 20, 
      0, 0, 0, 20};
  // we are also interested in the four whitespace characters
  // space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d
  // (4,0) (1, 2) (1, 1) (1, 5)
  // bit-reversing (subtract low 3 bits from 7) yields:
  // (4,7) (1, 5) (1, 6) (1, 2)
  
  const uint8x16_t whitespace_bitvec = (uint8x16_t){ 
      0, 100, 0, 0, 
      128, 0, 0, 0, 
      0, 0, 0, 0, 
      0, 0, 0, 0};
  const uint8x16_t low_3bits_and_mask = vmovq_n_u8(0x7); 
  const uint8x16_t high_1bit_tst_mask = vmovq_n_u8(0x80); 

  int8x16_t low_3bits_0 = vreinterpretq_s8_u8(vandq_u8(in.i.val[0], low_3bits_and_mask));
  uint8x16_t high_5bits_0 = vshrq_n_u8(in.i.val[0], 3);
  uint8x16_t shuffle_structural_0 = vshlq_u8(vqtbl1q_u8(structural_bitvec, high_5bits_0), low_3bits_0);
  uint8x16_t shuffle_ws_0 = vshlq_u8(vqtbl1q_u8(whitespace_bitvec, high_5bits_0), low_3bits_0);
  uint8x16_t tmp_0 = vtstq_u8(shuffle_structural_0, high_1bit_tst_mask);
  uint8x16_t tmp_ws_0 = vtstq_u8(shuffle_ws_0, high_1bit_tst_mask);

  int8x16_t low_3bits_1 = vreinterpretq_s8_u8(vandq_u8(in.i.val[1], low_3bits_and_mask));
  uint8x16_t high_5bits_1 = vshrq_n_u8(in.i.val[1], 3);
  uint8x16_t shuffle_structural_1 = vshlq_u8(vqtbl1q_u8(structural_bitvec, high_5bits_1), low_3bits_1);
  uint8x16_t shuffle_ws_1 = vshlq_u8(vqtbl1q_u8(whitespace_bitvec, high_5bits_1), low_3bits_1);
  uint8x16_t tmp_1 = vtstq_u8(shuffle_structural_1, high_1bit_tst_mask);
  uint8x16_t tmp_ws_1 = vtstq_u8(shuffle_ws_1, high_1bit_tst_mask);

  int8x16_t low_3bits_2 = vreinterpretq_s8_u8(vandq_u8(in.i.val[2], low_3bits_and_mask));
  uint8x16_t high_5bits_2 = vshrq_n_u8(in.i.val[2], 3);
  uint8x16_t shuffle_structural_2 = vshlq_u8(vqtbl1q_u8(structural_bitvec, high_5bits_2), low_3bits_2);
  uint8x16_t shuffle_ws_2 = vshlq_u8(vqtbl1q_u8(whitespace_bitvec, high_5bits_2), low_3bits_2);
  uint8x16_t tmp_2 = vtstq_u8(shuffle_structural_2, high_1bit_tst_mask);
  uint8x16_t tmp_ws_2 = vtstq_u8(shuffle_ws_2, high_1bit_tst_mask);

  int8x16_t low_3bits_3 = vreinterpretq_s8_u8(vandq_u8(in.i.val[3], low_3bits_and_mask));
  uint8x16_t high_5bits_3 = vshrq_n_u8(in.i.val[3], 3);
  uint8x16_t shuffle_structural_3 = vshlq_u8(vqtbl1q_u8(structural_bitvec, high_5bits_3), low_3bits_3);
  uint8x16_t shuffle_ws_3 = vshlq_u8(vqtbl1q_u8(whitespace_bitvec, high_5bits_3), low_3bits_3);
  uint8x16_t tmp_3 = vtstq_u8(shuffle_structural_3, high_1bit_tst_mask);
  uint8x16_t tmp_ws_3 = vtstq_u8(shuffle_ws_3, high_1bit_tst_mask);

  structurals = neonmovemask_bulk(tmp_0, tmp_1, tmp_2, tmp_3);
  whitespace = neonmovemask_bulk(tmp_ws_0, tmp_ws_1, tmp_ws_2, tmp_ws_3);
#endif
#else 
#warning It appears that neither ARM NEON nor AVX2 are detected.
#endif
}

// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
really_inline void flatten_bits(uint32_t *base_ptr, uint32_t &base,
                                uint32_t idx, uint64_t bits) {
  // In some instances, the next branch is expensive because it is mispredicted. 
  // Unfortunately, in other cases,
  // it helps tremendously.
  if(bits == 0) return; 
  uint32_t cnt = hamming(bits);
  uint32_t next_base = base + cnt;
  idx -= 64;
  base_ptr += base;
  { 
    base_ptr[0] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[1] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[2] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[3] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[4] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[5] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[6] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[7] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr += 8;
  }
  // We hope that the next branch is easily predicted.
  if (cnt > 8) {
    base_ptr[0] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[1] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[2] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[3] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[4] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[5] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[6] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[7] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr += 8;
  }
  if (cnt > 16) { // unluckly: we rarely get here
    // since it means having one structural or pseudo-structral element 
    // every 4 characters (possible with inputs like "","","",...).
    do {
      base_ptr[0] = idx + trailingzeroes(bits);
      bits = bits & (bits - 1);
      base_ptr++;
    } while(bits != 0);
  }
  base = next_base;
}

// return a updated structural bit vector with quoted contents cleared out and
// pseudo-structural characters added to the mask
// updates prev_iter_ends_pseudo_pred which tells us whether the previous
// iteration ended on a whitespace or a structural character (which means that
// the next iteration
// will have a pseudo-structural character at its start)
really_inline uint64_t finalize_structurals(
    uint64_t structurals, uint64_t whitespace, uint64_t quote_mask,
    uint64_t quote_bits, uint64_t &prev_iter_ends_pseudo_pred) {
  // mask off anything inside quotes
  structurals &= ~quote_mask;
  // add the real quote bits back into our bitmask as well, so we can
  // quickly traverse the strings we've spent all this trouble gathering
  structurals |= quote_bits;
  // Now, establish "pseudo-structural characters". These are non-whitespace
  // characters that are (a) outside quotes and (b) have a predecessor that's
  // either whitespace or a structural character. This means that subsequent
  // passes will get a chance to encounter the first character of every string
  // of non-whitespace and, if we're parsing an atom like true/false/null or a
  // number we can stop at the first whitespace or structural character
  // following it.

  // a qualified predecessor is something that can happen 1 position before an
  // psuedo-structural character
  uint64_t pseudo_pred = structurals | whitespace;

  uint64_t shifted_pseudo_pred =
      (pseudo_pred << 1) | prev_iter_ends_pseudo_pred;
  prev_iter_ends_pseudo_pred = pseudo_pred >> 63;
  uint64_t pseudo_structurals =
      shifted_pseudo_pred & (~whitespace) & (~quote_mask);
  structurals |= pseudo_structurals;

  // now, we've used our close quotes all we need to. So let's switch them off
  // they will be off in the quote mask and on in quote bits.
  structurals &= ~(quote_bits & ~quote_mask);
  return structurals;
}

WARN_UNUSED
/*never_inline*/ bool find_structural_bits(const uint8_t *buf, size_t len,
                                           ParsedJson &pj) {
  if (len > pj.bytecapacity) {
    std::cerr << "Your ParsedJson object only supports documents up to "
         << pj.bytecapacity << " bytes but you are trying to process " << len
         << " bytes" << std::endl;
    return false;
  }
  uint32_t *base_ptr = pj.structural_indexes;
  uint32_t base = 0;
#ifdef SIMDJSON_UTF8VALIDATE
  __m256i has_error = _mm256_setzero_si256();
  struct avx_processed_utf_bytes previous {};
  previous.rawbytes = _mm256_setzero_si256();
  previous.high_nibbles = _mm256_setzero_si256();
  previous.carried_continuations = _mm256_setzero_si256();
#endif

  // we have padded the input out to 64 byte multiple with the remainder being
  // zeros

  // persistent state across loop
  // does the last iteration end with an odd-length sequence of backslashes? 
  // either 0 or 1, but a 64-bit value
  uint64_t prev_iter_ends_odd_backslash = 0ULL;
  // does the previous iteration end inside a double-quote pair?
  uint64_t prev_iter_inside_quote = 0ULL;  // either all zeros or all ones
  // does the previous iteration end on something that is a predecessor of a
  // pseudo-structural character - i.e. whitespace or a structural character
  // effectively the very first char is considered to follow "whitespace" for
  // the
  // purposes of pseudo-structural character detection so we initialize to 1
  uint64_t prev_iter_ends_pseudo_pred = 1ULL;

  // structurals are persistent state across loop as we flatten them on the
  // subsequent iteration into our array pointed to be base_ptr.
  // This is harmless on the first iteration as structurals==0
  // and is done for performance reasons; we can hide some of the latency of the
  // expensive carryless multiply in the previous step with this work
  uint64_t structurals = 0;

  size_t lenminus64 = len < 64 ? 0 : len - 64;
  size_t idx = 0;
  uint64_t error_mask = 0; // for unescaped characters within strings (ASCII code points < 0x20)

  for (; idx < lenminus64; idx += 64) {
#ifndef _MSC_VER
    __builtin_prefetch(buf + idx + 128);
#endif
    simd_input in = fill_input(buf+idx);
#ifdef SIMDJSON_UTF8VALIDATE
    check_utf8(in, has_error, previous);
#endif
    // detect odd sequences of backslashes
    uint64_t odd_ends = find_odd_backslash_sequences(
        in, prev_iter_ends_odd_backslash);

    // detect insides of quote pairs ("quote_mask") and also our quote_bits
    // themselves
    uint64_t quote_bits;
    uint64_t quote_mask = find_quote_mask_and_bits(
        in, odd_ends, prev_iter_inside_quote, quote_bits, error_mask);

    // take the previous iterations structural bits, not our current iteration,
    // and flatten
    flatten_bits(base_ptr, base, idx, structurals);

    uint64_t whitespace;
    find_whitespace_and_structurals(in, whitespace, structurals);

    // fixup structurals to reflect quotes and add pseudo-structural characters
    structurals = finalize_structurals(structurals, whitespace, quote_mask,
                                       quote_bits, prev_iter_ends_pseudo_pred);
  }

  ////////////////
  /// we use a giant copy-paste which is ugly.
  /// but otherwise the string needs to be properly padded or else we
  /// risk invalidating the UTF-8 checks.
  ////////////
  if (idx < len) {
    uint8_t tmpbuf[64];
    memset(tmpbuf, 0x20, 64);
    memcpy(tmpbuf, buf + idx, len - idx);
    simd_input in = fill_input(tmpbuf);
#ifdef SIMDJSON_UTF8VALIDATE
    check_utf8(in, has_error, previous);
#endif

    // detect odd sequences of backslashes
    uint64_t odd_ends = find_odd_backslash_sequences(
        in, prev_iter_ends_odd_backslash);

    // detect insides of quote pairs ("quote_mask") and also our quote_bits
    // themselves
    uint64_t quote_bits;
    uint64_t quote_mask = find_quote_mask_and_bits(
        in, odd_ends, prev_iter_inside_quote, quote_bits, error_mask);

    // take the previous iterations structural bits, not our current iteration,
    // and flatten
    flatten_bits(base_ptr, base, idx, structurals);

    uint64_t whitespace;
    find_whitespace_and_structurals(in, whitespace, structurals);

    // fixup structurals to reflect quotes and add pseudo-structural characters
    structurals = finalize_structurals(structurals, whitespace, quote_mask,
                                       quote_bits, prev_iter_ends_pseudo_pred);
    idx += 64;
  }

  // is last string quote closed?
  if (prev_iter_inside_quote) {
      return false;
  }

  // finally, flatten out the remaining structurals from the last iteration
  flatten_bits(base_ptr, base, idx, structurals);

  pj.n_structural_indexes = base;
  // a valid JSON file cannot have zero structural indexes - we should have
  // found something
  if (pj.n_structural_indexes == 0u) {
printf("wacky exit\n");
    return false;
  }
  if (base_ptr[pj.n_structural_indexes - 1] > len) {
    fprintf(stderr, "Internal bug\n");
    return false;
  }
  if (len != base_ptr[pj.n_structural_indexes - 1]) {
    // the string might not be NULL terminated, but we add a virtual NULL ending
    // character.
    base_ptr[pj.n_structural_indexes++] = len;
  }
  // make it safe to dereference one beyond this array
  base_ptr[pj.n_structural_indexes] = 0;  
  if (error_mask) {
printf("had error mask\n");
    return false;
  }
#ifdef SIMDJSON_UTF8VALIDATE
  return _mm256_testz_si256(has_error, has_error) != 0;
#else
  return true;
#endif
}

bool find_structural_bits(const char *buf, size_t len, ParsedJson &pj) {
  return find_structural_bits(reinterpret_cast<const uint8_t *>(buf), len, pj);
}
