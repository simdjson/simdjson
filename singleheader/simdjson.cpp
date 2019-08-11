/* auto-generated on Sun Aug  4 15:43:41 EDT 2019. Do not edit! */
#include "simdjson.h"

/* used for http://dmalloc.com/ Dmalloc - Debug Malloc Library */
#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* begin file src/simdjson.cpp */
#include <map>

namespace simdjson {
const std::map<int, const std::string> error_strings = {
    {SUCCESS, "No errors"},
    {CAPACITY, "This ParsedJson can't support a document that big"},
    {MEMALLOC, "Error allocating memory, we're most likely out of memory"},
    {TAPE_ERROR, "Something went wrong while writing to the tape"},
    {STRING_ERROR, "Problem while parsing a string"},
    {T_ATOM_ERROR,
     "Problem while parsing an atom starting with the letter 't'"},
    {F_ATOM_ERROR,
     "Problem while parsing an atom starting with the letter 'f'"},
    {N_ATOM_ERROR,
     "Problem while parsing an atom starting with the letter 'n'"},
    {NUMBER_ERROR, "Problem while parsing a number"},
    {UTF8_ERROR, "The input is not valid UTF-8"},
    {UNITIALIZED, "Unitialized"},
    {EMPTY, "Empty"},
    {UNESCAPED_CHARS, "Within strings, some characters must be escapted, we "
                      "found unescapted characters"},
    {UNEXPECTED_ERROR, "Unexpected error, consider reporting this problem as "
                       "you may have found a bug in simdjson"},
};

const std::string &error_message(const int error_code) {
  return error_strings.at(error_code);
}
} // namespace simdjson
/* end file src/simdjson.cpp */
/* begin file src/jsonioutil.cpp */
#include <cstdlib>
#include <cstring>

namespace simdjson {
char *allocate_padded_buffer(size_t length) {
  // we could do a simple malloc
  // return (char *) malloc(length + SIMDJSON_PADDING);
  // However, we might as well align to cache lines...
  size_t totalpaddedlength = length + SIMDJSON_PADDING;
  char *padded_buffer = aligned_malloc_char(64, totalpaddedlength);
  return padded_buffer;
}

padded_string get_corpus(const std::string &filename) {
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  if (fp != nullptr) {
    std::fseek(fp, 0, SEEK_END);
    size_t len = std::ftell(fp);
    padded_string s(len);
    if (s.data() == nullptr) {
      std::fclose(fp);
      throw std::runtime_error("could not allocate memory");
    }
    std::rewind(fp);
    size_t readb = std::fread(s.data(), 1, len, fp);
    std::fclose(fp);
    if (readb != len) {
      throw std::runtime_error("could not read the data");
    }
    return s;
  }
  throw std::runtime_error("could not load corpus");
}
} // namespace simdjson
/* end file src/jsonioutil.cpp */
/* begin file src/jsonminifier.cpp */
#include <cstdint>

#ifndef __AVX2__

namespace simdjson {
static uint8_t jump_table[256 * 3] = {
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
};

size_t json_minify(const unsigned char *bytes, size_t how_many,
                   unsigned char *out) {
  size_t i = 0, pos = 0;
  uint8_t quote = 0;
  uint8_t nonescape = 1;

  while (i < how_many) {
    unsigned char c = bytes[i];
    uint8_t *meta = jump_table + 3 * c;

    quote = quote ^ (meta[0] & nonescape);
    out[pos] = c;
    pos += meta[2] | quote;

    i += 1;
    nonescape = (~nonescape) | (meta[1]);
  }
  return pos;
}
} // namespace simdjson
#else
#include <cstring>

