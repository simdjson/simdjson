#include "simdjson/portability.h"
#include <cassert>
#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"

#ifndef SIMDJSON_SKIPUTF8VALIDATION
#define SIMDJSON_UTF8VALIDATE
#endif

// It seems that many parsers do UTF-8 validation.
// RapidJSON does not do it by default, but a flag
// allows it.
#ifdef SIMDJSON_UTF8VALIDATE
#include "simdjson/simdutf8check.h"
#endif
using namespace std;


// a straightforward comparison of a mask against input. 5 uops; would be
// cheaper in AVX512.
really_inline uint64_t cmp_mask_against_input(__m256i input_lo, __m256i input_hi,
                                         __m256i mask) {
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(input_lo, mask);
  uint64_t res_0 = (uint32_t)_mm256_movemask_epi8(cmp_res_0);
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(input_hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

WARN_UNUSED
/*never_inline*/ bool find_structural_bits(const uint8_t *buf, size_t len,
                                           ParsedJson &pj) {
  if (len > pj.bytecapacity) {
    cerr << "Your ParsedJson object only supports documents up to "<< pj.bytecapacity << " bytes but you are trying to process " <<  len  << " bytes\n";
    return false;
  }
  uint32_t *base_ptr = pj.structural_indexes;
  uint32_t base = 0;
#ifdef SIMDJSON_UTF8VALIDATE
  __m256i has_error = _mm256_setzero_si256();
  struct avx_processed_utf_bytes previous;
  previous.rawbytes = _mm256_setzero_si256();
  previous.high_nibbles = _mm256_setzero_si256();
  previous.carried_continuations = _mm256_setzero_si256();
 #endif

  // Useful constant masks
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;

  // for now, just work in 64-byte chunks
  // we have padded the input out to 64 byte multiple with the remainder being
  // zeros

  // persistent state across loop
  uint64_t prev_iter_ends_odd_backslash = 0ULL; // either 0 or 1, but a 64-bit value
  uint64_t prev_iter_inside_quote = 0ULL;       // either all zeros or all ones

  // effectively the very first char is considered to follow "whitespace" for the
  // purposes of psuedo-structural character detection
  uint64_t prev_iter_ends_pseudo_pred = 1ULL;
  size_t lenminus64 = len < 64 ? 0 : len - 64;
  size_t idx = 0;
  uint64_t structurals = 0;
  for (; idx < lenminus64; idx += 64) {
#ifndef _MSC_VER
    __builtin_prefetch(buf + idx + 128);
#endif
	__m256i input_lo = _mm256_loadu_si256((const __m256i *)(buf + idx + 0));
    __m256i input_hi = _mm256_loadu_si256((const __m256i *)(buf + idx + 32));
#ifdef SIMDJSON_UTF8VALIDATE
    __m256i highbit = _mm256_set1_epi8(0x80);
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

    uint64_t bs_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('\\'));
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
        prev_iter_ends_odd_backslash; // push in bit zero as a potential end
                                      // if we had an odd-numbered run at the
                                      // end of the previous iteration
    prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
    uint64_t even_carry_ends = even_carries & ~bs_bits;
    uint64_t odd_carry_ends = odd_carries & ~bs_bits;
    uint64_t even_start_odd_end = even_carry_ends & odd_bits;
    uint64_t odd_start_even_end = odd_carry_ends & even_bits;
    uint64_t odd_ends = even_start_odd_end | odd_start_even_end;

    ////////////////////////////////////////////////////////////////////////////////////////////
    //     Step 2: detect insides of quote pairs
    ////////////////////////////////////////////////////////////////////////////////////////////

    uint64_t quote_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('"'));
    quote_bits = quote_bits & ~odd_ends;
    uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
        _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));



    uint32_t cnt = hamming(structurals);
    uint32_t next_base = base + cnt;
    while (structurals) {
      base_ptr[base + 0] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 1] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 2] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 3] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 4] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 5] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 6] = (uint32_t)idx - 64 + trailingzeroes(structurals);                   
      structurals = structurals & (structurals - 1);
      base_ptr[base + 7] = (uint32_t)idx - 64 + trailingzeroes(structurals); 
      structurals = structurals & (structurals - 1);
      base += 8;
    }
    base = next_base;

    quote_mask ^= prev_iter_inside_quote;
    prev_iter_inside_quote = (uint64_t)((int64_t)quote_mask >> 63); // right shift of a signed value expected to be well-defined and standard compliant as of C++20, John Regher from Utah U. says this is fine code

    // How do we build up a user traversable data structure
    // first, do a 'shufti' to detect structural JSON characters
    // they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
    // these go into the first 3 buckets of the comparison (1/2/4)

    // we are also interested in the four whitespace characters
    // space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d
    // these go into the next 2 buckets of the comparison (8/16)
    const __m256i low_nibble_mask = _mm256_setr_epi8(
        //  0                           9  a   b  c  d
        16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0, 16, 0, 0, 0, 0, 0, 0,
        0, 0, 8, 12, 1, 2, 9, 0, 0);
    const __m256i high_nibble_mask = _mm256_setr_epi8(
        //  0     2   3     5     7
        8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0, 8, 0, 18, 4, 0, 1, 0,
        1, 0, 0, 0, 3, 2, 1, 0, 0);

    __m256i structural_shufti_mask = _mm256_set1_epi8(0x7);
    __m256i whitespace_shufti_mask = _mm256_set1_epi8(0x18);

    __m256i v_lo = _mm256_and_si256(
        _mm256_shuffle_epi8(low_nibble_mask, input_lo),
        _mm256_shuffle_epi8(high_nibble_mask,
                            _mm256_and_si256(_mm256_srli_epi32(input_lo, 4),
                                             _mm256_set1_epi8(0x7f))));

    __m256i v_hi = _mm256_and_si256(
        _mm256_shuffle_epi8(low_nibble_mask, input_hi),
        _mm256_shuffle_epi8(high_nibble_mask,
                            _mm256_and_si256(_mm256_srli_epi32(input_hi, 4),
                                             _mm256_set1_epi8(0x7f))));
    __m256i tmp_lo = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_lo, structural_shufti_mask), _mm256_set1_epi8(0));
    __m256i tmp_hi = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_hi, structural_shufti_mask), _mm256_set1_epi8(0));

    uint64_t structural_res_0 = (uint32_t)_mm256_movemask_epi8(tmp_lo);
    uint64_t structural_res_1 = _mm256_movemask_epi8(tmp_hi);
    structurals = ~(structural_res_0 | (structural_res_1 << 32));

    // this additional mask and transfer is non-trivially expensive,
    // unfortunately
    __m256i tmp_ws_lo = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_lo, whitespace_shufti_mask), _mm256_set1_epi8(0));
    __m256i tmp_ws_hi = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_hi, whitespace_shufti_mask), _mm256_set1_epi8(0));

    uint64_t ws_res_0 = (uint32_t)_mm256_movemask_epi8(tmp_ws_lo);
    uint64_t ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
    uint64_t whitespace = ~(ws_res_0 | (ws_res_1 << 32));
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
    uint64_t shifted_pseudo_pred = (pseudo_pred << 1) | prev_iter_ends_pseudo_pred;
    prev_iter_ends_pseudo_pred = pseudo_pred >> 63;
    uint64_t pseudo_structurals =
        shifted_pseudo_pred & (~whitespace) & (~quote_mask);
    structurals |= pseudo_structurals;

    // now, we've used our close quotes all we need to. So let's switch them off
    // they will be off in the quote mask and on in quote bits.
    structurals &= ~(quote_bits & ~quote_mask);

    //*(uint64_t *)(pj.structurals + idx / 8) = structurals;
  }

  ////////////////
  /// we use a giant copy-paste which is ugly.
  /// but otherwise the string needs to be properly padded or else we
  /// risk invalidating the UTF-8 checks.
  ////////////
  if (idx < len) {
    uint8_t tmpbuf[64];
    memset(tmpbuf,0x20,64);
    memcpy(tmpbuf,buf+idx,len - idx);
    __m256i input_lo = _mm256_loadu_si256((const __m256i *)(tmpbuf + 0));
    __m256i input_hi = _mm256_loadu_si256((const __m256i *)(tmpbuf + 32));
#ifdef SIMDJSON_UTF8VALIDATE
    __m256i highbit = _mm256_set1_epi8(0x80);
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

    uint64_t bs_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('\\'));
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
    //bool iter_ends_odd_backslash =
	add_overflow(bs_bits, odd_starts, &odd_carries);

    odd_carries |=
        prev_iter_ends_odd_backslash; // push in bit zero as a potential end
                                      // if we had an odd-numbered run at the
                                      // end of the previous iteration
    //prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
    uint64_t even_carry_ends = even_carries & ~bs_bits;
    uint64_t odd_carry_ends = odd_carries & ~bs_bits;
    uint64_t even_start_odd_end = even_carry_ends & odd_bits;
    uint64_t odd_start_even_end = odd_carry_ends & even_bits;
    uint64_t odd_ends = even_start_odd_end | odd_start_even_end;

    ////////////////////////////////////////////////////////////////////////////////////////////
    //     Step 2: detect insides of quote pairs
    ////////////////////////////////////////////////////////////////////////////////////////////

    uint64_t quote_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('"'));
    quote_bits = quote_bits & ~odd_ends;
    uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
        _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
    quote_mask ^= prev_iter_inside_quote;
    //prev_iter_inside_quote = (uint64_t)((int64_t)quote_mask >> 63); // right shift of a signed value expected to be well-defined and standard compliant as of C++20

    uint32_t cnt = hamming(structurals);
    uint32_t next_base = base + cnt;
    while (structurals) {
      base_ptr[base + 0] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 1] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 2] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 3] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 4] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 5] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 6] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 7] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base += 8;
    }
    base = next_base;
    // How do we build up a user traversable data structure
    // first, do a 'shufti' to detect structural JSON characters
    // they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
    // these go into the first 3 buckets of the comparison (1/2/4)

    // we are also interested in the four whitespace characters
    // space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d
    // these go into the next 2 buckets of the comparison (8/16)
    const __m256i low_nibble_mask = _mm256_setr_epi8(
        //  0                           9  a   b  c  d
        16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0, 16, 0, 0, 0, 0, 0, 0,
        0, 0, 8, 12, 1, 2, 9, 0, 0);
    const __m256i high_nibble_mask = _mm256_setr_epi8(
        //  0     2   3     5     7
        8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0, 8, 0, 18, 4, 0, 1, 0,
        1, 0, 0, 0, 3, 2, 1, 0, 0);

    __m256i structural_shufti_mask = _mm256_set1_epi8(0x7);
    __m256i whitespace_shufti_mask = _mm256_set1_epi8(0x18);

    __m256i v_lo = _mm256_and_si256(
        _mm256_shuffle_epi8(low_nibble_mask, input_lo),
        _mm256_shuffle_epi8(high_nibble_mask,
                            _mm256_and_si256(_mm256_srli_epi32(input_lo, 4),
                                             _mm256_set1_epi8(0x7f))));

    __m256i v_hi = _mm256_and_si256(
        _mm256_shuffle_epi8(low_nibble_mask, input_hi),
        _mm256_shuffle_epi8(high_nibble_mask,
                            _mm256_and_si256(_mm256_srli_epi32(input_hi, 4),
                                             _mm256_set1_epi8(0x7f))));
    __m256i tmp_lo = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_lo, structural_shufti_mask), _mm256_set1_epi8(0));
    __m256i tmp_hi = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_hi, structural_shufti_mask), _mm256_set1_epi8(0));

    uint64_t structural_res_0 = (uint32_t)_mm256_movemask_epi8(tmp_lo);
    uint64_t structural_res_1 = _mm256_movemask_epi8(tmp_hi);
    structurals = ~(structural_res_0 | (structural_res_1 << 32));

    // this additional mask and transfer is non-trivially expensive,
    // unfortunately
    __m256i tmp_ws_lo = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_lo, whitespace_shufti_mask), _mm256_set1_epi8(0));
    __m256i tmp_ws_hi = _mm256_cmpeq_epi8(
        _mm256_and_si256(v_hi, whitespace_shufti_mask), _mm256_set1_epi8(0));

    uint64_t ws_res_0 = (uint32_t)_mm256_movemask_epi8(tmp_ws_lo);
    uint64_t ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
    uint64_t whitespace = ~(ws_res_0 | (ws_res_1 << 32));


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
    uint64_t shifted_pseudo_pred = (pseudo_pred << 1) | prev_iter_ends_pseudo_pred;
    //prev_iter_ends_pseudo_pred = pseudo_pred >> 63;
    uint64_t pseudo_structurals =
        shifted_pseudo_pred & (~whitespace) & (~quote_mask);
    structurals |= pseudo_structurals;

    // now, we've used our close quotes all we need to. So let's switch them off
    // they will be off in the quote mask and on in quote bits.
    structurals &= ~(quote_bits & ~quote_mask);
    //*(uint64_t *)(pj.structurals + idx / 8) = structurals;
    idx += 64;
  }
    uint32_t cnt = hamming(structurals);
    uint32_t next_base = base + cnt;
    while (structurals) {
      base_ptr[base + 0] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 1] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 2] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 3] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 4] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 5] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 6] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base_ptr[base + 7] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          
      structurals = structurals & (structurals - 1);
      base += 8;
    }
    base = next_base;

  pj.n_structural_indexes = base;
  if(base_ptr[pj.n_structural_indexes-1] > len) {
    fprintf( stderr,"Internal bug\n");
    return false;
  }
  if(len != base_ptr[pj.n_structural_indexes-1]) {
    // the string might not be NULL terminated, but we add a virtual NULL ending character. 
    base_ptr[pj.n_structural_indexes++] = len;
  }
  base_ptr[pj.n_structural_indexes] = 0; // make it safe to dereference one beyond this array

#ifdef SIMDJSON_UTF8VALIDATE
  return _mm256_testz_si256(has_error, has_error);
#else
  return true;
#endif
}
