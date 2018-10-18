#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <immintrin.h>
#include <x86intrin.h>
#endif

#include <cassert>

#include "jsonparser/common_defs.h"
#include "jsonparser/simdjson_internal.h"

#define UTF8VALIDATE
// It seems that many parsers do UTF-8 validation.
// RapidJSON does not do it by default, but a flag
// allows it. It appears that sajson might do utf-8
// validation
#ifdef UTF8VALIDATE
#include "jsonparser/simdutf8check.h"
#endif
using namespace std;

// a straightforward comparison of a mask against input. 5 uops; would be
// cheaper in AVX512.
really_inline u64 cmp_mask_against_input(m256 input_lo, m256 input_hi,
                                         m256 mask) {
  m256 cmp_res_0 = _mm256_cmpeq_epi8(input_lo, mask);
  u64 res_0 = (u32)_mm256_movemask_epi8(cmp_res_0);
  m256 cmp_res_1 = _mm256_cmpeq_epi8(input_hi, mask);
  u64 res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

WARN_UNUSED
/*never_inline*/ bool find_structural_bits(const u8 *buf, size_t len,
                                           ParsedJson &pj) {
  if (len > 0xffffff) {
    cerr << "Currently only support JSON files < 16MB\n";
    return false;
  }
#ifdef UTF8VALIDATE
  __m256i has_error = _mm256_setzero_si256();
  struct avx_processed_utf_bytes previous = {
      .rawbytes = _mm256_setzero_si256(),
      .high_nibbles = _mm256_setzero_si256(),
      .carried_continuations = _mm256_setzero_si256()};
 #endif

  // Useful constant masks
  const u64 even_bits = 0x5555555555555555ULL;
  const u64 odd_bits = ~even_bits;

  // for now, just work in 64-byte chunks
  // we have padded the input out to 64 byte multiple with the remainder being
  // zeros

  // persistent state across loop
  u64 prev_iter_ends_odd_backslash = 0ULL; // either 0 or 1, but a 64-bit value
  u64 prev_iter_inside_quote = 0ULL;       // either all zeros or all ones

  // effectively the very first char is considered to follow "whitespace" for the
  // purposes of psuedo-structural character detection
  u64 prev_iter_ends_pseudo_pred = 1ULL;

  for (size_t idx = 0; idx < len; idx += 64) {
    __builtin_prefetch(buf + idx + 128);
#ifdef DEBUG
    cout << "Idx is " << idx << "\n";
    for (u32 j = 0; j < 64; j++) {
      char c = *(buf + idx + j);
      if (isprint(c)) {
        cout << c;
      } else {
        cout << '_';
      }
    }
    cout << "|  ... input\n";
#endif
    m256 input_lo = _mm256_load_si256((const m256 *)(buf + idx + 0));
    m256 input_hi = _mm256_load_si256((const m256 *)(buf + idx + 32));
#ifdef UTF8VALIDATE
    m256 highbit = _mm256_set1_epi8(0x80);
    if((_mm256_testz_si256(_mm256_or_si256(input_lo, input_hi),highbit)) == 1) {
        // it is ascii, we just check continuation
        has_error = _mm256_or_si256(
          _mm256_cmpgt_epi8(previous.carried_continuations,
                          _mm256_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                           9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                           9, 9, 9, 9, 9, 9, 9, 1)),has_error);
 
    } else {
        // it is not ascii so we have to do heavy work
        previous = avxcheckUTF8Bytes(input_lo, &previous, &has_error);
        previous = avxcheckUTF8Bytes(input_hi, &previous, &has_error);
    }
#endif
    ////////////////////////////////////////////////////////////////////////////////////////////
    //     Step 1: detect odd sequences of backslashes
    ////////////////////////////////////////////////////////////////////////////////////////////

    u64 bs_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('\\'));
    dumpbits(bs_bits, "backslash bits");
    u64 start_edges = bs_bits & ~(bs_bits << 1);
    dumpbits(start_edges, "start_edges");

    // flip lowest if we have an odd-length run at the end of the prior
    // iteration
    u64 even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
    u64 even_starts = start_edges & even_start_mask;
    u64 odd_starts = start_edges & ~even_start_mask;

    dumpbits(even_starts, "even_starts");
    dumpbits(odd_starts, "odd_starts");

    u64 even_carries = bs_bits + even_starts;

    u64 odd_carries;
    // must record the carry-out of our odd-carries out of bit 63; this
    // indicates whether the sense of any edge going to the next iteration
    // should be flipped
    bool iter_ends_odd_backslash =
        __builtin_uaddll_overflow(bs_bits, odd_starts, &odd_carries);

    odd_carries |=
        prev_iter_ends_odd_backslash; // push in bit zero as a potential end
                                      // if we had an odd-numbered run at the
                                      // end of the previous iteration
    prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;

    dumpbits(even_carries, "even_carries");
    dumpbits(odd_carries, "odd_carries");

    u64 even_carry_ends = even_carries & ~bs_bits;
    u64 odd_carry_ends = odd_carries & ~bs_bits;
    dumpbits(even_carry_ends, "even_carry_ends");
    dumpbits(odd_carry_ends, "odd_carry_ends");

    u64 even_start_odd_end = even_carry_ends & odd_bits;
    u64 odd_start_even_end = odd_carry_ends & even_bits;
    dumpbits(even_start_odd_end, "esoe");
    dumpbits(odd_start_even_end, "osee");

    u64 odd_ends = even_start_odd_end | odd_start_even_end;
    dumpbits(odd_ends, "odd_ends");

    ////////////////////////////////////////////////////////////////////////////////////////////
    //     Step 2: detect insides of quote pairs
    ////////////////////////////////////////////////////////////////////////////////////////////

    u64 quote_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('"'));
    quote_bits = quote_bits & ~odd_ends;
    dumpbits(quote_bits, "quote_bits");
    u64 quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
        _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
    quote_mask ^= prev_iter_inside_quote;
    prev_iter_inside_quote = (u64)((s64)quote_mask >> 63);
    dumpbits(quote_mask, "quote_mask");

    // How do we build up a user traversable data structure
    // first, do a 'shufti' to detect structural JSON characters
    // they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
    // these go into the first 3 buckets of the comparison (1/2/4)

    // we are also interested in the four whitespace characters
    // space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d
    // these go into the next 2 buckets of the comparison (8/16)
    const m256 low_nibble_mask = _mm256_setr_epi8(
        //  0                           9  a   b  c  d
        16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0, 16, 0, 0, 0, 0, 0, 0,
        0, 0, 8, 12, 1, 2, 9, 0, 0);
    const m256 high_nibble_mask = _mm256_setr_epi8(
        //  0     2   3     5     7
        8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0, 8, 0, 18, 4, 0, 1, 0,
        1, 0, 0, 0, 3, 2, 1, 0, 0);

    m256 structural_shufti_mask = _mm256_set1_epi8(0x7);
    m256 whitespace_shufti_mask = _mm256_set1_epi8(0x18);

    m256 v_lo = _mm256_and_si256(
        _mm256_shuffle_epi8(low_nibble_mask, input_lo),
        _mm256_shuffle_epi8(high_nibble_mask,
                            _mm256_and_si256(_mm256_srli_epi32(input_lo, 4),
                                             _mm256_set1_epi8(0x7f))));

    m256 v_hi = _mm256_and_si256(
        _mm256_shuffle_epi8(low_nibble_mask, input_hi),
        _mm256_shuffle_epi8(high_nibble_mask,
                            _mm256_and_si256(_mm256_srli_epi32(input_hi, 4),
                                             _mm256_set1_epi8(0x7f))));
    m256 tmp_lo = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_lo, structural_shufti_mask), _mm256_set1_epi8(0));
    m256 tmp_hi = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_hi, structural_shufti_mask), _mm256_set1_epi8(0));

    u64 structural_res_0 = (u32)_mm256_movemask_epi8(tmp_lo);
    u64 structural_res_1 = _mm256_movemask_epi8(tmp_hi);
    u64 structurals = ~(structural_res_0 | (structural_res_1 << 32));

    // this additional mask and transfer is non-trivially expensive,
    // unfortunately
    m256 tmp_ws_lo = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_lo, whitespace_shufti_mask), _mm256_set1_epi8(0));
    m256 tmp_ws_hi = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_hi, whitespace_shufti_mask), _mm256_set1_epi8(0));

    u64 ws_res_0 = (u32)_mm256_movemask_epi8(tmp_ws_lo);
    u64 ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
    u64 whitespace = ~(ws_res_0 | (ws_res_1 << 32));

    dumpbits(structurals, "structurals");
    dumpbits(whitespace, "whitespace");

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
    u64 pseudo_pred = structurals | whitespace;
    dumpbits(pseudo_pred, "pseudo_pred");
    u64 shifted_pseudo_pred = (pseudo_pred << 1) | prev_iter_ends_pseudo_pred;
    dumpbits(shifted_pseudo_pred, "shifted_pseudo_pred");
    prev_iter_ends_pseudo_pred = pseudo_pred >> 63;
    u64 pseudo_structurals =
        shifted_pseudo_pred & (~whitespace) & (~quote_mask);
    dumpbits(pseudo_structurals, "pseudo_structurals");
    dumpbits(structurals, "final structurals without pseudos");
    structurals |= pseudo_structurals;
    dumpbits(structurals, "final structurals and pseudo structurals");

    // now, we've used our close quotes all we need to. So let's switch them off
    // they will be off in the quote mask and on in quote bits.
    structurals &= ~(quote_bits & ~quote_mask);
    dumpbits(
        structurals,
        "final structurals and pseudo structurals after close quote removal");
    *(u64 *)(pj.structurals + idx / 8) = structurals;
  }
#ifdef UTF8VALIDATE
  return _mm256_testz_si256(has_error, has_error);
#else
  return true;
#endif
}