namespace simdjson {

// some intrinsics are missing under GCC?
#ifndef __clang__
#ifndef _MSC_VER
static __m256i inline _mm256_loadu2_m128i(__m128i const *__addr_hi,
                                          __m128i const *__addr_lo) {
  __m256i __v256 = _mm256_castsi128_si256(_mm_loadu_si128(__addr_lo));
  return _mm256_insertf128_si256(__v256, _mm_loadu_si128(__addr_hi), 1);
}

static inline void _mm256_storeu2_m128i(__m128i *__addr_hi, __m128i *__addr_lo,
                                        __m256i __a) {
  __m128i __v128;
  __v128 = _mm256_castsi256_si128(__a);
  _mm_storeu_si128(__addr_lo, __v128);
  __v128 = _mm256_extractf128_si256(__a, 1);
  _mm_storeu_si128(__addr_hi, __v128);
}
#endif
#endif

// a straightforward comparison of a mask against input.
static uint64_t cmp_mask_against_input_mini(__m256i input_lo, __m256i input_hi,
                                            __m256i mask) {
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(input_lo, mask);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(input_hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

// take input from buf and remove useless whitespace, input and output can be
// the same, result is null terminated, return the string length (minus the null
// termination)
size_t json_minify(const uint8_t *buf, size_t len, uint8_t *out) {
  // Useful constant masks
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint8_t *initout(out);
  uint64_t prev_iter_ends_odd_backslash =
      0ULL;                               // either 0 or 1, but a 64-bit value
  uint64_t prev_iter_inside_quote = 0ULL; // either all zeros or all ones
  size_t idx = 0;
  if (len >= 64) {
    size_t avx_len = len - 63;

    for (; idx < avx_len; idx += 64) {
      __m256i input_lo =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + idx + 0));
      __m256i input_hi =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + idx + 32));
      uint64_t bs_bits = cmp_mask_against_input_mini(input_lo, input_hi,
                                                     _mm256_set1_epi8('\\'));
      uint64_t start_edges = bs_bits & ~(bs_bits << 1);
      uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
      uint64_t even_starts = start_edges & even_start_mask;
      uint64_t odd_starts = start_edges & ~even_start_mask;
      uint64_t even_carries = bs_bits + even_starts;
      uint64_t odd_carries;
      bool iter_ends_odd_backslash =
          add_overflow(bs_bits, odd_starts, &odd_carries);
      odd_carries |= prev_iter_ends_odd_backslash;
      prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
      uint64_t even_carry_ends = even_carries & ~bs_bits;
      uint64_t odd_carry_ends = odd_carries & ~bs_bits;
      uint64_t even_start_odd_end = even_carry_ends & odd_bits;
      uint64_t odd_start_even_end = odd_carry_ends & even_bits;
      uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
      uint64_t quote_bits = cmp_mask_against_input_mini(input_lo, input_hi,
                                                        _mm256_set1_epi8('"'));
      quote_bits = quote_bits & ~odd_ends;
      uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
          _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
      quote_mask ^= prev_iter_inside_quote;
      prev_iter_inside_quote = static_cast<uint64_t>(
          static_cast<int64_t>(quote_mask) >>
          63); // might be undefined behavior, should be fully defined in C++20,
               // ok according to John Regher from Utah University
      const __m256i low_nibble_mask = _mm256_setr_epi8(
          //  0                           9  a   b  c  d
          16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0, 16, 0, 0, 0, 0, 0,
          0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
      const __m256i high_nibble_mask = _mm256_setr_epi8(
          //  0     2   3     5     7
          8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0, 8, 0, 18, 4, 0, 1, 0,
          1, 0, 0, 0, 3, 2, 1, 0, 0);
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
      __m256i tmp_ws_lo = _mm256_cmpeq_epi8(
          _mm256_and_si256(v_lo, whitespace_shufti_mask), _mm256_set1_epi8(0));
      __m256i tmp_ws_hi = _mm256_cmpeq_epi8(
          _mm256_and_si256(v_hi, whitespace_shufti_mask), _mm256_set1_epi8(0));

      uint64_t ws_res_0 =
          static_cast<uint32_t>(_mm256_movemask_epi8(tmp_ws_lo));
      uint64_t ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
      uint64_t whitespace = ~(ws_res_0 | (ws_res_1 << 32));
      whitespace &= ~quote_mask;
      int mask1 = whitespace & 0xFFFF;
      int mask2 = (whitespace >> 16) & 0xFFFF;
      int mask3 = (whitespace >> 32) & 0xFFFF;
      int mask4 = (whitespace >> 48) & 0xFFFF;
      int pop1 = hamming((~whitespace) & 0xFFFF);
      int pop2 = hamming((~whitespace) & UINT64_C(0xFFFFFFFF));
      int pop3 = hamming((~whitespace) & UINT64_C(0xFFFFFFFFFFFF));
      int pop4 = hamming((~whitespace));
      __m256i vmask1 = _mm256_loadu2_m128i(
          reinterpret_cast<const __m128i *>(mask128_epi8) + (mask2 & 0x7FFF),
          reinterpret_cast<const __m128i *>(mask128_epi8) + (mask1 & 0x7FFF));
      __m256i vmask2 = _mm256_loadu2_m128i(
          reinterpret_cast<const __m128i *>(mask128_epi8) + (mask4 & 0x7FFF),
          reinterpret_cast<const __m128i *>(mask128_epi8) + (mask3 & 0x7FFF));
      __m256i result1 = _mm256_shuffle_epi8(input_lo, vmask1);
      __m256i result2 = _mm256_shuffle_epi8(input_hi, vmask2);
      _mm256_storeu2_m128i(reinterpret_cast<__m128i *>(out + pop1),
                           reinterpret_cast<__m128i *>(out), result1);
      _mm256_storeu2_m128i(reinterpret_cast<__m128i *>(out + pop3),
                           reinterpret_cast<__m128i *>(out + pop2), result2);
      out += pop4;
    }
  }
  // we finish off the job... copying and pasting the code is not ideal here,
  // but it gets the job done.
  if (idx < len) {
    uint8_t buffer[64];
    memset(buffer, 0, 64);
    memcpy(buffer, buf + idx, len - idx);
    __m256i input_lo =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buffer));
    __m256i input_hi =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buffer + 32));
    uint64_t bs_bits =
        cmp_mask_against_input_mini(input_lo, input_hi, _mm256_set1_epi8('\\'));
    uint64_t start_edges = bs_bits & ~(bs_bits << 1);
    uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
    uint64_t even_starts = start_edges & even_start_mask;
    uint64_t odd_starts = start_edges & ~even_start_mask;
    uint64_t even_carries = bs_bits + even_starts;
    uint64_t odd_carries;
    // bool iter_ends_odd_backslash =
    add_overflow(bs_bits, odd_starts, &odd_carries);
    odd_carries |= prev_iter_ends_odd_backslash;
    // prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
    // // we never use it
    uint64_t even_carry_ends = even_carries & ~bs_bits;
    uint64_t odd_carry_ends = odd_carries & ~bs_bits;
    uint64_t even_start_odd_end = even_carry_ends & odd_bits;
    uint64_t odd_start_even_end = odd_carry_ends & even_bits;
    uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
    uint64_t quote_bits =
        cmp_mask_against_input_mini(input_lo, input_hi, _mm256_set1_epi8('"'));
    quote_bits = quote_bits & ~odd_ends;
    uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
        _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
    quote_mask ^= prev_iter_inside_quote;
    // prev_iter_inside_quote = (uint64_t)((int64_t)quote_mask >> 63);// we
    // don't need this anymore

    __m256i mask_20 = _mm256_set1_epi8(0x20); // c==32
    __m256i mask_70 =
        _mm256_set1_epi8(0x70); // adding 0x70 does not check low 4-bits
    // but moves any value >= 16 above 128

    __m256i lut_cntrl = _mm256_setr_epi8(
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00,
        0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00);

    __m256i tmp_ws_lo = _mm256_or_si256(
        _mm256_cmpeq_epi8(mask_20, input_lo),
        _mm256_shuffle_epi8(lut_cntrl, _mm256_adds_epu8(mask_70, input_lo)));
    __m256i tmp_ws_hi = _mm256_or_si256(
        _mm256_cmpeq_epi8(mask_20, input_hi),
        _mm256_shuffle_epi8(lut_cntrl, _mm256_adds_epu8(mask_70, input_hi)));
    uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(tmp_ws_lo));
    uint64_t ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
    uint64_t whitespace = (ws_res_0 | (ws_res_1 << 32));
    whitespace &= ~quote_mask;

    if (len - idx < 64) {
      whitespace |= UINT64_C(0xFFFFFFFFFFFFFFFF) << (len - idx);
    }
    int mask1 = whitespace & 0xFFFF;
    int mask2 = (whitespace >> 16) & 0xFFFF;
    int mask3 = (whitespace >> 32) & 0xFFFF;
    int mask4 = (whitespace >> 48) & 0xFFFF;
    int pop1 = hamming((~whitespace) & 0xFFFF);
    int pop2 = hamming((~whitespace) & UINT64_C(0xFFFFFFFF));
    int pop3 = hamming((~whitespace) & UINT64_C(0xFFFFFFFFFFFF));
    int pop4 = hamming((~whitespace));
    __m256i vmask1 = _mm256_loadu2_m128i(
        reinterpret_cast<const __m128i *>(mask128_epi8) + (mask2 & 0x7FFF),
        reinterpret_cast<const __m128i *>(mask128_epi8) + (mask1 & 0x7FFF));
    __m256i vmask2 = _mm256_loadu2_m128i(
        reinterpret_cast<const __m128i *>(mask128_epi8) + (mask4 & 0x7FFF),
        reinterpret_cast<const __m128i *>(mask128_epi8) + (mask3 & 0x7FFF));
    __m256i result1 = _mm256_shuffle_epi8(input_lo, vmask1);
    __m256i result2 = _mm256_shuffle_epi8(input_hi, vmask2);
    _mm256_storeu2_m128i(reinterpret_cast<__m128i *>(buffer + pop1),
                         reinterpret_cast<__m128i *>(buffer), result1);
    _mm256_storeu2_m128i(reinterpret_cast<__m128i *>(buffer + pop3),
                         reinterpret_cast<__m128i *>(buffer + pop2), result2);
    memcpy(out, buffer, pop4);
    out += pop4;
  }
  *out = '\0'; // NULL termination
  return out - initout;
}
} // namespace simdjson
#endif
/* end file src/jsonminifier.cpp */
/* begin file src/jsonparser.cpp */

namespace simdjson {

// The function that users are expected to call is json_parse.
// We have more than one such function because we want to support several
// instruction sets.

// function pointer type for json_parse
using json_parse_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj,
                                bool realloc_if_needed);

// Pointer that holds the json_parse implementation corresponding to the
// available SIMD instruction set
extern json_parse_functype *json_parse_ptr;

int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj,
               bool realloc_if_needed) {
  return json_parse_ptr(buf, len, pj, realloc_if_needed);
}

int json_parse(const char *buf, size_t len, ParsedJson &pj,
               bool realloc_if_needed) {
  return json_parse_ptr(reinterpret_cast<const uint8_t *>(buf), len, pj,
                        realloc_if_needed);
}

Architecture find_best_supported_implementation() {
  constexpr uint32_t haswell_flags =
      instruction_set::AVX2 | instruction_set::PCLMULQDQ |
      instruction_set::BMI1 | instruction_set::BMI2;
  constexpr uint32_t westmere_flags =
      instruction_set::SSE42 | instruction_set::PCLMULQDQ;

  uint32_t supports = detect_supported_architectures();
  // Order from best to worst (within architecture)
  if ((haswell_flags & supports) == haswell_flags)
    return Architecture::HASWELL;
  if ((westmere_flags & supports) == westmere_flags)
    return Architecture::WESTMERE;
  if (instruction_set::NEON)
    return Architecture::ARM64;

  return Architecture::NONE;
}

// Responsible to select the best json_parse implementation
int json_parse_dispatch(const uint8_t *buf, size_t len, ParsedJson &pj,
                        bool realloc_if_needed) {
  Architecture best_implementation = find_best_supported_implementation();
  // Selecting the best implementation
  switch (best_implementation) {
#ifdef IS_X86_64
  case Architecture::HASWELL:
    json_parse_ptr = &json_parse_implementation<Architecture::HASWELL>;
    break;
  case Architecture::WESTMERE:
    json_parse_ptr = &json_parse_implementation<Architecture::WESTMERE>;
    break;
#endif
#ifdef IS_ARM64
  case Architecture::ARM64:
    json_parse_ptr = &json_parse_implementation<Architecture::ARM64>;
    break;
#endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    return simdjson::UNEXPECTED_ERROR;
  }

  return json_parse_ptr(buf, len, pj, realloc_if_needed);
}

json_parse_functype *json_parse_ptr = &json_parse_dispatch;

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len,
                             bool realloc_if_needed) {
  ParsedJson pj;
  bool ok = pj.allocate_capacity(len);
  if (ok) {
    json_parse(buf, len, pj, realloc_if_needed);
  } else {
    std::cerr << "failure during memory allocation " << std::endl;
  }
  return pj;
}
} // namespace simdjson
/* end file src/jsonparser.cpp */
/* begin file src/stage1_find_marks.cpp */

#ifdef IS_X86_64

TARGET_HASWELL
namespace simdjson {
template <>
int find_structural_bits<Architecture::HASWELL>(const uint8_t *buf, size_t len,
                                                ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(Architecture::HASWELL, buf, len, pj,
                       simdjson::haswell::flatten_bits);
}
} // namespace simdjson
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {
template <>
int find_structural_bits<Architecture::WESTMERE>(const uint8_t *buf, size_t len,
                                                 ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(Architecture::WESTMERE, buf, len, pj,
                       simdjson::flatten_bits);
}
} // namespace simdjson
UNTARGET_REGION

#endif

#ifdef IS_ARM64
namespace simdjson {
template <>
int find_structural_bits<Architecture::ARM64>(const uint8_t *buf, size_t len,
                                              ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(Architecture::ARM64, buf, len, pj,
                       simdjson::flatten_bits);
}
} // namespace simdjson
#endif
/* end file src/stage1_find_marks.cpp */
/* begin file src/stage2_build_tape.cpp */

namespace simdjson {

// this macro reads the next structural character, updating idx, i and c.
#define UPDATE_CHAR()                                                          \
  {                                                                            \
    idx = pj.structural_indexes[i++];                                          \
    c = buf[idx];                                                              \
  }

#ifdef SIMDJSON_USE_COMPUTED_GOTO
#define SET_GOTO_ARRAY_CONTINUE() pj.ret_address[depth] = &&array_continue;
#define SET_GOTO_OBJECT_CONTINUE() pj.ret_address[depth] = &&object_continue;
#define SET_GOTO_START_CONTINUE() pj.ret_address[depth] = &&start_continue;
#define GOTO_CONTINUE() goto *pj.ret_address[depth];
#else
#define SET_GOTO_ARRAY_CONTINUE() pj.ret_address[depth] = 'a';
#define SET_GOTO_OBJECT_CONTINUE() pj.ret_address[depth] = 'o';
#define SET_GOTO_START_CONTINUE() pj.ret_address[depth] = 's';
#define GOTO_CONTINUE()                                                        \
  {                                                                            \
    if (pj.ret_address[depth] == 'a') {                                        \
      goto array_continue;                                                     \
    } else if (pj.ret_address[depth] == 'o') {                                 \
      goto object_continue;                                                    \
    } else {                                                                   \
      goto start_continue;                                                     \
    }                                                                          \
  }
#endif

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
// We need to compile that code for multiple architectures. However, target
// attributes can be used only once by function definition. Huge macro seemed
// better than huge code duplication. int UNIFIED_MACHINE(const uint8_t *buf,
// size_t len, ParsedJson &pj)
#define UNIFIED_MACHINE(T, buf, len, pj)                                       \
  {                                                                            \
    if (ALLOW_SAME_PAGE_BUFFER_OVERRUN) {                                      \
      memset((uint8_t *)buf + len, 0,                                          \
             SIMDJSON_PADDING); /* to please valgrind */                       \
    }                                                                          \
    uint32_t i = 0; /* index of the structural character (0,1,2,3...) */       \
    uint32_t                                                                   \
        idx;   /* location of the structural character in the input (buf)   */ \
    uint8_t c; /* used to track the (structural) character we are looking at,  \
                  updated */                                                   \
    /* by UPDATE_CHAR macro */                                                 \
    uint32_t depth = 0; /* could have an arbitrary starting depth */           \
    pj.init();          /* sets is_valid to false          */                  \
    if (pj.byte_capacity < len) {                                              \
      pj.error_code = simdjson::CAPACITY;                                      \
      return pj.error_code;                                                    \
    }                                                                          \
                                                                               \
    /*//////////////////////////// START STATE /////////////////////////////   \
     */                                                                        \
    SET_GOTO_START_CONTINUE()                                                  \
    pj.containing_scope_offset[depth] = pj.get_current_loc();                  \
    pj.write_tape(0, 'r'); /* r for root, 0 is going to get overwritten */     \
    /* the root is used, if nothing else, to capture the size of the tape */   \
    depth++; /* everything starts at depth = 1, depth = 0 is just for the      \
                root, the root may contain an object, an array or something    \
                else. */                                                       \
    if (depth >= pj.depth_capacity) {                                          \
      goto fail;                                                               \
    }                                                                          \
                                                                               \
    UPDATE_CHAR();                                                             \
    switch (c) {                                                               \
    case '{':                                                                  \
      pj.containing_scope_offset[depth] = pj.get_current_loc();                \
      SET_GOTO_START_CONTINUE();                                               \
      depth++;                                                                 \
      if (depth >= pj.depth_capacity) {                                        \
        goto fail;                                                             \
      }                                                                        \
      pj.write_tape(                                                           \
          0,                                                                   \
          c); /* strangely, moving this to object_begin slows things down */   \
      goto object_begin;                                                       \
    case '[':                                                                  \
      pj.containing_scope_offset[depth] = pj.get_current_loc();                \
      SET_GOTO_START_CONTINUE();                                               \
      depth++;                                                                 \
      if (depth >= pj.depth_capacity) {                                        \
        goto fail;                                                             \
      }                                                                        \
      pj.write_tape(0, c);                                                     \
      goto array_begin;                                                        \
      /* #define SIMDJSON_ALLOWANYTHINGINROOT                                  \
       * A JSON text is a serialized value.  Note that certain previous        \
       * specifications of JSON constrained a JSON text to be an object or an  \
       * array.  Implementations that generate only objects or arrays where a  \
       * JSON text is called for will be interoperable in the sense that all   \
       * implementations will accept these as conforming JSON texts.           \
       * https://tools.ietf.org/html/rfc8259                                   \
       * #ifdef SIMDJSON_ALLOWANYTHINGINROOT */                                \
    case '"': {                                                                \
      if (!parse_string<T>(buf, len, pj, depth, idx)) {                        \
        goto fail;                                                             \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    case 't': {                                                                \
      /* we need to make a copy to make sure that the string is space          \
       * terminated.                                                           \
       * this only applies to the JSON document made solely of the true value. \
       * this will almost never be called in practice */                       \
      char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));        \
      if (copy == nullptr) {                                                   \
        goto fail;                                                             \
      }                                                                        \
      memcpy(copy, buf, len);                                                  \
      copy[len] = ' ';                                                         \
      if (!is_valid_true_atom(reinterpret_cast<const uint8_t *>(copy) +        \
                              idx)) {                                          \
        free(copy);                                                            \
        goto fail;                                                             \
      }                                                                        \
      free(copy);                                                              \
      pj.write_tape(0, c);                                                     \
      break;                                                                   \
    }                                                                          \
    case 'f': {                                                                \
      /* we need to make a copy to make sure that the string is space          \
       * terminated.                                                           \
       * this only applies to the JSON document made solely of the false       \
       * value.                                                                \
       * this will almost never be called in practice */                       \
      char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));        \
      if (copy == nullptr) {                                                   \
        goto fail;                                                             \
      }                                                                        \
      memcpy(copy, buf, len);                                                  \
      copy[len] = ' ';                                                         \
      if (!is_valid_false_atom(reinterpret_cast<const uint8_t *>(copy) +       \
                               idx)) {                                         \
        free(copy);                                                            \
        goto fail;                                                             \
      }                                                                        \
      free(copy);                                                              \
      pj.write_tape(0, c);                                                     \
      break;                                                                   \
    }                                                                          \
    case 'n': {                                                                \
      /* we need to make a copy to make sure that the string is space          \
       * terminated.                                                           \
       * this only applies to the JSON document made solely of the null value. \
       * this will almost never be called in practice */                       \
      char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));        \
      if (copy == nullptr) {                                                   \
        goto fail;                                                             \
      }                                                                        \
      memcpy(copy, buf, len);                                                  \
      copy[len] = ' ';                                                         \
      if (!is_valid_null_atom(reinterpret_cast<const uint8_t *>(copy) +        \
                              idx)) {                                          \
        free(copy);                                                            \
        goto fail;                                                             \
      }                                                                        \
      free(copy);                                                              \
      pj.write_tape(0, c);                                                     \
      break;                                                                   \
    }                                                                          \
    case '0':                                                                  \
    case '1':                                                                  \
    case '2':                                                                  \
    case '3':                                                                  \
    case '4':                                                                  \
    case '5':                                                                  \
    case '6':                                                                  \
    case '7':                                                                  \
    case '8':                                                                  \
    case '9': {                                                                \
      /* we need to make a copy to make sure that the string is space          \
       * terminated.                                                           \
       * this is done only for JSON documents made of a sole number            \
       * this will almost never be called in practice. We terminate with a     \
       * space                                                                 \
       * because we do not want to allow NULLs in the middle of a number       \
       * (whereas a                                                            \
       * space in the middle of a number would be identified in stage 1). */   \
      char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));        \
      if (copy == nullptr) {                                                   \
        goto fail;                                                             \
      }                                                                        \
      memcpy(copy, buf, len);                                                  \
      copy[len] = ' ';                                                         \
      if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx,      \
                        false)) {                                              \
        free(copy);                                                            \
        goto fail;                                                             \
      }                                                                        \
      free(copy);                                                              \
      break;                                                                   \
    }                                                                          \
    case '-': {                                                                \
      /* we need to make a copy to make sure that the string is NULL           \
       * terminated.                                                           \
       * this is done only for JSON documents made of a sole number            \
       * this will almost never be called in practice */                       \
      char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));        \
      if (copy == nullptr) {                                                   \
        goto fail;                                                             \
      }                                                                        \
      memcpy(copy, buf, len);                                                  \
      copy[len] = ' ';                                                         \
      if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx,      \
                        true)) {                                               \
        free(copy);                                                            \
        goto fail;                                                             \
      }                                                                        \
      free(copy);                                                              \
      break;                                                                   \
    }                                                                          \
    default:                                                                   \
      goto fail;                                                               \
    }                                                                          \
  start_continue:                                                              \
    /* the string might not be NULL terminated. */                             \
    if (i + 1 == pj.n_structural_indexes) {                                    \
      goto succeed;                                                            \
    } else {                                                                   \
      goto fail;                                                               \
    }                                                                          \
    /*//////////////////////////// OBJECT STATES ///////////////////////////*/ \
                                                                               \
  object_begin:                                                                \
    UPDATE_CHAR();                                                             \
    switch (c) {                                                               \
    case '"': {                                                                \
      if (!parse_string<T>(buf, len, pj, depth, idx)) {                        \
        goto fail;                                                             \
      }                                                                        \
      goto object_key_state;                                                   \
    }                                                                          \
    case '}':                                                                  \
      goto scope_end; /* could also go to object_continue */                   \
    default:                                                                   \
      goto fail;                                                               \
    }                                                                          \
                                                                               \
  object_key_state:                                                            \
    UPDATE_CHAR();                                                             \
    if (c != ':') {                                                            \
      goto fail;                                                               \
    }                                                                          \
    UPDATE_CHAR();                                                             \
    switch (c) {                                                               \
    case '"': {                                                                \
      if (!parse_string<T>(buf, len, pj, depth, idx)) {                        \
        goto fail;                                                             \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    case 't':                                                                  \
      if (!is_valid_true_atom(buf + idx)) {                                    \
        goto fail;                                                             \
      }                                                                        \
      pj.write_tape(0, c);                                                     \
      break;                                                                   \
    case 'f':                                                                  \
      if (!is_valid_false_atom(buf + idx)) {                                   \
        goto fail;                                                             \
      }                                                                        \
      pj.write_tape(0, c);                                                     \
      break;                                                                   \
    case 'n':                                                                  \
      if (!is_valid_null_atom(buf + idx)) {                                    \
        goto fail;                                                             \
      }                                                                        \
      pj.write_tape(0, c);                                                     \
      break;                                                                   \
    case '0':                                                                  \
    case '1':                                                                  \
    case '2':                                                                  \
    case '3':                                                                  \
    case '4':                                                                  \
    case '5':                                                                  \
    case '6':                                                                  \
    case '7':                                                                  \
    case '8':                                                                  \
    case '9': {                                                                \
      if (!parse_number(buf, pj, idx, false)) {                                \
        goto fail;                                                             \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    case '-': {                                                                \
      if (!parse_number(buf, pj, idx, true)) {                                 \
        goto fail;                                                             \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    case '{': {                                                                \
      pj.containing_scope_offset[depth] = pj.get_current_loc();                \
      pj.write_tape(0, c); /* here the compilers knows what c is so this gets  \
                              optimized */                                     \
      /* we have not yet encountered } so we need to come back for it */       \
      SET_GOTO_OBJECT_CONTINUE()                                               \
      /* we found an object inside an object, so we need to increment the      \
       * depth                                                             */  \
      depth++;                                                                 \
      if (depth >= pj.depth_capacity) {                                        \
        goto fail;                                                             \
      }                                                                        \
                                                                               \
      goto object_begin;                                                       \
    }                                                                          \
    case '[': {                                                                \
      pj.containing_scope_offset[depth] = pj.get_current_loc();                \
      pj.write_tape(0, c); /* here the compilers knows what c is so this gets  \
                              optimized */                                     \
      /* we have not yet encountered } so we need to come back for it */       \
      SET_GOTO_OBJECT_CONTINUE()                                               \
      /* we found an array inside an object, so we need to increment the depth \
       */                                                                      \
      depth++;                                                                 \
      if (depth >= pj.depth_capacity) {                                        \
        goto fail;                                                             \
      }                                                                        \
      goto array_begin;                                                        \
    }                                                                          \
    default:                                                                   \
      goto fail;                                                               \
    }                                                                          \
                                                                               \
  object_continue:                                                             \
    UPDATE_CHAR();                                                             \
    switch (c) {                                                               \
    case ',':                                                                  \
      UPDATE_CHAR();                                                           \
      if (c != '"') {                                                          \
        goto fail;                                                             \
      } else {                                                                 \
        if (!parse_string<T>(buf, len, pj, depth, idx)) {                      \
          goto fail;                                                           \
        }                                                                      \
        goto object_key_state;                                                 \
      }                                                                        \
    case '}':                                                                  \
      goto scope_end;                                                          \
    default:                                                                   \
      goto fail;                                                               \
    }                                                                          \
                                                                               \
    /*//////////////////////////// COMMON STATE ///////////////////////////*/  \
                                                                               \
  scope_end:                                                                   \
    /* write our tape location to the header scope */                          \
    depth--;                                                                   \
    pj.write_tape(pj.containing_scope_offset[depth], c);                       \
    pj.annotate_previous_loc(pj.containing_scope_offset[depth],                \
                             pj.get_current_loc());                            \
    /* goto saved_state */                                                     \
    GOTO_CONTINUE()                                                            \
                                                                               \
    /*//////////////////////////// ARRAY STATES ///////////////////////////*/  \
  array_begin:                                                                 \
    UPDATE_CHAR();                                                             \
    if (c == ']') {                                                            \
      goto scope_end; /* could also go to array_continue */                    \
    }                                                                          \
                                                                               \
  main_array_switch:                                                           \
    /* we call update char on all paths in, so we can peek at c on the         \
     * on paths that can accept a close square brace (post-, and at start) */  \
    switch (c) {                                                               \
    case '"': {                                                                \
      if (!parse_string<T>(buf, len, pj, depth, idx)) {                        \
        goto fail;                                                             \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    case 't':                                                                  \
      if (!is_valid_true_atom(buf + idx)) {                                    \
        goto fail;                                                             \
      }                                                                        \
      pj.write_tape(0, c);                                                     \
      break;                                                                   \
    case 'f':                                                                  \
      if (!is_valid_false_atom(buf + idx)) {                                   \
        goto fail;                                                             \
      }                                                                        \
      pj.write_tape(0, c);                                                     \
      break;                                                                   \
    case 'n':                                                                  \
      if (!is_valid_null_atom(buf + idx)) {                                    \
        goto fail;                                                             \
      }                                                                        \
      pj.write_tape(0, c);                                                     \
      break; /* goto array_continue; */                                        \
                                                                               \
    case '0':                                                                  \
    case '1':                                                                  \
    case '2':                                                                  \
    case '3':                                                                  \
    case '4':                                                                  \
    case '5':                                                                  \
    case '6':                                                                  \
    case '7':                                                                  \
    case '8':                                                                  \
    case '9': {                                                                \
      if (!parse_number(buf, pj, idx, false)) {                                \
        goto fail;                                                             \
      }                                                                        \
      break; /* goto array_continue; */                                        \
    }                                                                          \
    case '-': {                                                                \
      if (!parse_number(buf, pj, idx, true)) {                                 \
        goto fail;                                                             \
      }                                                                        \
      break; /* goto array_continue; */                                        \
    }                                                                          \
    case '{': {                                                                \
      /* we have not yet encountered ] so we need to come back for it */       \
      pj.containing_scope_offset[depth] = pj.get_current_loc();                \
      pj.write_tape(0, c); /* here the compilers knows what c is so this gets  \
                              optimized */                                     \
      SET_GOTO_ARRAY_CONTINUE()                                                \
      /* we found an object inside an array, so we need to increment the depth \
       */                                                                      \
      depth++;                                                                 \
      if (depth >= pj.depth_capacity) {                                        \
        goto fail;                                                             \
      }                                                                        \
                                                                               \
      goto object_begin;                                                       \
    }                                                                          \
    case '[': {                                                                \
      /* we have not yet encountered ] so we need to come back for it */       \
      pj.containing_scope_offset[depth] = pj.get_current_loc();                \
      pj.write_tape(0, c); /* here the compilers knows what c is so this gets  \
                              optimized */                                     \
      SET_GOTO_ARRAY_CONTINUE()                                                \
      /* we found an array inside an array, so we need to increment the depth  \
       */                                                                      \
      depth++;                                                                 \
      if (depth >= pj.depth_capacity) {                                        \
        goto fail;                                                             \
      }                                                                        \
      goto array_begin;                                                        \
    }                                                                          \
    default:                                                                   \
      goto fail;                                                               \
    }                                                                          \
                                                                               \
  array_continue:                                                              \
    UPDATE_CHAR();                                                             \
    switch (c) {                                                               \
    case ',':                                                                  \
      UPDATE_CHAR();                                                           \
      goto main_array_switch;                                                  \
    case ']':                                                                  \
      goto scope_end;                                                          \
    default:                                                                   \
      goto fail;                                                               \
    }                                                                          \
                                                                               \
    /*//////////////////////////// FINAL STATES ///////////////////////////*/  \
                                                                               \
  succeed:                                                                     \
    depth--;                                                                   \
    if (depth != 0) {                                                          \
      fprintf(stderr, "internal bug\n");                                       \
      abort();                                                                 \
    }                                                                          \
    if (pj.containing_scope_offset[depth] != 0) {                              \
      fprintf(stderr, "internal bug\n");                                       \
      abort();                                                                 \
    }                                                                          \
    pj.annotate_previous_loc(pj.containing_scope_offset[depth],                \
                             pj.get_current_loc());                            \
    pj.write_tape(pj.containing_scope_offset[depth], 'r'); /* r is root */     \
                                                                               \
    pj.valid = true;                                                           \
    pj.error_code = simdjson::SUCCESS;                                         \
    return pj.error_code;                                                      \
  fail:                                                                        \
    /* we do not need the next line because this is done by pj.init(),         \
     * pessimistically.                                                        \
     * pj.is_valid  = false;                                                   \
     * At this point in the code, we have all the time in the world.           \
     * Note that we know exactly where we are in the document so we could,     \
     * without any overhead on the processing code, report a specific          \
     * location.                                                               \
     * We could even trigger special code paths to assess what happened        \
     * carefully,                                                              \
     * all without any added cost. */                                          \
    if (depth >= pj.depth_capacity) {                                          \
      pj.error_code = simdjson::DEPTH_ERROR;                                   \
      return pj.error_code;                                                    \
    }                                                                          \
    switch (c) {                                                               \
    case '"':                                                                  \
      pj.error_code = simdjson::STRING_ERROR;                                  \
      return pj.error_code;                                                    \
    case '0':                                                                  \
    case '1':                                                                  \
    case '2':                                                                  \
    case '3':                                                                  \
    case '4':                                                                  \
    case '5':                                                                  \
    case '6':                                                                  \
    case '7':                                                                  \
    case '8':                                                                  \
    case '9':                                                                  \
    case '-':                                                                  \
      pj.error_code = simdjson::NUMBER_ERROR;                                  \
      return pj.error_code;                                                    \
    case 't':                                                                  \
      pj.error_code = simdjson::T_ATOM_ERROR;                                  \
      return pj.error_code;                                                    \
    case 'n':                                                                  \
      pj.error_code = simdjson::N_ATOM_ERROR;                                  \
      return pj.error_code;                                                    \
    case 'f':                                                                  \
      pj.error_code = simdjson::F_ATOM_ERROR;                                  \
      return pj.error_code;                                                    \
    default:                                                                   \
      break;                                                                   \
    }                                                                          \
    pj.error_code = simdjson::TAPE_ERROR;                                      \
    return pj.error_code;                                                      \
  }

} // namespace simdjson

#ifdef IS_X86_64
TARGET_HASWELL
namespace simdjson {
template <>
WARN_UNUSED ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER LENIENT_MEM_SANITIZER int
unified_machine<Architecture::HASWELL>(const uint8_t *buf, size_t len,
                                       ParsedJson &pj) {
  UNIFIED_MACHINE(Architecture::HASWELL, buf, len, pj);
}
} // namespace simdjson
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {
template <>
WARN_UNUSED ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER LENIENT_MEM_SANITIZER int
unified_machine<Architecture::WESTMERE>(const uint8_t *buf, size_t len,
                                        ParsedJson &pj) {
  UNIFIED_MACHINE(Architecture::WESTMERE, buf, len, pj);
}
} // namespace simdjson
UNTARGET_REGION
#endif // IS_X86_64

#ifdef IS_ARM64
namespace simdjson {
template <>
WARN_UNUSED ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER LENIENT_MEM_SANITIZER int
unified_machine<Architecture::ARM64>(const uint8_t *buf, size_t len,
                                     ParsedJson &pj) {
  UNIFIED_MACHINE(Architecture::ARM64, buf, len, pj);
}
} // namespace simdjson
#endif
/* end file src/stage2_build_tape.cpp */
/* begin file src/parsedjson.cpp */

namespace simdjson {
ParsedJson::ParsedJson()
    : structural_indexes(nullptr), tape(nullptr),
      containing_scope_offset(nullptr), ret_address(nullptr),
      string_buf(nullptr), current_string_buf_loc(nullptr) {}

ParsedJson::~ParsedJson() { deallocate(); }

ParsedJson::ParsedJson(ParsedJson &&p)
    : byte_capacity(p.byte_capacity), depth_capacity(p.depth_capacity),
      tape_capacity(p.tape_capacity), string_capacity(p.string_capacity),
      current_loc(p.current_loc), n_structural_indexes(p.n_structural_indexes),
      structural_indexes(p.structural_indexes), tape(p.tape),
      containing_scope_offset(p.containing_scope_offset),
      ret_address(p.ret_address), string_buf(p.string_buf),
      current_string_buf_loc(p.current_string_buf_loc), valid(p.valid) {
  p.structural_indexes = nullptr;
  p.tape = nullptr;
  p.containing_scope_offset = nullptr;
  p.ret_address = nullptr;
  p.string_buf = nullptr;
  p.current_string_buf_loc = nullptr;
}

WARN_UNUSED
bool ParsedJson::allocate_capacity(size_t len, size_t max_depth) {
  if (max_depth <= 0) {
    max_depth = 1; // don't let the user allocate nothing
  }
  if (len <= 0) {
    len = 64; // allocating 0 bytes is wasteful.
  }
  if (len > SIMDJSON_MAXSIZE_BYTES) {
    return false;
  }
  if ((len <= byte_capacity) && (depth_capacity < max_depth)) {
    return true;
  }
  deallocate();
  valid = false;
  byte_capacity = 0; // will only set it to len after allocations are a success
  n_structural_indexes = 0;
  uint32_t max_structures = ROUNDUP_N(len, 64) + 2 + 7;
  structural_indexes = new (std::nothrow) uint32_t[max_structures];
  // a pathological input like "[[[[..." would generate len tape elements, so
  // need a capacity of len + 1
  size_t local_tape_capacity = ROUNDUP_N(len + 1, 64);
  // a document with only zero-length strings... could have len/3 string
  // and we would need len/3 * 5 bytes on the string buffer
  size_t local_string_capacity = ROUNDUP_N(5 * len / 3 + 32, 64);
  string_buf = new (std::nothrow) uint8_t[local_string_capacity];
  tape = new (std::nothrow) uint64_t[local_tape_capacity];
  containing_scope_offset = new (std::nothrow) uint32_t[max_depth];
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  ret_address = new (std::nothrow) void *[max_depth];
#else
  ret_address = new (std::nothrow) char[max_depth];
#endif
  if ((string_buf == nullptr) || (tape == nullptr) ||
      (containing_scope_offset == nullptr) || (ret_address == nullptr) ||
      (structural_indexes == nullptr)) {
    std::cerr << "Could not allocate memory" << std::endl;
    delete[] ret_address;
    delete[] containing_scope_offset;
    delete[] tape;
    delete[] string_buf;
    delete[] structural_indexes;

    return false;
  }
  /*
  // We do not need to initialize this content for parsing, though we could
  // need to initialize it for safety.
  memset(string_buf, 0 , local_string_capacity);
  memset(structural_indexes, 0, max_structures * sizeof(uint32_t));
  memset(tape, 0, local_tape_capacity * sizeof(uint64_t));
  */
  byte_capacity = len;
  depth_capacity = max_depth;
  tape_capacity = local_tape_capacity;
  string_capacity = local_string_capacity;
  return true;
}

bool ParsedJson::is_valid() const { return valid; }

int ParsedJson::get_error_code() const { return error_code; }

std::string ParsedJson::get_error_message() const {
  return error_message(error_code);
}

void ParsedJson::deallocate() {
  byte_capacity = 0;
  depth_capacity = 0;
  tape_capacity = 0;
  string_capacity = 0;
  delete[] ret_address;
  delete[] containing_scope_offset;
  delete[] tape;
  delete[] string_buf;
  delete[] structural_indexes;
  valid = false;
}

void ParsedJson::init() {
  current_string_buf_loc = string_buf;
  current_loc = 0;
  valid = false;
}

WARN_UNUSED
bool ParsedJson::print_json(std::ostream &os) {
  if (!valid) {
    return false;
  }
  uint32_t string_length;
  size_t tape_idx = 0;
  uint64_t tape_val = tape[tape_idx];
  uint8_t type = (tape_val >> 56);
  size_t how_many = 0;
  if (type == 'r') {
    how_many = tape_val & JSON_VALUE_MASK;
  } else {
    fprintf(stderr, "Error: no starting root node?");
    return false;
  }
  if (how_many > tape_capacity) {
    fprintf(
        stderr,
        "We may be exceeding the tape capacity. Is this a valid document?\n");
    return false;
  }
  tape_idx++;
  bool *in_object = new bool[depth_capacity];
  auto *in_object_idx = new size_t[depth_capacity];
  int depth = 1; // only root at level 0
  in_object_idx[depth] = 0;
  in_object[depth] = false;
  for (; tape_idx < how_many; tape_idx++) {
    tape_val = tape[tape_idx];
    uint64_t payload = tape_val & JSON_VALUE_MASK;
    type = (tape_val >> 56);
    if (!in_object[depth]) {
      if ((in_object_idx[depth] > 0) && (type != ']')) {
        os << ",";
      }
      in_object_idx[depth]++;
    } else { // if (in_object) {
      if ((in_object_idx[depth] > 0) && ((in_object_idx[depth] & 1) == 0) &&
          (type != '}')) {
        os << ",";
      }
      if (((in_object_idx[depth] & 1) == 1)) {
        os << ":";
      }
      in_object_idx[depth]++;
    }
    switch (type) {
    case '"': // we have a string
      os << '"';
      memcpy(&string_length, string_buf + payload, sizeof(uint32_t));
      print_with_escapes(
          (const unsigned char *)(string_buf + payload + sizeof(uint32_t)),
          string_length);
      os << '"';
      break;
    case 'l': // we have a long int
      if (tape_idx + 1 >= how_many) {
        delete[] in_object;
        delete[] in_object_idx;
        return false;
      }
      os << static_cast<int64_t>(tape[++tape_idx]);
      break;
    case 'd': // we have a double
      if (tape_idx + 1 >= how_many) {
        delete[] in_object;
        delete[] in_object_idx;
        return false;
      }
      double answer;
      memcpy(&answer, &tape[++tape_idx], sizeof(answer));
      os << answer;
      break;
    case 'n': // we have a null
      os << "null";
      break;
    case 't': // we have a true
      os << "true";
      break;
    case 'f': // we have a false
      os << "false";
      break;
    case '{': // we have an object
      os << '{';
      depth++;
      in_object[depth] = true;
      in_object_idx[depth] = 0;
      break;
    case '}': // we end an object
      depth--;
      os << '}';
      break;
    case '[': // we start an array
      os << '[';
      depth++;
      in_object[depth] = false;
      in_object_idx[depth] = 0;
      break;
    case ']': // we end an array
      depth--;
      os << ']';
      break;
    case 'r': // we start and end with the root node
      fprintf(stderr, "should we be hitting the root node?\n");
      delete[] in_object;
      delete[] in_object_idx;
      return false;
    default:
      fprintf(stderr, "bug %c\n", type);
      delete[] in_object;
      delete[] in_object_idx;
      return false;
    }
  }
  delete[] in_object;
  delete[] in_object_idx;
  return true;
}

WARN_UNUSED
bool ParsedJson::dump_raw_tape(std::ostream &os) {
  if (!valid) {
    return false;
  }
  uint32_t string_length;
  size_t tape_idx = 0;
  uint64_t tape_val = tape[tape_idx];
  uint8_t type = (tape_val >> 56);
  os << tape_idx << " : " << type;
  tape_idx++;
  size_t how_many = 0;
  if (type == 'r') {
    how_many = tape_val & JSON_VALUE_MASK;
  } else {
    fprintf(stderr, "Error: no starting root node?");
    return false;
  }
  os << "\t// pointing to " << how_many << " (right after last node)\n";
  uint64_t payload;
  for (; tape_idx < how_many; tape_idx++) {
    os << tape_idx << " : ";
    tape_val = tape[tape_idx];
    payload = tape_val & JSON_VALUE_MASK;
    type = (tape_val >> 56);
    switch (type) {
    case '"': // we have a string
      os << "string \"";
      memcpy(&string_length, string_buf + payload, sizeof(uint32_t));
      print_with_escapes(
          (const unsigned char *)(string_buf + payload + sizeof(uint32_t)),
          string_length);
      os << '"';
      os << '\n';
      break;
    case 'l': // we have a long int
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "integer " << static_cast<int64_t>(tape[++tape_idx]) << "\n";
      break;
    case 'd': // we have a double
      os << "float ";
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      double answer;
      memcpy(&answer, &tape[++tape_idx], sizeof(answer));
      os << answer << '\n';
      break;
    case 'n': // we have a null
      os << "null\n";
      break;
    case 't': // we have a true
      os << "true\n";
      break;
    case 'f': // we have a false
      os << "false\n";
      break;
    case '{': // we have an object
      os << "{\t// pointing to next tape location " << payload
         << " (first node after the scope) \n";
      break;
    case '}': // we end an object
      os << "}\t// pointing to previous tape location " << payload
         << " (start of the scope) \n";
      break;
    case '[': // we start an array
      os << "[\t// pointing to next tape location " << payload
         << " (first node after the scope) \n";
      break;
    case ']': // we end an array
      os << "]\t// pointing to previous tape location " << payload
         << " (start of the scope) \n";
      break;
    case 'r': // we start and end with the root node
      printf("end of root\n");
      return false;
    default:
      return false;
    }
  }
  tape_val = tape[tape_idx];
  payload = tape_val & JSON_VALUE_MASK;
  type = (tape_val >> 56);
  os << tape_idx << " : " << type << "\t// pointing to " << payload
     << " (start root)\n";
  return true;
}
} // namespace simdjson
/* end file src/parsedjson.cpp */
/* begin file src/parsedjsoniterator.cpp */
#include <iterator>

namespace simdjson {
ParsedJson::Iterator::Iterator(ParsedJson &pj_)
    : pj(pj_), depth(0), location(0), tape_length(0), depth_index(nullptr) {
  if (!pj.is_valid()) {
    throw InvalidJSON();
  }
  // we overallocate by "1" to silence a warning in Visual Studio
  depth_index = new scopeindex_t[pj.depth_capacity + 1];
  // memory allocation would throw
  // if(depth_index == nullptr) {
  //    return;
  //}
  depth_index[0].start_of_scope = location;
  current_val = pj.tape[location++];
  current_type = (current_val >> 56);
  depth_index[0].scope_type = current_type;
  if (current_type == 'r') {
    tape_length = current_val & JSON_VALUE_MASK;
    if (location < tape_length) {
      // If we make it here, then depth_capacity must >=2, but the compiler
      // may not know this.
      current_val = pj.tape[location];
      current_type = (current_val >> 56);
      depth++;
      depth_index[depth].start_of_scope = location;
      depth_index[depth].scope_type = current_type;
    }
  } else {
    // should never happen
    throw InvalidJSON();
  }
}

ParsedJson::Iterator::~Iterator() { delete[] depth_index; }

ParsedJson::Iterator::Iterator(const Iterator &o) noexcept
    : pj(o.pj), depth(o.depth), location(o.location), tape_length(0),
      current_type(o.current_type), current_val(o.current_val),
      depth_index(nullptr) {
  depth_index = new scopeindex_t[pj.depth_capacity];
  // allocation might throw
  memcpy(depth_index, o.depth_index,
         pj.depth_capacity * sizeof(depth_index[0]));
  tape_length = o.tape_length;
}

ParsedJson::Iterator::Iterator(Iterator &&o) noexcept
    : pj(o.pj), depth(o.depth), location(o.location),
      tape_length(o.tape_length), current_type(o.current_type),
      current_val(o.current_val), depth_index(o.depth_index) {
  o.depth_index = nullptr; // we take ownership
}

bool ParsedJson::Iterator::print(std::ostream &os, bool escape_strings) const {
  if (!is_ok()) {
    return false;
  }
  switch (current_type) {
  case '"': // we have a string
    os << '"';
    if (escape_strings) {
      print_with_escapes(get_string(), os, get_string_length());
    } else {
      // was: os << get_string();, but given that we can include null chars, we
      // have to do something crazier:
      std::copy(get_string(), get_string() + get_string_length(),
                std::ostream_iterator<char>(os));
    }
    os << '"';
    break;
  case 'l': // we have a long int
    os << get_integer();
    break;
  case 'd':
    os << get_double();
    break;
  case 'n': // we have a null
    os << "null";
    break;
  case 't': // we have a true
    os << "true";
    break;
  case 'f': // we have a false
    os << "false";
    break;
  case '{': // we have an object
  case '}': // we end an object
  case '[': // we start an array
  case ']': // we end an array
    os << static_cast<char>(current_type);
    break;
  default:
    return false;
  }
  return true;
}

bool ParsedJson::Iterator::move_to(const char *pointer, uint32_t length) {
  char *new_pointer = nullptr;
  if (pointer[0] == '#') {
    // Converting fragment representation to string representation
    new_pointer = new char[length];
    uint32_t new_length = 0;
    for (uint32_t i = 1; i < length; i++) {
      if (pointer[i] == '%' && pointer[i + 1] == 'x') {
        try {
          int fragment =
              std::stoi(std::string(&pointer[i + 2], 2), nullptr, 16);
          if (fragment == '\\' || fragment == '"' || (fragment <= 0x1F)) {
            // escaping the character
            new_pointer[new_length] = '\\';
            new_length++;
          }
          new_pointer[new_length] = fragment;
          i += 3;
        } catch (std::invalid_argument &) {
          delete[] new_pointer;
          return false; // the fragment is invalid
        }
      } else {
        new_pointer[new_length] = pointer[i];
      }
      new_length++;
    }
    length = new_length;
    pointer = new_pointer;
  }

  // saving the current state
  size_t depth_s = depth;
  size_t location_s = location;
  uint8_t current_type_s = current_type;
  uint64_t current_val_s = current_val;
  scopeindex_t *depth_index_s = depth_index;

  rewind(); // The json pointer is used from the root of the document.

  bool found = relative_move_to(pointer, length);
  delete[] new_pointer;

  if (!found) {
    // since the pointer has found nothing, we get back to the original
    // position.
    depth = depth_s;
    location = location_s;
    current_type = current_type_s;
    current_val = current_val_s;
    depth_index = depth_index_s;
  }

  return found;
}

bool ParsedJson::Iterator::relative_move_to(const char *pointer,
                                            uint32_t length) {
  if (length == 0) {
    // returns the whole document
    return true;
  }

  if (pointer[0] != '/') {
    // '/' must be the first character
    return false;
  }

  // finding the key in an object or the index in an array
  std::string key_or_index;
  uint32_t offset = 1;

  // checking for the "-" case
  if (is_array() && pointer[1] == '-') {
    if (length != 2) {
      // the pointer must be exactly "/-"
      // there can't be anything more after '-' as an index
      return false;
    }
    key_or_index = '-';
    offset = length; // will skip the loop coming right after
  }

  // We either transform the first reference token to a valid json key
  // or we make sure it is a valid index in an array.
  for (; offset < length; offset++) {
    if (pointer[offset] == '/') {
      // beginning of the next key or index
      break;
    }
    if (is_array() && (pointer[offset] < '0' || pointer[offset] > '9')) {
      // the index of an array must be an integer
      // we also make sure std::stoi won't discard whitespaces later
      return false;
    }
    if (pointer[offset] == '~') {
      // "~1" represents "/"
      if (pointer[offset + 1] == '1') {
        key_or_index += '/';
        offset++;
        continue;
      }
      // "~0" represents "~"
      if (pointer[offset + 1] == '0') {
        key_or_index += '~';
        offset++;
        continue;
      }
    }
    if (pointer[offset] == '\\') {
      if (pointer[offset + 1] == '\\' || pointer[offset + 1] == '"' ||
          (pointer[offset + 1] <= 0x1F)) {
        key_or_index += pointer[offset + 1];
        offset++;
        continue;
      }
      return false; // invalid escaped character
    }
    if (pointer[offset] == '\"') {
      // unescaped quote character. this is an invalid case.
      // lets do nothing and assume most pointers will be valid.
      // it won't find any corresponding json key anyway.
      // return false;
    }
    key_or_index += pointer[offset];
  }

  bool found = false;
  if (is_object()) {
    if (move_to_key(key_or_index.c_str(), key_or_index.length())) {
      found = relative_move_to(pointer + offset, length - offset);
    }
  } else if (is_array()) {
    if (key_or_index == "-") { // handling "-" case first
      if (down()) {
        while (next())
          ; // moving to the end of the array
        // moving to the nonexistent value right after...
        size_t npos;
        if ((current_type == '[') || (current_type == '{')) {
          // we need to jump
          npos = (current_val & JSON_VALUE_MASK);
        } else {
          npos =
              location + ((current_type == 'd' || current_type == 'l') ? 2 : 1);
        }
        location = npos;
        current_val = pj.tape[npos];
        current_type = (current_val >> 56);
        return true; // how could it fail ?
      }
    } else { // regular numeric index
      // The index can't have a leading '0'
      if (key_or_index[0] == '0' && key_or_index.length() > 1) {
        return false;
      }
      // it cannot be empty
      if (key_or_index.length() == 0) {
        return false;
      }
      // we already checked the index contains only valid digits
      uint32_t index = std::stoi(key_or_index);
      if (move_to_index(index)) {
        found = relative_move_to(pointer + offset, length - offset);
      }
    }
  }

  return found;
}
} // namespace simdjson
/* end file src/parsedjsoniterator.cpp */
