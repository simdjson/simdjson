/* auto-generated on Sun Oct  6 13:23:03 DST 2019. Do not edit! */
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
    {UNESCAPED_CHARS, "Within strings, some characters must be escaped, we "
                      "found unescaped characters"},
    {UNCLOSED_STRING, "A string is opened, but never closed."},
    {UNEXPECTED_ERROR, "Unexpected error, consider reporting this problem as "
                       "you may have found a bug in simdjson"},
};

// string returned when the error code is not recognized
const std::string unexpected_error_msg {"Unexpected error"};

// returns a string matching the error code
const std::string &error_message(const int error_code) {
  auto keyvalue = error_strings.find(error_code);
  if(keyvalue == error_strings.end()) {
    return unexpected_error_msg;
  }
  return keyvalue->second;
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
#include <atomic>

namespace simdjson {

// The function that users are expected to call is json_parse.
// We have more than one such function because we want to support several
// instruction sets.

// function pointer type for json_parse
using json_parse_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj,
                                bool realloc);

// Pointer that holds the json_parse implementation corresponding to the
// available SIMD instruction set
extern std::atomic<json_parse_functype *> json_parse_ptr;

int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj,
               bool realloc) {
  return json_parse_ptr.load(std::memory_order_relaxed)(buf, len, pj, realloc);
}

int json_parse(const char *buf, size_t len, ParsedJson &pj,
               bool realloc) {
  return json_parse_ptr.load(std::memory_order_relaxed)(reinterpret_cast<const uint8_t *>(buf), len, pj,
                                                        realloc);
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
                        bool realloc) {
  Architecture best_implementation = find_best_supported_implementation();
  // Selecting the best implementation
  switch (best_implementation) {
#ifdef IS_X86_64
  case Architecture::HASWELL:
    json_parse_ptr.store(&json_parse_implementation<Architecture::HASWELL>, std::memory_order_relaxed);
    break;
  case Architecture::WESTMERE:
    json_parse_ptr.store(&json_parse_implementation<Architecture::WESTMERE>, std::memory_order_relaxed);
    break;
#endif
#ifdef IS_ARM64
  case Architecture::ARM64:
    json_parse_ptr.store(&json_parse_implementation<Architecture::ARM64>, std::memory_order_relaxed);
    break;
#endif
  default:
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    return simdjson::UNEXPECTED_ERROR;
  }

  return json_parse_ptr.load(std::memory_order_relaxed)(buf, len, pj, realloc);
}

std::atomic<json_parse_functype *> json_parse_ptr = &json_parse_dispatch;

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len,
                             bool realloc) {
  ParsedJson pj;
  bool ok = pj.allocate_capacity(len);
  if (ok) {
    json_parse(buf, len, pj, realloc);
  } else {
    std::cerr << "failure during memory allocation " << std::endl;
  }
  return pj;
}
} // namespace simdjson
/* end file src/jsonparser.cpp */
/* begin file src/arm64/simd_input.h */
#ifndef SIMDJSON_ARM64_SIMD_INPUT_H
#define SIMDJSON_ARM64_SIMD_INPUT_H


#ifdef IS_ARM64

namespace simdjson::arm64 {

really_inline uint16_t neon_movemask(uint8x16_t input) {
  const uint8x16_t bit_mask = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                              0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  uint8x16_t minput = vandq_u8(input, bit_mask);
  uint8x16_t tmp = vpaddq_u8(minput, minput);
  tmp = vpaddq_u8(tmp, tmp);
  tmp = vpaddq_u8(tmp, tmp);
  return vgetq_lane_u16(vreinterpretq_u16_u8(tmp), 0);
}

really_inline uint64_t neon_movemask_bulk(uint8x16_t p0, uint8x16_t p1,
                                          uint8x16_t p2, uint8x16_t p3) {
  const uint8x16_t bit_mask = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                              0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  uint8x16_t t0 = vandq_u8(p0, bit_mask);
  uint8x16_t t1 = vandq_u8(p1, bit_mask);
  uint8x16_t t2 = vandq_u8(p2, bit_mask);
  uint8x16_t t3 = vandq_u8(p3, bit_mask);
  uint8x16_t sum0 = vpaddq_u8(t0, t1);
  uint8x16_t sum1 = vpaddq_u8(t2, t3);
  sum0 = vpaddq_u8(sum0, sum1);
  sum0 = vpaddq_u8(sum0, sum0);
  return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
}

struct simd_input {
  const uint8x16_t chunks[4];

  really_inline simd_input()
    : chunks{uint8x16_t(), uint8x16_t(), uint8x16_t(), uint8x16_t() } {}

  really_inline simd_input(const uint8x16_t chunk0, const uint8x16_t chunk1, const uint8x16_t chunk2, const uint8x16_t chunk3)
    : chunks{chunk0, chunk1, chunk2, chunk3 } {}

  really_inline simd_input(const uint8_t *ptr)
      : chunks{
        vld1q_u8(ptr + 0*16),
        vld1q_u8(ptr + 1*16),
        vld1q_u8(ptr + 2*16),
        vld1q_u8(ptr + 3*16)
       } {}

  template <typename F>
  really_inline void each(F const& each_chunk) const {
    each_chunk(this->chunks[0]);
    each_chunk(this->chunks[1]);
    each_chunk(this->chunks[2]);
    each_chunk(this->chunks[3]);
  }

  template <typename F>
  really_inline simd_input map(F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0]),
      map_chunk(this->chunks[1]),
      map_chunk(this->chunks[2]),
      map_chunk(this->chunks[3])
    );
  }

  template <typename F>
  really_inline simd_input map(simd_input b, F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0], b.chunks[0]),
      map_chunk(this->chunks[1], b.chunks[1]),
      map_chunk(this->chunks[2], b.chunks[2]),
      map_chunk(this->chunks[3], b.chunks[3])
    );
  }

  template <typename F>
  really_inline uint8x16_t reduce(F const& reduce_pair) const {
    uint8x16_t r01 = reduce_pair(this->chunks[0], this->chunks[1]);
    uint8x16_t r23 = reduce_pair(this->chunks[2], this->chunks[3]);
    return reduce_pair(r01, r23);
  }

  really_inline uint64_t to_bitmask() const {
    return neon_movemask_bulk(this->chunks[0], this->chunks[1], this->chunks[2], this->chunks[3]);
  }

  really_inline simd_input bit_or(const uint8_t m) const {
    const uint8x16_t mask = vmovq_n_u8(m);
    return this->map( [&](auto a) {
      return vorrq_u8(a, mask);
    });
  }

  really_inline uint64_t eq(const uint8_t m) const {
    const uint8x16_t mask = vmovq_n_u8(m);
    return this->map( [&](auto a) {
      return vceqq_u8(a, mask);
    }).to_bitmask();
  }

  really_inline uint64_t lteq(const uint8_t m) const {
    const uint8x16_t mask = vmovq_n_u8(m);
    return this->map( [&](auto a) {
      return vcleq_u8(a, mask);
    }).to_bitmask();
  }

}; // struct simd_input

} // namespace simdjson::arm64

#endif // IS_ARM64
#endif // SIMDJSON_ARM64_SIMD_INPUT_H
/* end file src/arm64/simd_input.h */
/* begin file src/haswell/simd_input.h */
#ifndef SIMDJSON_HASWELL_SIMD_INPUT_H
#define SIMDJSON_HASWELL_SIMD_INPUT_H


#ifdef IS_X86_64

TARGET_HASWELL
namespace simdjson::haswell {

struct simd_input {
  const __m256i chunks[2];

  really_inline simd_input() : chunks{__m256i(), __m256i()} {}

  really_inline simd_input(const __m256i chunk0, const __m256i chunk1)
      : chunks{chunk0, chunk1} {}

  really_inline simd_input(const uint8_t *ptr)
      : chunks{
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 0*32)),
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 1*32))
      } {}

  template <typename F>
  really_inline void each(F const& each_chunk) const
  {
    each_chunk(this->chunks[0]);
    each_chunk(this->chunks[1]);
  }

  template <typename F>
  really_inline simd_input map(F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0]),
      map_chunk(this->chunks[1])
    );
  }

  template <typename F>
  really_inline simd_input map(const simd_input b, F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0], b.chunks[0]),
      map_chunk(this->chunks[1], b.chunks[1])
    );
  }

  template <typename F>
  really_inline __m256i reduce(F const& reduce_pair) const {
    return reduce_pair(this->chunks[0], this->chunks[1]);
  }

  really_inline uint64_t to_bitmask() const {
    uint64_t r_lo = static_cast<uint32_t>(_mm256_movemask_epi8(this->chunks[0]));
    uint64_t r_hi =                       _mm256_movemask_epi8(this->chunks[1]);
    return r_lo | (r_hi << 32);
  }

  really_inline simd_input bit_or(const uint8_t m) const {
    const __m256i mask = _mm256_set1_epi8(m);
    return this->map( [&](auto a) {
      return _mm256_or_si256(a, mask);
    });
  }

  really_inline uint64_t eq(const uint8_t m) const {
    const __m256i mask = _mm256_set1_epi8(m);
    return this->map( [&](auto a) {
      return _mm256_cmpeq_epi8(a, mask);
    }).to_bitmask();
  }

  really_inline uint64_t lteq(const uint8_t m) const {
    const __m256i maxval = _mm256_set1_epi8(m);
    return this->map( [&](auto a) {
      return _mm256_cmpeq_epi8(_mm256_max_epu8(maxval, a), maxval);
    }).to_bitmask();
  }

}; // struct simd_input

} // namespace simdjson::haswell
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_HASWELL_SIMD_INPUT_H
/* end file src/haswell/simd_input.h */
/* begin file src/westmere/simd_input.h */
#ifndef SIMDJSON_WESTMERE_SIMD_INPUT_H
#define SIMDJSON_WESTMERE_SIMD_INPUT_H


#ifdef IS_X86_64

TARGET_WESTMERE
namespace simdjson::westmere {

struct simd_input {
  const __m128i chunks[4];

  really_inline simd_input()
      : chunks { __m128i(), __m128i(), __m128i(), __m128i() } {}

  really_inline simd_input(const __m128i chunk0, const __m128i chunk1, const __m128i chunk2, const __m128i chunk3)
      : chunks{chunk0, chunk1, chunk2, chunk3} {}

  really_inline simd_input(const uint8_t *ptr)
      : simd_input(
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 0)),
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 16)),
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 32)),
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 48))
      ) {}

  template <typename F>
  really_inline void each(F const& each_chunk) const {
    each_chunk(this->chunks[0]);
    each_chunk(this->chunks[1]);
    each_chunk(this->chunks[2]);
    each_chunk(this->chunks[3]);
  }

  template <typename F>
  really_inline simd_input map(F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0]),
      map_chunk(this->chunks[1]),
      map_chunk(this->chunks[2]),
      map_chunk(this->chunks[3])
    );
  }

  template <typename F>
  really_inline simd_input map(const simd_input b, F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0], b.chunks[0]),
      map_chunk(this->chunks[1], b.chunks[1]),
      map_chunk(this->chunks[2], b.chunks[2]),
      map_chunk(this->chunks[3], b.chunks[3])
    );
  }

  template <typename F>
  really_inline __m128i reduce(F const& reduce_pair) const {
    __m128i r01 = reduce_pair(this->chunks[0], this->chunks[1]);
    __m128i r23 = reduce_pair(this->chunks[2], this->chunks[3]);
    return reduce_pair(r01, r23);
  }

  really_inline uint64_t to_bitmask() const {
    uint64_t r0 = static_cast<uint32_t>(_mm_movemask_epi8(this->chunks[0]));
    uint64_t r1 =                       _mm_movemask_epi8(this->chunks[1]);
    uint64_t r2 =                       _mm_movemask_epi8(this->chunks[2]);
    uint64_t r3 =                       _mm_movemask_epi8(this->chunks[3]);
    return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
  }

  really_inline simd_input bit_or(const uint8_t m) const {
    const __m128i mask = _mm_set1_epi8(m);
    return this->map( [&](auto a) {
      return _mm_or_si128(a, mask);
    });
  }

  really_inline uint64_t eq(const uint8_t m) const {
    const __m128i mask = _mm_set1_epi8(m);
    return this->map( [&](auto a) {
      return _mm_cmpeq_epi8(a, mask);
    }).to_bitmask();
  }

  really_inline uint64_t lteq(const uint8_t m) const {
    const __m128i maxval = _mm_set1_epi8(m);
    return this->map( [&](auto a) {
      return _mm_cmpeq_epi8(_mm_max_epu8(maxval, a), maxval);
    }).to_bitmask();
  }

}; // struct simd_input

} // namespace simdjson::westmere
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_WESTMERE_SIMD_INPUT_H
/* end file src/westmere/simd_input.h */
/* begin file src/arm64/simdutf8check.h */
// From https://github.com/cyb70289/utf8/blob/master/lemire-neon.c
// Adapted from https://github.com/lemire/fastvalidate-utf-8

#ifndef SIMDJSON_ARM64_SIMDUTF8CHECK_H
#define SIMDJSON_ARM64_SIMDUTF8CHECK_H

// TODO this is different from IS_ARM64 in portability.h, which we use in other places ...
#if defined(_ARM_NEON) || defined(__aarch64__) ||                              \
    (defined(_MSC_VER) && defined(_M_ARM64))

#include <arm_neon.h>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

/*
 * legal utf-8 byte sequence
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 *  Code Points        1st       2s       3s       4s
 * U+0000..U+007F     00..7F
 * U+0080..U+07FF     C2..DF   80..BF
 * U+0800..U+0FFF     E0       A0..BF   80..BF
 * U+1000..U+CFFF     E1..EC   80..BF   80..BF
 * U+D000..U+D7FF     ED       80..9F   80..BF
 * U+E000..U+FFFF     EE..EF   80..BF   80..BF
 * U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
 * U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
 * U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
 *
 */
namespace simdjson::arm64 {

static const int8_t _nibbles[] = {
    1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
    0, 0, 0, 0,             // 10xx (continuation)
    2, 2,                   // 110x
    3,                      // 1110
    4,                      // 1111, next should be 0 (not checked here)
};

static const int8_t _initial_mins[] = {
    -128,         -128, -128, -128, -128, -128,
    -128,         -128, -128, -128, -128, -128, // 10xx => false
    (int8_t)0xC2, -128,                         // 110x
    (int8_t)0xE1,                               // 1110
    (int8_t)0xF1,
};

static const int8_t _second_mins[] = {
    -128,         -128, -128, -128, -128, -128,
    -128,         -128, -128, -128, -128, -128, // 10xx => false
    127,          127,                          // 110x => true
    (int8_t)0xA0,                               // 1110
    (int8_t)0x90,
};

struct processed_utf_bytes {
  int8x16_t raw_bytes;
  int8x16_t high_nibbles;
  int8x16_t carried_continuations;
};

struct utf8_checker {
  int8x16_t has_error{};
  processed_utf_bytes previous{};

  really_inline void add_errors(uint8x16_t errors) {
    this->has_error = vorrq_s8(this->has_error, errors);
  }

  // all byte values must be no larger than 0xF4
  really_inline void check_smaller_than_0xF4(int8x16_t current_bytes) {
    // unsigned, saturates to 0 below max
    this->has_error = vreinterpretq_s8_u8(vqsubq_u8(
                        vreinterpretq_u8_s8(current_bytes), vdupq_n_u8(0xF4)));
  }

  really_inline int8x16_t continuation_lengths(int8x16_t high_nibbles) {
    return vqtbl1q_s8(vld1q_s8(_nibbles), vreinterpretq_u8_s8(high_nibbles));
  }

  really_inline int8x16_t carry_continuations(int8x16_t initial_lengths,
                                              int8x16_t previous_carries) {
    int8x16_t right1 = vreinterpretq_s8_u8(vqsubq_u8(
        vreinterpretq_u8_s8(vextq_s8(previous_carries, initial_lengths, 16 - 1)),
        vdupq_n_u8(1)));
    int8x16_t sum = vaddq_s8(initial_lengths, right1);

    int8x16_t right2 = vreinterpretq_s8_u8(
        vqsubq_u8(vreinterpretq_u8_s8(vextq_s8(previous_carries, sum, 16 - 2)),
                  vdupq_n_u8(2)));
    return vaddq_s8(sum, right2);
  }

  really_inline void check_continuations(int8x16_t initial_lengths,
                                         int8x16_t carries) {

    // overlap || underlap
    // carry > length && length > 0 || !(carry > length) && !(length > 0)
    // (carries > length) == (lengths > 0)
    uint8x16_t overunder = vceqq_u8(vcgtq_s8(carries, initial_lengths),
                                    vcgtq_s8(initial_lengths, vdupq_n_s8(0)));

    this->add_errors( vreinterpretq_s8_u8(overunder) );
  }

  // when 0xED is found, next byte must be no larger than 0x9F
  // when 0xF4 is found, next byte must be no larger than 0x8F
  // next byte must be continuation, ie sign bit is set, so signed < is ok
  really_inline void check_first_continuation_max(int8x16_t current_bytes,
                                                  int8x16_t off1_current_bytes) {
    uint8x16_t maskED = vceqq_s8(off1_current_bytes, vdupq_n_s8(0xED));
    uint8x16_t maskF4 = vceqq_s8(off1_current_bytes, vdupq_n_s8(0xF4));

    uint8x16_t badfollowED =
        vandq_u8(vcgtq_s8(current_bytes, vdupq_n_s8(0x9F)), maskED);
    uint8x16_t badfollowF4 =
        vandq_u8(vcgtq_s8(current_bytes, vdupq_n_s8(0x8F)), maskF4);

    this->add_errors( vreinterpretq_s8_u8(vorrq_u8(badfollowED, badfollowF4)) );
  }

  // map off1_hibits => error condition
  // hibits     off1    cur
  // C       => < C2 && true
  // E       => < E1 && < A0
  // F       => < F1 && < 90
  // else      false && false
  really_inline void check_overlong(int8x16_t current_bytes,
                                    int8x16_t off1_current_bytes,
                                    int8x16_t high_nibbles) {
    int8x16_t off1_hibits = vextq_s8(this->previous.high_nibbles, high_nibbles, 16 - 1);
    int8x16_t initial_mins =
        vqtbl1q_s8(vld1q_s8(_initial_mins), vreinterpretq_u8_s8(off1_hibits));

    uint8x16_t initial_under = vcgtq_s8(initial_mins, off1_current_bytes);

    int8x16_t second_mins =
        vqtbl1q_s8(vld1q_s8(_second_mins), vreinterpretq_u8_s8(off1_hibits));
    uint8x16_t second_under = vcgtq_s8(second_mins, current_bytes);
    this->add_errors( vreinterpretq_s8_u8(vandq_u8(initial_under, second_under)) );
  }

  really_inline void count_nibbles(int8x16_t bytes, struct processed_utf_bytes *answer) {
    answer->raw_bytes = bytes;
    answer->high_nibbles = vreinterpretq_s8_u8(vshrq_n_u8(vreinterpretq_u8_s8(bytes), 4));
  }

  // check whether the current bytes are valid UTF-8
  // at the end of the function, previous gets updated
  really_inline void check_utf8_bytes(int8x16_t current_bytes) {
    struct processed_utf_bytes pb;
    this->count_nibbles(current_bytes, &pb);

    this->check_smaller_than_0xF4(current_bytes);

    int8x16_t initial_lengths = this->continuation_lengths(pb.high_nibbles);

    pb.carried_continuations = this->carry_continuations(initial_lengths);

    this->check_continuations(initial_lengths, pb.carried_continuations);

    int8x16_t off1_current_bytes =
        vextq_s8(this->previous.raw_bytes, pb.raw_bytes, 16 - 1);
    this->check_first_continuation_max(current_bytes, off1_current_bytes);

    this->check_overlong(current_bytes, off1_current_bytes, pb.high_nibbles);
    this->previous = pb;
  }

  // Checks that all bytes are ascii
  really_inline bool check_ascii_neon(simd_input in) {
    // checking if the most significant bit is always equal to 0.
    uint8x16_t high_bit = vdupq_n_u8(0x80);
    uint8x16_t any_bits_on = in.reduce([&](auto a, auto b) {
      return vorrq_u8(a, b);
    });
    uint8x16_t high_bit_on = vandq_u8(any_bits_on, high_bit);
    uint64x2_t v64 = vreinterpretq_u64_u8(high_bit_on);
    uint32x2_t v32 = vqmovn_u64(v64);
    uint64x1_t result = vreinterpret_u64_u32(v32);
    return vget_lane_u64(result, 0) == 0;
  }

  really_inline void check_carried_continuations() {
      const int8x16_t verror =
          (int8x16_t){9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1};
      this->add_errors(
        vreinterpretq_s8_u8(vcgtq_s8(this->previous.carried_continuations, verror))
      );
  }

  really_inline void check_next_input(uint8x16_t in) {
    if (this->check_ascii_neon(in)) {
      // All bytes are ascii. Therefore the byte that was just before must be
      // ascii too. We only check the byte that was just before simd_input. Nines
      // are arbitrary values.
      this->check_carried_continuations();
    } else {
      // it is not ascii so we have to do heavy work
      this->check_utf8_bytes(vreinterpretq_s8_u8(in));
    }
  }

  really_inline void check_next_input(simd_input in) {
    if (this->check_ascii_neon(in)) {
      // All bytes are ascii. Therefore the byte that was just before must be
      // ascii too. We only check the byte that was just before simd_input. Nines
      // are arbitrary values.
      this->check_carried_continuations();
    } else {
      // it is not ascii so we have to do heavy work
      in.each([&](auto _in) { this->check_utf8_bytes(vreinterpretq_s8_u8(_in)); });
    }
  }

  really_inline ErrorValues errors() {
    uint64x2_t v64 = vreinterpretq_u64_s8(this->has_error);
    uint32x2_t v32 = vqmovn_u64(v64);
    uint64x1_t result = vreinterpret_u64_u32(v32);
    return vget_lane_u64(result, 0) != 0 ? simdjson::UTF8_ERROR
                                        : simdjson::SUCCESS;
  }

}; // struct utf8_checker

} // namespace simdjson::arm64

#endif // ARM_NEON
#endif // SIMDJSON_ARM64_SIMDUTF8CHECK_H
/* end file src/arm64/simdutf8check.h */
/* begin file src/haswell/simdutf8check.h */
#ifndef SIMDJSON_HASWELL_SIMDUTF8CHECK_H
#define SIMDJSON_HASWELL_SIMDUTF8CHECK_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef IS_X86_64
/*
 * legal utf-8 byte sequence
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 *  Code Points        1st       2s       3s       4s
 * U+0000..U+007F     00..7F
 * U+0080..U+07FF     C2..DF   80..BF
 * U+0800..U+0FFF     E0       A0..BF   80..BF
 * U+1000..U+CFFF     E1..EC   80..BF   80..BF
 * U+D000..U+D7FF     ED       80..9F   80..BF
 * U+E000..U+FFFF     EE..EF   80..BF   80..BF
 * U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
 * U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
 * U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
 *
 */

// all byte values must be no larger than 0xF4

TARGET_HASWELL
namespace simdjson::haswell {

static inline __m256i push_last_byte_of_a_to_b(__m256i a, __m256i b) {
  return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 15);
}

static inline __m256i push_last_2bytes_of_a_to_b(__m256i a, __m256i b) {
  return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 14);
}

struct processed_utf_bytes {
  __m256i raw_bytes;
  __m256i high_nibbles;
  __m256i carried_continuations;
};

struct utf8_checker {
  __m256i has_error;
  processed_utf_bytes previous;

  utf8_checker() {
    has_error = _mm256_setzero_si256();
    previous.raw_bytes = _mm256_setzero_si256();
    previous.high_nibbles = _mm256_setzero_si256();
    previous.carried_continuations = _mm256_setzero_si256();
  }

  really_inline void add_errors(__m256i errors) {
    this->has_error = _mm256_or_si256(this->has_error, errors);
  }

  // all byte values must be no larger than 0xF4
  really_inline void check_smaller_than_0xF4(__m256i current_bytes) {
    // unsigned, saturates to 0 below max
    this->add_errors( _mm256_subs_epu8(current_bytes, _mm256_set1_epi8(0xF4u)) );
  }

  really_inline __m256i continuation_lengths(__m256i high_nibbles) {
    return _mm256_shuffle_epi8(
        _mm256_setr_epi8(1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
                         0, 0, 0, 0,             // 10xx (continuation)
                         2, 2,                   // 110x
                         3,                      // 1110
                         4, // 1111, next should be 0 (not checked here)
                         1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
                         0, 0, 0, 0,             // 10xx (continuation)
                         2, 2,                   // 110x
                         3,                      // 1110
                         4), // 1111, next should be 0 (not checked here)
                         
        high_nibbles);
  }

  really_inline __m256i carry_continuations(__m256i initial_lengths) {
    __m256i right1 = _mm256_subs_epu8(
        push_last_byte_of_a_to_b(this->previous.carried_continuations, initial_lengths),
        _mm256_set1_epi8(1));
    __m256i sum = _mm256_add_epi8(initial_lengths, right1);

    __m256i right2 = _mm256_subs_epu8(
        push_last_2bytes_of_a_to_b(this->previous.carried_continuations, sum), _mm256_set1_epi8(2));
    return _mm256_add_epi8(sum, right2);
  }

  really_inline void check_continuations(__m256i initial_lengths,
                                             __m256i carries) {

    // overlap || underlap
    // carry > length && length > 0 || !(carry > length) && !(length > 0)
    // (carries > length) == (lengths > 0)
    __m256i overunder = _mm256_cmpeq_epi8(
        _mm256_cmpgt_epi8(carries, initial_lengths),
        _mm256_cmpgt_epi8(initial_lengths, _mm256_setzero_si256()));

    this->add_errors( overunder );
  }

  really_inline void check_carried_continuations() {
    this->add_errors(
      _mm256_cmpgt_epi8(this->previous.carried_continuations,
      _mm256_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                       9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                       9, 9, 9, 9, 9, 9, 9, 1))
    );
  }

  // when 0xED is found, next byte must be no larger than 0x9F
  // when 0xF4 is found, next byte must be no larger than 0x8F
  // next byte must be continuation, ie sign bit is set, so signed < is ok
  really_inline void check_first_continuation_max(__m256i current_bytes,
                                                      __m256i off1_current_bytes) {
    __m256i maskED =
        _mm256_cmpeq_epi8(off1_current_bytes, _mm256_set1_epi8(0xEDu));
    __m256i maskF4 =
        _mm256_cmpeq_epi8(off1_current_bytes, _mm256_set1_epi8(0xF4u));

    __m256i badfollowED = _mm256_and_si256(
        _mm256_cmpgt_epi8(current_bytes, _mm256_set1_epi8(0x9Fu)), maskED);
    __m256i badfollowF4 = _mm256_and_si256(
        _mm256_cmpgt_epi8(current_bytes, _mm256_set1_epi8(0x8Fu)), maskF4);

    this->add_errors( _mm256_or_si256(badfollowED, badfollowF4) );
  }

  // map off1_hibits => error condition
  // hibits     off1    cur
  // C       => < C2 && true
  // E       => < E1 && < A0
  // F       => < F1 && < 90
  // else      false && false
  really_inline void check_overlong(__m256i current_bytes,
                                        __m256i off1_current_bytes,
                                        __m256i high_nibbles) {
    __m256i off1_high_nibbles = push_last_byte_of_a_to_b(this->previous.high_nibbles, high_nibbles);
    __m256i initial_mins = _mm256_shuffle_epi8(
        _mm256_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128,
                         -128, -128, -128, // 10xx => false
                         0xC2u, -128,      // 110x
                         0xE1u,            // 1110
                         0xF1u,            // 1111
                         -128, -128, -128, -128, -128, -128, -128, -128, -128,
                         -128, -128, -128, // 10xx => false
                         0xC2u, -128,      // 110x
                         0xE1u,            // 1110
                         0xF1u),           // 1111
        off1_high_nibbles);

    __m256i initial_under = _mm256_cmpgt_epi8(initial_mins, off1_current_bytes);

    __m256i second_mins = _mm256_shuffle_epi8(
        _mm256_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128,
                         -128, -128, -128, // 10xx => false
                         127, 127,         // 110x => true
                         0xA0u,            // 1110
                         0x90u,            // 1111
                         -128, -128, -128, -128, -128, -128, -128, -128, -128,
                         -128, -128, -128, // 10xx => false
                         127, 127,         // 110x => true
                         0xA0u,            // 1110
                         0x90u),           // 1111
        off1_high_nibbles);
    __m256i second_under = _mm256_cmpgt_epi8(second_mins, current_bytes);
    this->add_errors( _mm256_and_si256(initial_under, second_under) );
  }

  really_inline void count_nibbles(__m256i bytes, struct processed_utf_bytes *answer) {
    answer->raw_bytes = bytes;
    answer->high_nibbles = _mm256_and_si256(_mm256_srli_epi16(bytes, 4), _mm256_set1_epi8(0x0F));
  }

  // check whether the current bytes are valid UTF-8
  // at the end of the function, previous gets updated
  really_inline void check_utf8_bytes(__m256i current_bytes) {
    struct processed_utf_bytes pb {};
    this->count_nibbles(current_bytes, &pb);

    this->check_smaller_than_0xF4(current_bytes);

    __m256i initial_lengths = this->continuation_lengths(pb.high_nibbles);

    pb.carried_continuations = this->carry_continuations(initial_lengths);

    this->check_continuations(initial_lengths, pb.carried_continuations);

    __m256i off1_current_bytes =
        push_last_byte_of_a_to_b(this->previous.raw_bytes, pb.raw_bytes);
    this->check_first_continuation_max(current_bytes, off1_current_bytes);

    this->check_overlong(current_bytes, off1_current_bytes, pb.high_nibbles);
    this->previous = pb;
  }

  really_inline void check_next_input(__m256i in) {
    __m256i high_bit = _mm256_set1_epi8(0x80u);
    if (likely(_mm256_testz_si256(in, high_bit) == 1)) {
      this->check_carried_continuations();
    } else {
      this->check_utf8_bytes(in);
    }
  }

  really_inline void check_next_input(simd_input in) {
    __m256i high_bit = _mm256_set1_epi8(0x80u);
    __m256i any_bits_on = in.reduce([&](auto a, auto b) {
      return _mm256_or_si256(a, b);
    });
    if (likely(_mm256_testz_si256(any_bits_on, high_bit) == 1)) {
      // it is ascii, we just check carried continuations.
      this->check_carried_continuations();
    } else {
      // it is not ascii so we have to do heavy work
      in.each([&](auto _in) { check_utf8_bytes(_in); });
    }
  }

  really_inline ErrorValues errors() {
    return _mm256_testz_si256(this->has_error, this->has_error) == 0
              ? simdjson::UTF8_ERROR
              : simdjson::SUCCESS;
  }
}; // struct utf8_checker

}; // namespace simdjson::haswell
UNTARGET_REGION // haswell

#endif // IS_X86_64

#endif // SIMDJSON_HASWELL_SIMDUTF8CHECK_H
/* end file src/haswell/simdutf8check.h */
/* begin file src/westmere/simdutf8check.h */
#ifndef SIMDJSON_WESTMERE_SIMDUTF8CHECK_H
#define SIMDJSON_WESTMERE_SIMDUTF8CHECK_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef IS_X86_64

/*
 * legal utf-8 byte sequence
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 *  Code Points        1st       2s       3s       4s
 * U+0000..U+007F     00..7F
 * U+0080..U+07FF     C2..DF   80..BF
 * U+0800..U+0FFF     E0       A0..BF   80..BF
 * U+1000..U+CFFF     E1..EC   80..BF   80..BF
 * U+D000..U+D7FF     ED       80..9F   80..BF
 * U+E000..U+FFFF     EE..EF   80..BF   80..BF
 * U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
 * U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
 * U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
 *
 */

// all byte values must be no larger than 0xF4

/********** sse code **********/
TARGET_WESTMERE
namespace simdjson::westmere {

struct processed_utf_bytes {
  __m128i raw_bytes;
  __m128i high_nibbles;
  __m128i carried_continuations;
};

struct utf8_checker {
  __m128i has_error = _mm_setzero_si128();
  processed_utf_bytes previous{
      _mm_setzero_si128(), // raw_bytes
      _mm_setzero_si128(), // high_nibbles
      _mm_setzero_si128()  // carried_continuations
  };

  really_inline void add_errors(__m128i errors) {
    this->has_error = _mm_or_si128(errors, this->has_error);
  }

  // all byte values must be no larger than 0xF4
  really_inline void check_smaller_than_0xF4(__m128i current_bytes) {
    // unsigned, saturates to 0 below max
    this->add_errors( _mm_subs_epu8(current_bytes, _mm_set1_epi8(0xF4u)) );
  }

  really_inline __m128i continuation_lengths(__m128i high_nibbles) {
    return _mm_shuffle_epi8(
        _mm_setr_epi8(1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
                      0, 0, 0, 0,             // 10xx (continuation)
                      2, 2,                   // 110x
                      3,                      // 1110
                      4), // 1111, next should be 0 (not checked here)
        high_nibbles);
  }

  really_inline __m128i carry_continuations(__m128i initial_lengths) {

    __m128i right1 =
        _mm_subs_epu8(_mm_alignr_epi8(initial_lengths, this->previous.carried_continuations, 16 - 1),
                      _mm_set1_epi8(1));
    __m128i sum = _mm_add_epi8(initial_lengths, right1);

    __m128i right2 = _mm_subs_epu8(_mm_alignr_epi8(sum, this->previous.carried_continuations, 16 - 2),
                                  _mm_set1_epi8(2));
    return _mm_add_epi8(sum, right2);
  }

  really_inline void check_continuations(__m128i initial_lengths, __m128i carries) {

    // overlap || underlap
    // carry > length && length > 0 || !(carry > length) && !(length > 0)
    // (carries > length) == (lengths > 0)
    __m128i overunder =
        _mm_cmpeq_epi8(_mm_cmpgt_epi8(carries, initial_lengths),
                      _mm_cmpgt_epi8(initial_lengths, _mm_setzero_si128()));

    this->add_errors( overunder );
  }

  // when 0xED is found, next byte must be no larger than 0x9F
  // when 0xF4 is found, next byte must be no larger than 0x8F
  // next byte must be continuation, ie sign bit is set, so signed < is ok
  really_inline void check_first_continuation_max(__m128i current_bytes, __m128i off1_current_bytes) {
    __m128i maskED = _mm_cmpeq_epi8(off1_current_bytes, _mm_set1_epi8(0xEDu));
    __m128i maskF4 = _mm_cmpeq_epi8(off1_current_bytes, _mm_set1_epi8(0xF4u));

    __m128i badfollowED = _mm_and_si128(
        _mm_cmpgt_epi8(current_bytes, _mm_set1_epi8(0x9Fu)), maskED);
    __m128i badfollowF4 = _mm_and_si128(
        _mm_cmpgt_epi8(current_bytes, _mm_set1_epi8(0x8Fu)), maskF4);

    this->add_errors( _mm_or_si128(badfollowED, badfollowF4) );
  }

  // map off1_hibits => error condition
  // hibits     off1    cur
  // C       => < C2 && true
  // E       => < E1 && < A0
  // F       => < F1 && < 90
  // else      false && false
  really_inline void check_overlong(__m128i current_bytes,
                                    __m128i off1_current_bytes, __m128i high_nibbles) {
    __m128i off1_hibits = _mm_alignr_epi8(high_nibbles, this->previous.high_nibbles, 16 - 1);
    __m128i initial_mins = _mm_shuffle_epi8(
        _mm_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
                      -128, -128,  // 10xx => false
                      0xC2u, -128, // 110x
                      0xE1u,       // 1110
                      0xF1u),
        off1_hibits);

    __m128i initial_under = _mm_cmpgt_epi8(initial_mins, off1_current_bytes);

    __m128i second_mins = _mm_shuffle_epi8(
        _mm_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
                      -128, -128, // 10xx => false
                      127, 127,   // 110x => true
                      0xA0u,      // 1110
                      0x90u),
        off1_hibits);
    __m128i second_under = _mm_cmpgt_epi8(second_mins, current_bytes);
    this->add_errors( _mm_and_si128(initial_under, second_under) );
  }

  really_inline void count_nibbles(__m128i bytes, struct processed_utf_bytes *answer) {
    answer->raw_bytes = bytes;
    answer->high_nibbles = _mm_and_si128(_mm_srli_epi16(bytes, 4), _mm_set1_epi8(0x0F));
  }

  // check whether the current bytes are valid UTF-8
  // at the end of the function, previous gets updated
  really_inline void check_utf8_bytes(__m128i current_bytes) {
    struct processed_utf_bytes pb;
    this->count_nibbles(current_bytes, &pb);

    this->check_smaller_than_0xF4(current_bytes);

    __m128i initial_lengths = this->continuation_lengths(pb.high_nibbles);

    pb.carried_continuations = this->carry_continuations(initial_lengths);

    this->check_continuations(initial_lengths, pb.carried_continuations);

    __m128i off1_current_bytes =
        _mm_alignr_epi8(pb.raw_bytes, this->previous.raw_bytes, 16 - 1);
    this->check_first_continuation_max(current_bytes, off1_current_bytes);

    this->check_overlong(current_bytes, off1_current_bytes, pb.high_nibbles);
    this->previous = pb;
  }

  really_inline void check_carried_continuations() {
      this->has_error = _mm_cmpgt_epi8(this->previous.carried_continuations,
                                       _mm_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                                     9, 9, 9, 9, 9, 1));
  }

  really_inline void check_next_input(__m128i in) {
    __m128i high_bit = _mm_set1_epi8(0x80u);
    if (_mm_testz_si128( in, high_bit) == 1) {
      // it is ascii, we just check continuations
      this->check_carried_continuations();
    } else {
      // it is not ascii so we have to do heavy work
      this->check_utf8_bytes(in);
    }
  }

  really_inline void check_next_input(simd_input in) {
    __m128i high_bit = _mm_set1_epi8(0x80u);
    __m128i any_bits_on = in.reduce([&](auto a, auto b) {
      return _mm_or_si128(a, b);
    });
    if (_mm_testz_si128(any_bits_on, high_bit) == 1) {
      // it is ascii, we just check continuations
      this->check_carried_continuations();
    } else {
      // it is not ascii so we have to do heavy work
      in.each([&](auto _in) { this->check_utf8_bytes(_in); });
    }
  }

  really_inline ErrorValues errors() {
    return _mm_testz_si128(this->has_error, this->has_error) == 0
              ? simdjson::UTF8_ERROR
              : simdjson::SUCCESS;
  }

}; // struct utf8_checker

} // namespace simdjson::westmere
UNTARGET_REGION // westmere

#endif // IS_X86_64

#endif
/* end file src/westmere/simdutf8check.h */
/* begin file src/arm64/stage1_find_marks.h */
#ifndef SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
#define SIMDJSON_ARM64_STAGE1_FIND_MARKS_H


#ifdef IS_ARM64


namespace simdjson::arm64 {

really_inline uint64_t compute_quote_mask(const uint64_t quote_bits) {

#ifdef __ARM_FEATURE_CRYPTO // some ARM processors lack this extension
  return vmull_p64(-1ULL, quote_bits);
#else
  return portable_compute_quote_mask(quote_bits);
#endif
}

really_inline void find_whitespace_and_operators(
    const simd_input in,
    uint64_t &whitespace, uint64_t &op) {
  const uint8x16_t low_nibble_mask =
      (uint8x16_t){16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0};
  const uint8x16_t high_nibble_mask =
      (uint8x16_t){8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0};
  const uint8x16_t low_nib_and_mask = vmovq_n_u8(0xf);

  auto v = in.map([&](auto chunk) {
    uint8x16_t nib_lo = vandq_u8(chunk, low_nib_and_mask);
    uint8x16_t nib_hi = vshrq_n_u8(chunk, 4);
    uint8x16_t shuf_lo = vqtbl1q_u8(low_nibble_mask, nib_lo);
    uint8x16_t shuf_hi = vqtbl1q_u8(high_nibble_mask, nib_hi);
    return vandq_u8(shuf_lo, shuf_hi);
  });

  const uint8x16_t operator_shufti_mask = vmovq_n_u8(0x7);
  op = v.map([&](auto _v) {
    return vtstq_u8(_v, operator_shufti_mask);
  }).to_bitmask();

  const uint8x16_t whitespace_shufti_mask = vmovq_n_u8(0x18);
  whitespace = v.map([&](auto _v) {
    return vtstq_u8(_v, whitespace_shufti_mask);
  }).to_bitmask();
}

// This file contains a non-architecture-specific version of "flatten" used in stage1.
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

#ifdef SIMDJSON_NAIVE_FLATTEN // useful for benchmarking

// This is just a naive implementation. It should be normally
// disable, but can be used for research purposes to compare
// again our optimized version.
really_inline void flatten_bits(uint32_t *base_ptr, uint32_t &base, uint32_t idx, uint64_t bits) {
  uint32_t *out_ptr = base_ptr + base;
  idx -= 64;
  while (bits != 0) {
    out_ptr[0] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    out_ptr++;
  }
  base = (out_ptr - base_ptr);
}

#else // SIMDJSON_NAIVE_FLATTEN

// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
really_inline void flatten_bits(uint32_t *&base_ptr, uint32_t idx, uint64_t bits) {
  // In some instances, the next branch is expensive because it is mispredicted.
  // Unfortunately, in other cases,
  // it helps tremendously.
  if (bits == 0)
    return;
  uint32_t cnt = hamming(bits);
  idx -= 64;

  // Do the first 8 all together
  for (int i=0; i<8; i++) {
    base_ptr[i] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
  }

  // Do the next 8 all together (we hope in most cases it won't happen at all
  // and the branch is easily predicted).
  if (unlikely(cnt > 8)) {
    for (int i=8; i<16; i++) {
      base_ptr[i] = idx + trailing_zeroes(bits);
      bits = bits & (bits - 1);
    }

    // Most files don't have 16+ structurals per block, so we take several basically guaranteed
    // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
    // or the start of a value ("abc" true 123) every 4 characters.
    if (unlikely(cnt > 16)) {
      uint32_t i = 16;
      do {
        base_ptr[i] = idx + trailing_zeroes(bits);
        bits = bits & (bits - 1);
        i++;
      } while (i < cnt);
    }
  }

  base_ptr += cnt;
}
#endif // SIMDJSON_NAIVE_FLATTEN
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

// return a bitvector indicating where we have characters that end an odd-length
// sequence of backslashes (and thus change the behavior of the next character
// to follow). A even-length sequence of backslashes, and, for that matter, the
// largest even-length prefix of our odd-length sequence of backslashes, simply
// modify the behavior of the backslashes themselves.
// We also update the prev_iter_ends_odd_backslash reference parameter to
// indicate whether we end an iteration on an odd-length sequence of
// backslashes, which modifies our subsequent search for odd-length
// sequences of backslashes in an obvious way.
really_inline uint64_t follows_odd_sequence_of(const uint64_t match, uint64_t &overflow) {
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint64_t start_edges = match & ~(match << 1);
  /* flip lowest if we have an odd-length run at the end of the prior
   * iteration */
  uint64_t even_start_mask = even_bits ^ overflow;
  uint64_t even_starts = start_edges & even_start_mask;
  uint64_t odd_starts = start_edges & ~even_start_mask;
  uint64_t even_carries = match + even_starts;

  uint64_t odd_carries;
  /* must record the carry-out of our odd-carries out of bit 63; this
   * indicates whether the sense of any edge going to the next iteration
   * should be flipped */
  bool new_overflow = add_overflow(match, odd_starts, &odd_carries);

  odd_carries |= overflow; /* push in bit zero as a
                              * potential end if we had an
                              * odd-numbered run at the
                              * end of the previous
                              * iteration */
  overflow = new_overflow ? 0x1ULL : 0x0ULL;
  uint64_t even_carry_ends = even_carries & ~match;
  uint64_t odd_carry_ends = odd_carries & ~match;
  uint64_t even_start_odd_end = even_carry_ends & odd_bits;
  uint64_t odd_start_even_end = odd_carry_ends & even_bits;
  uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
  return odd_ends;
}

//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

//
// Check if the current character follows a matching character, with possible "filler" between.
// For example, this checks for empty curly braces, e.g. 
//
//     in.eq('}') & follows(in.eq('['), in.eq(' '), prev_empty_array) // { <whitespace>* }
//
really_inline uint64_t follows(const uint64_t match, const uint64_t filler, uint64_t &overflow ) {
  uint64_t follows_match = follows(match, overflow);
  uint64_t result;
  overflow |= add_overflow(follows_match, filler, &result);
  return result;
}

really_inline ErrorValues detect_errors_on_eof(
  uint64_t &unescaped_chars_error,
  const uint64_t prev_in_string) {
  if (prev_in_string) {
    return UNCLOSED_STRING;
  }
  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }
  return SUCCESS;
}

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
really_inline uint64_t find_strings(const simd_input in, uint64_t &prev_escaped, uint64_t &prev_in_string) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = follows_odd_sequence_of(backslash, prev_escaped);
  const uint64_t quote = in.eq('"') & ~escaped;
  // compute_quote_mask returns start quote plus string contents.
  const uint64_t in_string = compute_quote_mask(quote) ^ prev_in_string;
  /* right shift of a signed value expected to be well-defined and standard
   * compliant as of C++20,
   * John Regher from Utah U. says this is fine code */
  prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
  // Use ^ to turn the beginning quote off, and the end quote on.
  return in_string ^ quote;
}

really_inline uint64_t invalid_string_bytes(const uint64_t unescaped, const uint64_t quote_mask) {
  /* All Unicode characters may be placed within the
   * quotation marks, except for the characters that MUST be escaped:
   * quotation mark, reverse solidus, and the control characters (U+0000
   * through U+001F).
   * https://tools.ietf.org/html/rfc8259 */
  return quote_mask & unescaped;
}

//
// Determine which characters are *structural*:
// - braces: [] and {}
// - the start of primitives (123, true, false, null)
// - the start of invalid non-whitespace (+, &, ture, UTF-8)
//
// Also detects value sequence errors:
// - two values with no separator between ("hello" "world")
// - separators with no values ([1,] [1,,]and [,2])
//
// This method will find all of the above whether it is in a string or not.
//
// To reduce dependency on the expensive "what is in a string" computation, this method treats the
// contents of a string the same as content outside. Errors and structurals inside the string or on
// the trailing quote will need to be removed later when the correct string information is known.
//
really_inline uint64_t find_potential_structurals(const simd_input in, uint64_t &prev_primitive) {
  // These use SIMD so let's kick them off before running the regular 64-bit stuff ...
  uint64_t whitespace, op;
  find_whitespace_and_operators(in, whitespace, op);

  // Detect the start of a run of primitive characters. Includes numbers, booleans, and strings (").
  // Everything except whitespace, braces, colon and comma.
  const uint64_t primitive = ~(op | whitespace);
  const uint64_t follows_primitive = follows(primitive, prev_primitive);
  const uint64_t start_primitive = primitive & ~follows_primitive;

  // Return final structurals
  return op | start_primitive;
}

static const size_t STEP_SIZE = 128;

//
// Find the important bits of JSON in a 128-byte chunk, and add them to :
//
//
//
// PERF NOTES:
// We pipe 2 inputs through these stages:
// 1. Load JSON into registers. This takes a long time and is highly parallelizable, so we load
//    2 inputs' worth at once so that by the time step 2 is looking for them input, it's available.
// 2. Scan the JSON for critical data: strings, primitives and operators. This is the critical path.
//    The output of step 1 depends entirely on this information. These functions don't quite use
//    up enough CPU: the second half of the functions is highly serial, only using 1 execution core
//    at a time. The second input's scans has some dependency on the first ones finishing it, but
//    they can make a lot of progress before they need that information.
// 3. Step 1 doesn't use enough capacity, so we run some extra stuff while we're waiting for that
//    to finish: utf-8 checks and generating the output from the last iteration.
// 
// The reason we run 2 inputs at a time, is steps 2 and 3 are *still* not enough to soak up all
// available capacity with just one input. Running 2 at a time seems to give the CPU a good enough
// workout.
//
really_inline void find_structural_bits_128(
    const uint8_t *buf, const size_t idx, uint32_t *&base_ptr,
    uint64_t &prev_escaped, uint64_t &prev_in_string,
    uint64_t &prev_primitive,
    uint64_t &prev_structurals,
    uint64_t &unescaped_chars_error,
    utf8_checker &utf8_state) {
  //
  // Load up all 128 bytes into SIMD registers
  //
  simd_input in_1(buf);
  simd_input in_2(buf+64);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = find_strings(in_1, prev_escaped, prev_in_string);
  uint64_t structurals_1 = find_potential_structurals(in_1, prev_primitive);
  uint64_t string_2 = find_strings(in_2, prev_escaped, prev_in_string);
  uint64_t structurals_2 = find_potential_structurals(in_2, prev_primitive);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_state.check_next_input(in_1);
  flatten_bits(base_ptr, idx, prev_structurals); // Output *last* iteration's structurals to ParsedJson
  prev_structurals = structurals_1 & ~string_1;
  unescaped_chars_error |= unescaped_1 & string_1;

  uint64_t unescaped_2 = in_2.lteq(0x1F);
  utf8_state.check_next_input(in_2);
  flatten_bits(base_ptr, idx+64, prev_structurals); // Output *last* iteration's structurals to ParsedJson
  prev_structurals = structurals_2 & ~string_2;
  unescaped_chars_error |= unescaped_2 & string_2;
}

int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  if (unlikely(len > pj.byte_capacity)) {
    std::cerr << "Your ParsedJson object only supports documents up to "
              << pj.byte_capacity << " bytes but you are trying to process "
              << len << " bytes" << std::endl;
    return simdjson::CAPACITY;
  }
  uint32_t *base_ptr = pj.structural_indexes;
  utf8_checker utf8_state;

  // Whether the first character of the next iteration is escaped.
  uint64_t prev_escaped = 0ULL;
  // Whether the last iteration was still inside a string (all 1's = true, all 0's = false).
  uint64_t prev_in_string = 0ULL;
  // Whether the last character of the previous iteration is a primitive value character
  // (anything except whitespace, braces, comma or colon).
  uint64_t prev_primitive = 0ULL;
  // Mask of structural characters from the last iteration.
  // Kept around for performance reasons, so we can call flatten_bits to soak up some unused
  // CPU capacity while the next iteration is busy with an expensive clmul in compute_quote_mask.
  uint64_t structurals = 0;

  size_t lenminusstep = len < STEP_SIZE ? 0 : len - STEP_SIZE;
  size_t idx = 0;
  // Errors with unescaped characters in strings (ASCII codepoints < 0x20)
  uint64_t unescaped_chars_error = 0;

  for (; idx < lenminusstep; idx += STEP_SIZE) {
    find_structural_bits_128(&buf[idx], idx, base_ptr,
                             prev_escaped, prev_in_string, prev_primitive,
                             structurals, unescaped_chars_error, utf8_state);
  }

  /* If we have a final chunk of less than 64 bytes, pad it to 64 with
   * spaces  before processing it (otherwise, we risk invalidating the UTF-8
   * checks). */
  if (likely(idx < len)) {
    uint8_t tmp_buf[STEP_SIZE];
    memset(tmp_buf, 0x20, STEP_SIZE);
    memcpy(tmp_buf, buf + idx, len - idx);
    find_structural_bits_128(&tmp_buf[0], idx, base_ptr,
                             prev_escaped, prev_in_string, prev_primitive,
                             structurals, unescaped_chars_error, utf8_state);
    idx += STEP_SIZE;
  }

  /* finally, flatten out the remaining structurals from the last iteration */
  flatten_bits(base_ptr, idx, structurals);

  simdjson::ErrorValues error = detect_errors_on_eof(unescaped_chars_error, prev_in_string);
  if (unlikely(error != simdjson::SUCCESS)) {
    return error;
  }

  pj.n_structural_indexes = base_ptr - pj.structural_indexes;
  /* a valid JSON file cannot have zero structural indexes - we should have
   * found something */
  if (unlikely(pj.n_structural_indexes == 0u)) {
    return simdjson::EMPTY;
  }
  if (unlikely(pj.structural_indexes[pj.n_structural_indexes - 1] > len)) {
    return simdjson::UNEXPECTED_ERROR;
  }
  if (len != pj.structural_indexes[pj.n_structural_indexes - 1]) {
    /* the string might not be NULL terminated, but we add a virtual NULL
     * ending character. */
    pj.structural_indexes[pj.n_structural_indexes++] = len;
  }
  /* make it safe to dereference one beyond this array */
  pj.structural_indexes[pj.n_structural_indexes] = 0;
  return utf8_state.errors();
}

} // namespace simdjson::arm64

namespace simdjson {

template <>
int find_structural_bits<Architecture::ARM64>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  return arm64::find_structural_bits(buf, len, pj);
}

} // namespace simdjson

#endif // IS_ARM64
#endif // SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
/* end file src/arm64/stage1_find_marks.h */
/* begin file src/haswell/stage1_find_marks.h */
#ifndef SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H
#define SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H


#ifdef IS_X86_64


TARGET_HASWELL
namespace simdjson::haswell {

really_inline uint64_t compute_quote_mask(const uint64_t quote_bits) {
  // There should be no such thing with a processing supporting avx2
  // but not clmul.
  uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFFu), 0));
  return quote_mask;
}

really_inline void find_whitespace_and_operators(
  const simd_input in,
  uint64_t &whitespace, uint64_t &op) {

  #ifdef SIMDJSON_NAIVE_STRUCTURAL

    // You should never need this naive approach, but it can be useful
    // for research purposes
    const __m256i mask_open_brace = _mm256_set1_epi8(0x7b);
    const __m256i mask_close_brace = _mm256_set1_epi8(0x7d);
    const __m256i mask_open_bracket = _mm256_set1_epi8(0x5b);
    const __m256i mask_close_bracket = _mm256_set1_epi8(0x5d);
    const __m256i mask_column = _mm256_set1_epi8(0x3a);
    const __m256i mask_comma = _mm256_set1_epi8(0x2c);
    op = in.map([&](auto in) {
      __m256i op = _mm256_cmpeq_epi8(in, mask_open_brace);
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_close_brace));
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_open_bracket));
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_close_bracket));
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_column));
      op = _mm256_or_si256(op, _mm256_cmpeq_epi8(in, mask_comma));
      return op;
    }).to_bitmask();

    const __m256i mask_space = _mm256_set1_epi8(0x20);
    const __m256i mask_linefeed = _mm256_set1_epi8(0x0a);
    const __m256i mask_tab = _mm256_set1_epi8(0x09);
    const __m256i mask_carriage = _mm256_set1_epi8(0x0d);
    whitespace = in.map([&](auto in) {
      __m256i space = _mm256_cmpeq_epi8(in, mask_space);
      space = _mm256_or_si256(space, _mm256_cmpeq_epi8(in, mask_linefeed));
      space = _mm256_or_si256(space, _mm256_cmpeq_epi8(in, mask_tab));
      space = _mm256_or_si256(space, _mm256_cmpeq_epi8(in, mask_carriage));
      return space;
    }).to_bitmask();
    // end of naive approach

  #else  // SIMDJSON_NAIVE_STRUCTURAL

    // clang-format off
    const __m256i operator_table =
        _mm256_setr_epi8(44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123,
                         44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
    const __m256i white_table = _mm256_setr_epi8(
        32, 100, 100, 100, 17, 100, 113, 2, 100, 9, 10, 112, 100, 13, 100, 100,
        32, 100, 100, 100, 17, 100, 113, 2, 100, 9, 10, 112, 100, 13, 100, 100);
    // clang-format on
    const __m256i op_offset = _mm256_set1_epi8(0xd4u);
    const __m256i op_mask = _mm256_set1_epi8(32);

    whitespace = in.map([&](auto _in) {
      return _mm256_cmpeq_epi8(_in, _mm256_shuffle_epi8(white_table, _in));
    }).to_bitmask();

    op = in.map([&](auto _in) {
      const __m256i r1 = _mm256_add_epi8(op_offset, _in);
      const __m256i r2 = _mm256_or_si256(_in, op_mask);
      const __m256i r3 = _mm256_shuffle_epi8(operator_table, r1);
      return _mm256_cmpeq_epi8(r2, r3);
    }).to_bitmask();

  #endif // else SIMDJSON_NAIVE_STRUCTURAL
}

// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
really_inline void flatten_bits(uint32_t *&base_ptr, uint32_t idx, uint64_t bits) {
  // In some instances, the next branch is expensive because it is mispredicted.
  // Unfortunately, in other cases,
  // it helps tremendously.
  if (bits == 0)
      return;
  uint32_t cnt = _mm_popcnt_u64(bits);
  idx -= 64;

  // Do the first 8 all together
  for (int i=0; i<8; i++) {
    base_ptr[i] = idx + trailing_zeroes(bits);
    bits = _blsr_u64(bits);
  }

  // Do the next 8 all together (we hope in most cases it won't happen at all
  // and the branch is easily predicted).
  if (unlikely(cnt > 8)) {
    for (int i=8; i<16; i++) {
      base_ptr[i] = idx + trailing_zeroes(bits);
      bits = _blsr_u64(bits);
    }

    // Most files don't have 16+ structurals per block, so we take several basically guaranteed
    // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
    // or the start of a value ("abc" true 123) every four characters.
    if (unlikely(cnt > 16)) {
      uint32_t i = 16;
      do {
        base_ptr[i] = idx + trailing_zeroes(bits);
        bits = _blsr_u64(bits);
        i++;
      } while (i < cnt);
    }
  }

  base_ptr += cnt;
}

// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

// return a bitvector indicating where we have characters that end an odd-length
// sequence of backslashes (and thus change the behavior of the next character
// to follow). A even-length sequence of backslashes, and, for that matter, the
// largest even-length prefix of our odd-length sequence of backslashes, simply
// modify the behavior of the backslashes themselves.
// We also update the prev_iter_ends_odd_backslash reference parameter to
// indicate whether we end an iteration on an odd-length sequence of
// backslashes, which modifies our subsequent search for odd-length
// sequences of backslashes in an obvious way.
really_inline uint64_t follows_odd_sequence_of(const uint64_t match, uint64_t &overflow) {
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint64_t start_edges = match & ~(match << 1);
  /* flip lowest if we have an odd-length run at the end of the prior
   * iteration */
  uint64_t even_start_mask = even_bits ^ overflow;
  uint64_t even_starts = start_edges & even_start_mask;
  uint64_t odd_starts = start_edges & ~even_start_mask;
  uint64_t even_carries = match + even_starts;

  uint64_t odd_carries;
  /* must record the carry-out of our odd-carries out of bit 63; this
   * indicates whether the sense of any edge going to the next iteration
   * should be flipped */
  bool new_overflow = add_overflow(match, odd_starts, &odd_carries);

  odd_carries |= overflow; /* push in bit zero as a
                              * potential end if we had an
                              * odd-numbered run at the
                              * end of the previous
                              * iteration */
  overflow = new_overflow ? 0x1ULL : 0x0ULL;
  uint64_t even_carry_ends = even_carries & ~match;
  uint64_t odd_carry_ends = odd_carries & ~match;
  uint64_t even_start_odd_end = even_carry_ends & odd_bits;
  uint64_t odd_start_even_end = odd_carry_ends & even_bits;
  uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
  return odd_ends;
}

//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

//
// Check if the current character follows a matching character, with possible "filler" between.
// For example, this checks for empty curly braces, e.g. 
//
//     in.eq('}') & follows(in.eq('['), in.eq(' '), prev_empty_array) // { <whitespace>* }
//
really_inline uint64_t follows(const uint64_t match, const uint64_t filler, uint64_t &overflow ) {
  uint64_t follows_match = follows(match, overflow);
  uint64_t result;
  overflow |= add_overflow(follows_match, filler, &result);
  return result;
}

really_inline ErrorValues detect_errors_on_eof(
  uint64_t &unescaped_chars_error,
  const uint64_t prev_in_string) {
  if (prev_in_string) {
    return UNCLOSED_STRING;
  }
  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }
  return SUCCESS;
}

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
really_inline uint64_t find_strings(const simd_input in, uint64_t &prev_escaped, uint64_t &prev_in_string) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = follows_odd_sequence_of(backslash, prev_escaped);
  const uint64_t quote = in.eq('"') & ~escaped;
  // compute_quote_mask returns start quote plus string contents.
  const uint64_t in_string = compute_quote_mask(quote) ^ prev_in_string;
  /* right shift of a signed value expected to be well-defined and standard
   * compliant as of C++20,
   * John Regher from Utah U. says this is fine code */
  prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
  // Use ^ to turn the beginning quote off, and the end quote on.
  return in_string ^ quote;
}

really_inline uint64_t invalid_string_bytes(const uint64_t unescaped, const uint64_t quote_mask) {
  /* All Unicode characters may be placed within the
   * quotation marks, except for the characters that MUST be escaped:
   * quotation mark, reverse solidus, and the control characters (U+0000
   * through U+001F).
   * https://tools.ietf.org/html/rfc8259 */
  return quote_mask & unescaped;
}

//
// Determine which characters are *structural*:
// - braces: [] and {}
// - the start of primitives (123, true, false, null)
// - the start of invalid non-whitespace (+, &, ture, UTF-8)
//
// Also detects value sequence errors:
// - two values with no separator between ("hello" "world")
// - separators with no values ([1,] [1,,]and [,2])
//
// This method will find all of the above whether it is in a string or not.
//
// To reduce dependency on the expensive "what is in a string" computation, this method treats the
// contents of a string the same as content outside. Errors and structurals inside the string or on
// the trailing quote will need to be removed later when the correct string information is known.
//
really_inline uint64_t find_potential_structurals(const simd_input in, uint64_t &prev_primitive) {
  // These use SIMD so let's kick them off before running the regular 64-bit stuff ...
  uint64_t whitespace, op;
  find_whitespace_and_operators(in, whitespace, op);

  // Detect the start of a run of primitive characters. Includes numbers, booleans, and strings (").
  // Everything except whitespace, braces, colon and comma.
  const uint64_t primitive = ~(op | whitespace);
  const uint64_t follows_primitive = follows(primitive, prev_primitive);
  const uint64_t start_primitive = primitive & ~follows_primitive;

  // Return final structurals
  return op | start_primitive;
}

static const size_t STEP_SIZE = 128;

//
// Find the important bits of JSON in a 128-byte chunk, and add them to :
//
//
//
// PERF NOTES:
// We pipe 2 inputs through these stages:
// 1. Load JSON into registers. This takes a long time and is highly parallelizable, so we load
//    2 inputs' worth at once so that by the time step 2 is looking for them input, it's available.
// 2. Scan the JSON for critical data: strings, primitives and operators. This is the critical path.
//    The output of step 1 depends entirely on this information. These functions don't quite use
//    up enough CPU: the second half of the functions is highly serial, only using 1 execution core
//    at a time. The second input's scans has some dependency on the first ones finishing it, but
//    they can make a lot of progress before they need that information.
// 3. Step 1 doesn't use enough capacity, so we run some extra stuff while we're waiting for that
//    to finish: utf-8 checks and generating the output from the last iteration.
// 
// The reason we run 2 inputs at a time, is steps 2 and 3 are *still* not enough to soak up all
// available capacity with just one input. Running 2 at a time seems to give the CPU a good enough
// workout.
//
really_inline void find_structural_bits_128(
    const uint8_t *buf, const size_t idx, uint32_t *&base_ptr,
    uint64_t &prev_escaped, uint64_t &prev_in_string,
    uint64_t &prev_primitive,
    uint64_t &prev_structurals,
    uint64_t &unescaped_chars_error,
    utf8_checker &utf8_state) {
  //
  // Load up all 128 bytes into SIMD registers
  //
  simd_input in_1(buf);
  simd_input in_2(buf+64);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = find_strings(in_1, prev_escaped, prev_in_string);
  uint64_t structurals_1 = find_potential_structurals(in_1, prev_primitive);
  uint64_t string_2 = find_strings(in_2, prev_escaped, prev_in_string);
  uint64_t structurals_2 = find_potential_structurals(in_2, prev_primitive);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_state.check_next_input(in_1);
  flatten_bits(base_ptr, idx, prev_structurals); // Output *last* iteration's structurals to ParsedJson
  prev_structurals = structurals_1 & ~string_1;
  unescaped_chars_error |= unescaped_1 & string_1;

  uint64_t unescaped_2 = in_2.lteq(0x1F);
  utf8_state.check_next_input(in_2);
  flatten_bits(base_ptr, idx+64, prev_structurals); // Output *last* iteration's structurals to ParsedJson
  prev_structurals = structurals_2 & ~string_2;
  unescaped_chars_error |= unescaped_2 & string_2;
}

int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  if (unlikely(len > pj.byte_capacity)) {
    std::cerr << "Your ParsedJson object only supports documents up to "
              << pj.byte_capacity << " bytes but you are trying to process "
              << len << " bytes" << std::endl;
    return simdjson::CAPACITY;
  }
  uint32_t *base_ptr = pj.structural_indexes;
  utf8_checker utf8_state;

  // Whether the first character of the next iteration is escaped.
  uint64_t prev_escaped = 0ULL;
  // Whether the last iteration was still inside a string (all 1's = true, all 0's = false).
  uint64_t prev_in_string = 0ULL;
  // Whether the last character of the previous iteration is a primitive value character
  // (anything except whitespace, braces, comma or colon).
  uint64_t prev_primitive = 0ULL;
  // Mask of structural characters from the last iteration.
  // Kept around for performance reasons, so we can call flatten_bits to soak up some unused
  // CPU capacity while the next iteration is busy with an expensive clmul in compute_quote_mask.
  uint64_t structurals = 0;

  size_t lenminusstep = len < STEP_SIZE ? 0 : len - STEP_SIZE;
  size_t idx = 0;
  // Errors with unescaped characters in strings (ASCII codepoints < 0x20)
  uint64_t unescaped_chars_error = 0;

  for (; idx < lenminusstep; idx += STEP_SIZE) {
    find_structural_bits_128(&buf[idx], idx, base_ptr,
                             prev_escaped, prev_in_string, prev_primitive,
                             structurals, unescaped_chars_error, utf8_state);
  }

  /* If we have a final chunk of less than 64 bytes, pad it to 64 with
   * spaces  before processing it (otherwise, we risk invalidating the UTF-8
   * checks). */
  if (likely(idx < len)) {
    uint8_t tmp_buf[STEP_SIZE];
    memset(tmp_buf, 0x20, STEP_SIZE);
    memcpy(tmp_buf, buf + idx, len - idx);
    find_structural_bits_128(&tmp_buf[0], idx, base_ptr,
                             prev_escaped, prev_in_string, prev_primitive,
                             structurals, unescaped_chars_error, utf8_state);
    idx += STEP_SIZE;
  }

  /* finally, flatten out the remaining structurals from the last iteration */
  flatten_bits(base_ptr, idx, structurals);

  simdjson::ErrorValues error = detect_errors_on_eof(unescaped_chars_error, prev_in_string);
  if (unlikely(error != simdjson::SUCCESS)) {
    return error;
  }

  pj.n_structural_indexes = base_ptr - pj.structural_indexes;
  /* a valid JSON file cannot have zero structural indexes - we should have
   * found something */
  if (unlikely(pj.n_structural_indexes == 0u)) {
    return simdjson::EMPTY;
  }
  if (unlikely(pj.structural_indexes[pj.n_structural_indexes - 1] > len)) {
    return simdjson::UNEXPECTED_ERROR;
  }
  if (len != pj.structural_indexes[pj.n_structural_indexes - 1]) {
    /* the string might not be NULL terminated, but we add a virtual NULL
     * ending character. */
    pj.structural_indexes[pj.n_structural_indexes++] = len;
  }
  /* make it safe to dereference one beyond this array */
  pj.structural_indexes[pj.n_structural_indexes] = 0;
  return utf8_state.errors();
}

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
/* end file src/haswell/stage1_find_marks.h */
/* begin file src/westmere/stage1_find_marks.h */
#ifndef SIMDJSON_WESTMERE_STAGE1_FIND_MARKS_H
#define SIMDJSON_WESTMERE_STAGE1_FIND_MARKS_H


#ifdef IS_X86_64


TARGET_WESTMERE
namespace simdjson::westmere {

really_inline uint64_t compute_quote_mask(const uint64_t quote_bits) {
  return _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFFu), 0));
}

really_inline void find_whitespace_and_operators(
  const simd_input in,
  uint64_t &whitespace, uint64_t &op) {

  const __m128i operator_table =
      _mm_setr_epi8(44, 125, 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
  const __m128i white_table = _mm_setr_epi8(32, 100, 100, 100,  17, 100, 113,   2,
                                           100,   9,  10, 112, 100,  13, 100, 100);
  const __m128i op_offset = _mm_set1_epi8(0xd4u);
  const __m128i op_mask = _mm_set1_epi8(32);

  whitespace = in.map([&](auto _in) {
    return _mm_cmpeq_epi8(_in, _mm_shuffle_epi8(white_table, _in));
  }).to_bitmask();

  op = in.map([&](auto _in) {
    const __m128i r1 = _mm_add_epi8(op_offset, _in);
    const __m128i r2 = _mm_or_si128(_in, op_mask);
    const __m128i r3 = _mm_shuffle_epi8(operator_table, r1);
    return _mm_cmpeq_epi8(r2, r3);
  }).to_bitmask();
}

// This file contains a non-architecture-specific version of "flatten" used in stage1.
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

#ifdef SIMDJSON_NAIVE_FLATTEN // useful for benchmarking

// This is just a naive implementation. It should be normally
// disable, but can be used for research purposes to compare
// again our optimized version.
really_inline void flatten_bits(uint32_t *base_ptr, uint32_t &base, uint32_t idx, uint64_t bits) {
  uint32_t *out_ptr = base_ptr + base;
  idx -= 64;
  while (bits != 0) {
    out_ptr[0] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
    out_ptr++;
  }
  base = (out_ptr - base_ptr);
}

#else // SIMDJSON_NAIVE_FLATTEN

// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
really_inline void flatten_bits(uint32_t *&base_ptr, uint32_t idx, uint64_t bits) {
  // In some instances, the next branch is expensive because it is mispredicted.
  // Unfortunately, in other cases,
  // it helps tremendously.
  if (bits == 0)
    return;
  uint32_t cnt = hamming(bits);
  idx -= 64;

  // Do the first 8 all together
  for (int i=0; i<8; i++) {
    base_ptr[i] = idx + trailing_zeroes(bits);
    bits = bits & (bits - 1);
  }

  // Do the next 8 all together (we hope in most cases it won't happen at all
  // and the branch is easily predicted).
  if (unlikely(cnt > 8)) {
    for (int i=8; i<16; i++) {
      base_ptr[i] = idx + trailing_zeroes(bits);
      bits = bits & (bits - 1);
    }

    // Most files don't have 16+ structurals per block, so we take several basically guaranteed
    // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
    // or the start of a value ("abc" true 123) every 4 characters.
    if (unlikely(cnt > 16)) {
      uint32_t i = 16;
      do {
        base_ptr[i] = idx + trailing_zeroes(bits);
        bits = bits & (bits - 1);
        i++;
      } while (i < cnt);
    }
  }

  base_ptr += cnt;
}
#endif // SIMDJSON_NAIVE_FLATTEN
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

// return a bitvector indicating where we have characters that end an odd-length
// sequence of backslashes (and thus change the behavior of the next character
// to follow). A even-length sequence of backslashes, and, for that matter, the
// largest even-length prefix of our odd-length sequence of backslashes, simply
// modify the behavior of the backslashes themselves.
// We also update the prev_iter_ends_odd_backslash reference parameter to
// indicate whether we end an iteration on an odd-length sequence of
// backslashes, which modifies our subsequent search for odd-length
// sequences of backslashes in an obvious way.
really_inline uint64_t follows_odd_sequence_of(const uint64_t match, uint64_t &overflow) {
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint64_t start_edges = match & ~(match << 1);
  /* flip lowest if we have an odd-length run at the end of the prior
   * iteration */
  uint64_t even_start_mask = even_bits ^ overflow;
  uint64_t even_starts = start_edges & even_start_mask;
  uint64_t odd_starts = start_edges & ~even_start_mask;
  uint64_t even_carries = match + even_starts;

  uint64_t odd_carries;
  /* must record the carry-out of our odd-carries out of bit 63; this
   * indicates whether the sense of any edge going to the next iteration
   * should be flipped */
  bool new_overflow = add_overflow(match, odd_starts, &odd_carries);

  odd_carries |= overflow; /* push in bit zero as a
                              * potential end if we had an
                              * odd-numbered run at the
                              * end of the previous
                              * iteration */
  overflow = new_overflow ? 0x1ULL : 0x0ULL;
  uint64_t even_carry_ends = even_carries & ~match;
  uint64_t odd_carry_ends = odd_carries & ~match;
  uint64_t even_start_odd_end = even_carry_ends & odd_bits;
  uint64_t odd_start_even_end = odd_carry_ends & even_bits;
  uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
  return odd_ends;
}

//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

//
// Check if the current character follows a matching character, with possible "filler" between.
// For example, this checks for empty curly braces, e.g. 
//
//     in.eq('}') & follows(in.eq('['), in.eq(' '), prev_empty_array) // { <whitespace>* }
//
really_inline uint64_t follows(const uint64_t match, const uint64_t filler, uint64_t &overflow ) {
  uint64_t follows_match = follows(match, overflow);
  uint64_t result;
  overflow |= add_overflow(follows_match, filler, &result);
  return result;
}

really_inline ErrorValues detect_errors_on_eof(
  uint64_t &unescaped_chars_error,
  const uint64_t prev_in_string) {
  if (prev_in_string) {
    return UNCLOSED_STRING;
  }
  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }
  return SUCCESS;
}

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
really_inline uint64_t find_strings(const simd_input in, uint64_t &prev_escaped, uint64_t &prev_in_string) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = follows_odd_sequence_of(backslash, prev_escaped);
  const uint64_t quote = in.eq('"') & ~escaped;
  // compute_quote_mask returns start quote plus string contents.
  const uint64_t in_string = compute_quote_mask(quote) ^ prev_in_string;
  /* right shift of a signed value expected to be well-defined and standard
   * compliant as of C++20,
   * John Regher from Utah U. says this is fine code */
  prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
  // Use ^ to turn the beginning quote off, and the end quote on.
  return in_string ^ quote;
}

really_inline uint64_t invalid_string_bytes(const uint64_t unescaped, const uint64_t quote_mask) {
  /* All Unicode characters may be placed within the
   * quotation marks, except for the characters that MUST be escaped:
   * quotation mark, reverse solidus, and the control characters (U+0000
   * through U+001F).
   * https://tools.ietf.org/html/rfc8259 */
  return quote_mask & unescaped;
}

//
// Determine which characters are *structural*:
// - braces: [] and {}
// - the start of primitives (123, true, false, null)
// - the start of invalid non-whitespace (+, &, ture, UTF-8)
//
// Also detects value sequence errors:
// - two values with no separator between ("hello" "world")
// - separators with no values ([1,] [1,,]and [,2])
//
// This method will find all of the above whether it is in a string or not.
//
// To reduce dependency on the expensive "what is in a string" computation, this method treats the
// contents of a string the same as content outside. Errors and structurals inside the string or on
// the trailing quote will need to be removed later when the correct string information is known.
//
really_inline uint64_t find_potential_structurals(const simd_input in, uint64_t &prev_primitive) {
  // These use SIMD so let's kick them off before running the regular 64-bit stuff ...
  uint64_t whitespace, op;
  find_whitespace_and_operators(in, whitespace, op);

  // Detect the start of a run of primitive characters. Includes numbers, booleans, and strings (").
  // Everything except whitespace, braces, colon and comma.
  const uint64_t primitive = ~(op | whitespace);
  const uint64_t follows_primitive = follows(primitive, prev_primitive);
  const uint64_t start_primitive = primitive & ~follows_primitive;

  // Return final structurals
  return op | start_primitive;
}

static const size_t STEP_SIZE = 128;

//
// Find the important bits of JSON in a 128-byte chunk, and add them to :
//
//
//
// PERF NOTES:
// We pipe 2 inputs through these stages:
// 1. Load JSON into registers. This takes a long time and is highly parallelizable, so we load
//    2 inputs' worth at once so that by the time step 2 is looking for them input, it's available.
// 2. Scan the JSON for critical data: strings, primitives and operators. This is the critical path.
//    The output of step 1 depends entirely on this information. These functions don't quite use
//    up enough CPU: the second half of the functions is highly serial, only using 1 execution core
//    at a time. The second input's scans has some dependency on the first ones finishing it, but
//    they can make a lot of progress before they need that information.
// 3. Step 1 doesn't use enough capacity, so we run some extra stuff while we're waiting for that
//    to finish: utf-8 checks and generating the output from the last iteration.
// 
// The reason we run 2 inputs at a time, is steps 2 and 3 are *still* not enough to soak up all
// available capacity with just one input. Running 2 at a time seems to give the CPU a good enough
// workout.
//
really_inline void find_structural_bits_128(
    const uint8_t *buf, const size_t idx, uint32_t *&base_ptr,
    uint64_t &prev_escaped, uint64_t &prev_in_string,
    uint64_t &prev_primitive,
    uint64_t &prev_structurals,
    uint64_t &unescaped_chars_error,
    utf8_checker &utf8_state) {
  //
  // Load up all 128 bytes into SIMD registers
  //
  simd_input in_1(buf);
  simd_input in_2(buf+64);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = find_strings(in_1, prev_escaped, prev_in_string);
  uint64_t structurals_1 = find_potential_structurals(in_1, prev_primitive);
  uint64_t string_2 = find_strings(in_2, prev_escaped, prev_in_string);
  uint64_t structurals_2 = find_potential_structurals(in_2, prev_primitive);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_state.check_next_input(in_1);
  flatten_bits(base_ptr, idx, prev_structurals); // Output *last* iteration's structurals to ParsedJson
  prev_structurals = structurals_1 & ~string_1;
  unescaped_chars_error |= unescaped_1 & string_1;

  uint64_t unescaped_2 = in_2.lteq(0x1F);
  utf8_state.check_next_input(in_2);
  flatten_bits(base_ptr, idx+64, prev_structurals); // Output *last* iteration's structurals to ParsedJson
  prev_structurals = structurals_2 & ~string_2;
  unescaped_chars_error |= unescaped_2 & string_2;
}

int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  if (unlikely(len > pj.byte_capacity)) {
    std::cerr << "Your ParsedJson object only supports documents up to "
              << pj.byte_capacity << " bytes but you are trying to process "
              << len << " bytes" << std::endl;
    return simdjson::CAPACITY;
  }
  uint32_t *base_ptr = pj.structural_indexes;
  utf8_checker utf8_state;

  // Whether the first character of the next iteration is escaped.
  uint64_t prev_escaped = 0ULL;
  // Whether the last iteration was still inside a string (all 1's = true, all 0's = false).
  uint64_t prev_in_string = 0ULL;
  // Whether the last character of the previous iteration is a primitive value character
  // (anything except whitespace, braces, comma or colon).
  uint64_t prev_primitive = 0ULL;
  // Mask of structural characters from the last iteration.
  // Kept around for performance reasons, so we can call flatten_bits to soak up some unused
  // CPU capacity while the next iteration is busy with an expensive clmul in compute_quote_mask.
  uint64_t structurals = 0;

  size_t lenminusstep = len < STEP_SIZE ? 0 : len - STEP_SIZE;
  size_t idx = 0;
  // Errors with unescaped characters in strings (ASCII codepoints < 0x20)
  uint64_t unescaped_chars_error = 0;

  for (; idx < lenminusstep; idx += STEP_SIZE) {
    find_structural_bits_128(&buf[idx], idx, base_ptr,
                             prev_escaped, prev_in_string, prev_primitive,
                             structurals, unescaped_chars_error, utf8_state);
  }

  /* If we have a final chunk of less than 64 bytes, pad it to 64 with
   * spaces  before processing it (otherwise, we risk invalidating the UTF-8
   * checks). */
  if (likely(idx < len)) {
    uint8_t tmp_buf[STEP_SIZE];
    memset(tmp_buf, 0x20, STEP_SIZE);
    memcpy(tmp_buf, buf + idx, len - idx);
    find_structural_bits_128(&tmp_buf[0], idx, base_ptr,
                             prev_escaped, prev_in_string, prev_primitive,
                             structurals, unescaped_chars_error, utf8_state);
    idx += STEP_SIZE;
  }

  /* finally, flatten out the remaining structurals from the last iteration */
  flatten_bits(base_ptr, idx, structurals);

  simdjson::ErrorValues error = detect_errors_on_eof(unescaped_chars_error, prev_in_string);
  if (unlikely(error != simdjson::SUCCESS)) {
    return error;
  }

  pj.n_structural_indexes = base_ptr - pj.structural_indexes;
  /* a valid JSON file cannot have zero structural indexes - we should have
   * found something */
  if (unlikely(pj.n_structural_indexes == 0u)) {
    return simdjson::EMPTY;
  }
  if (unlikely(pj.structural_indexes[pj.n_structural_indexes - 1] > len)) {
    return simdjson::UNEXPECTED_ERROR;
  }
  if (len != pj.structural_indexes[pj.n_structural_indexes - 1]) {
    /* the string might not be NULL terminated, but we add a virtual NULL
     * ending character. */
    pj.structural_indexes[pj.n_structural_indexes++] = len;
  }
  /* make it safe to dereference one beyond this array */
  pj.structural_indexes[pj.n_structural_indexes] = 0;
  return utf8_state.errors();
}

} // namespace westmere
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {

template <>
int find_structural_bits<Architecture::WESTMERE>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  return westmere::find_structural_bits(buf, len, pj);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_WESTMERE_STAGE1_FIND_MARKS_H
/* end file src/westmere/stage1_find_marks.h */
/* begin file src/stage1_find_marks.cpp */

namespace {
// for when clmul is unavailable
[[maybe_unused]] uint64_t portable_compute_quote_mask(uint64_t quote_bits) {
  uint64_t quote_mask = quote_bits ^ (quote_bits << 1);
  quote_mask = quote_mask ^ (quote_mask << 2);
  quote_mask = quote_mask ^ (quote_mask << 4);
  quote_mask = quote_mask ^ (quote_mask << 8);
  quote_mask = quote_mask ^ (quote_mask << 16);
  quote_mask = quote_mask ^ (quote_mask << 32);
  return quote_mask;
}
} // namespace

/* end file src/stage1_find_marks.cpp */
/* begin file src/arm64/stringparsing.h */
#ifndef SIMDJSON_ARM64_STRINGPARSING_H
#define SIMDJSON_ARM64_STRINGPARSING_H

#ifdef IS_ARM64


#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

namespace simdjson::arm64 {

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
  really_inline uint32_t bytes_processed() const { return sizeof(uint8x16_t); }
};

really_inline parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(2 * sizeof(uint8x16_t) - 1 <= SIMDJSON_PADDING);
  uint8x16_t v0 = vld1q_u8(src);
  uint8x16_t v1 = vld1q_u8(src + 16);
  vst1q_u8(dst, v0);
  vst1q_u8(dst + 16, v1);

  uint8x16_t bs_mask = vmovq_n_u8('\\');
  uint8x16_t qt_mask = vmovq_n_u8('"');
  const uint8x16_t bit_mask = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                               0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  uint8x16_t cmp_bs_0 = vceqq_u8(v0, bs_mask);
  uint8x16_t cmp_bs_1 = vceqq_u8(v1, bs_mask);
  uint8x16_t cmp_qt_0 = vceqq_u8(v0, qt_mask);
  uint8x16_t cmp_qt_1 = vceqq_u8(v1, qt_mask);

  cmp_bs_0 = vandq_u8(cmp_bs_0, bit_mask);
  cmp_bs_1 = vandq_u8(cmp_bs_1, bit_mask);
  cmp_qt_0 = vandq_u8(cmp_qt_0, bit_mask);
  cmp_qt_1 = vandq_u8(cmp_qt_1, bit_mask);

  uint8x16_t sum0 = vpaddq_u8(cmp_bs_0, cmp_bs_1);
  uint8x16_t sum1 = vpaddq_u8(cmp_qt_0, cmp_qt_1);
  sum0 = vpaddq_u8(sum0, sum1);
  sum0 = vpaddq_u8(sum0, sum0);
  return {
      vgetq_lane_u32(vreinterpretq_u32_u8(sum0), 0), // bs_bits
      vgetq_lane_u32(vreinterpretq_u32_u8(sum0), 1)  // quote_bits
  };

}

// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "stringparsing.h" (this simplifies amalgation)

// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
WARN_UNUSED
really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = hex_to_u32_nocheck(*src_ptr + 2);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  size_t offset = codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

WARN_UNUSED really_inline bool parse_string(UNUSED const uint8_t *buf,
                                            UNUSED size_t len, ParsedJson &pj,
                                            UNUSED const uint32_t depth,
                                            UNUSED uint32_t offset) {
  pj.write_tape(pj.current_string_buf_loc - pj.string_buf, '"');
  const uint8_t *src = &buf[offset + 1]; /* we know that buf at offset is a " */
  uint8_t *dst = pj.current_string_buf_loc + sizeof(uint32_t);
  const uint8_t *const start_of_string = dst;
  while (1) {
    parse_string_helper helper = find_bs_bits_and_quote_bits(src, dst);
    if (((helper.bs_bits - 1) & helper.quote_bits) != 0) {
      /* we encountered quotes first. Move dst to point to quotes and exit
       */

      /* find out where the quote is... */
      uint32_t quote_dist = trailing_zeroes(helper.quote_bits);

      /* NULL termination is still handy if you expect all your strings to
       * be NULL terminated? */
      /* It comes at a small cost */
      dst[quote_dist] = 0;

      uint32_t str_length = (dst - start_of_string) + quote_dist;
      memcpy(pj.current_string_buf_loc, &str_length, sizeof(uint32_t));
      /*****************************
       * Above, check for overflow in case someone has a crazy string
       * (>=4GB?)                 _
       * But only add the overflow check when the document itself exceeds
       * 4GB
       * Currently unneeded because we refuse to parse docs larger or equal
       * to 4GB.
       ****************************/

      /* we advance the point, accounting for the fact that we have a NULL
       * termination         */
      pj.current_string_buf_loc = dst + quote_dist + 1;
      return true;
    }
    if (((helper.quote_bits - 1) & helper.bs_bits) != 0) {
      /* find out where the backspace is */
      uint32_t bs_dist = trailing_zeroes(helper.bs_bits);
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return false;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return false; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += helper.bytes_processed();
      dst += helper.bytes_processed();
    }
  }
  /* can't be reached */
  return true;
}

}
// namespace simdjson::amd64

#endif // IS_ARM64
#endif
/* end file src/arm64/stringparsing.h */
/* begin file src/haswell/stringparsing.h */
#ifndef SIMDJSON_HASWELL_STRINGPARSING_H
#define SIMDJSON_HASWELL_STRINGPARSING_H

#ifdef IS_X86_64


#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

TARGET_HASWELL
namespace simdjson::haswell {

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
  really_inline uint32_t bytes_processed() const { return sizeof(__m256i); }
};

really_inline parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(__m256i) - 1 <= SIMDJSON_PADDING);
  __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
  // store to dest unconditionally - we can overwrite the bits we don't like
  // later
  _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), v);
  auto quote_mask = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('"'));
  return {
      static_cast<uint32_t>(_mm256_movemask_epi8(
          _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\')))),     // bs_bits
      static_cast<uint32_t>(_mm256_movemask_epi8(quote_mask)) // quote_bits
  };
}

// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "stringparsing.h" (this simplifies amalgation)

// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
WARN_UNUSED
really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = hex_to_u32_nocheck(*src_ptr + 2);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  size_t offset = codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

WARN_UNUSED really_inline bool parse_string(UNUSED const uint8_t *buf,
                                            UNUSED size_t len, ParsedJson &pj,
                                            UNUSED const uint32_t depth,
                                            UNUSED uint32_t offset) {
  pj.write_tape(pj.current_string_buf_loc - pj.string_buf, '"');
  const uint8_t *src = &buf[offset + 1]; /* we know that buf at offset is a " */
  uint8_t *dst = pj.current_string_buf_loc + sizeof(uint32_t);
  const uint8_t *const start_of_string = dst;
  while (1) {
    parse_string_helper helper = find_bs_bits_and_quote_bits(src, dst);
    if (((helper.bs_bits - 1) & helper.quote_bits) != 0) {
      /* we encountered quotes first. Move dst to point to quotes and exit
       */

      /* find out where the quote is... */
      uint32_t quote_dist = trailing_zeroes(helper.quote_bits);

      /* NULL termination is still handy if you expect all your strings to
       * be NULL terminated? */
      /* It comes at a small cost */
      dst[quote_dist] = 0;

      uint32_t str_length = (dst - start_of_string) + quote_dist;
      memcpy(pj.current_string_buf_loc, &str_length, sizeof(uint32_t));
      /*****************************
       * Above, check for overflow in case someone has a crazy string
       * (>=4GB?)                 _
       * But only add the overflow check when the document itself exceeds
       * 4GB
       * Currently unneeded because we refuse to parse docs larger or equal
       * to 4GB.
       ****************************/

      /* we advance the point, accounting for the fact that we have a NULL
       * termination         */
      pj.current_string_buf_loc = dst + quote_dist + 1;
      return true;
    }
    if (((helper.quote_bits - 1) & helper.bs_bits) != 0) {
      /* find out where the backspace is */
      uint32_t bs_dist = trailing_zeroes(helper.bs_bits);
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return false;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return false; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += helper.bytes_processed();
      dst += helper.bytes_processed();
    }
  }
  /* can't be reached */
  return true;
}

} // namespace simdjson::haswell
UNTARGET_REGION

#endif // IS_X86_64

#endif
/* end file src/haswell/stringparsing.h */
/* begin file src/westmere/stringparsing.h */
#ifndef SIMDJSON_WESTMERE_STRINGPARSING_H
#define SIMDJSON_WESTMERE_STRINGPARSING_H

#ifdef IS_X86_64


#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

TARGET_WESTMERE
namespace simdjson::westmere {

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
  really_inline uint32_t bytes_processed() const { return sizeof(__m128i); }
};

really_inline parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
  // store to dest unconditionally - we can overwrite the bits we don't like
  // later
  _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), v);
  auto quote_mask = _mm_cmpeq_epi8(v, _mm_set1_epi8('"'));
  return {
      static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(v, _mm_set1_epi8('\\')))), // bs_bits
      static_cast<uint32_t>(_mm_movemask_epi8(quote_mask)) // quote_bits
  };
}

// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "stringparsing.h" (this simplifies amalgation)

// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
WARN_UNUSED
really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = hex_to_u32_nocheck(*src_ptr + 2);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  size_t offset = codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

WARN_UNUSED really_inline bool parse_string(UNUSED const uint8_t *buf,
                                            UNUSED size_t len, ParsedJson &pj,
                                            UNUSED const uint32_t depth,
                                            UNUSED uint32_t offset) {
  pj.write_tape(pj.current_string_buf_loc - pj.string_buf, '"');
  const uint8_t *src = &buf[offset + 1]; /* we know that buf at offset is a " */
  uint8_t *dst = pj.current_string_buf_loc + sizeof(uint32_t);
  const uint8_t *const start_of_string = dst;
  while (1) {
    parse_string_helper helper = find_bs_bits_and_quote_bits(src, dst);
    if (((helper.bs_bits - 1) & helper.quote_bits) != 0) {
      /* we encountered quotes first. Move dst to point to quotes and exit
       */

      /* find out where the quote is... */
      uint32_t quote_dist = trailing_zeroes(helper.quote_bits);

      /* NULL termination is still handy if you expect all your strings to
       * be NULL terminated? */
      /* It comes at a small cost */
      dst[quote_dist] = 0;

      uint32_t str_length = (dst - start_of_string) + quote_dist;
      memcpy(pj.current_string_buf_loc, &str_length, sizeof(uint32_t));
      /*****************************
       * Above, check for overflow in case someone has a crazy string
       * (>=4GB?)                 _
       * But only add the overflow check when the document itself exceeds
       * 4GB
       * Currently unneeded because we refuse to parse docs larger or equal
       * to 4GB.
       ****************************/

      /* we advance the point, accounting for the fact that we have a NULL
       * termination         */
      pj.current_string_buf_loc = dst + quote_dist + 1;
      return true;
    }
    if (((helper.quote_bits - 1) & helper.bs_bits) != 0) {
      /* find out where the backspace is */
      uint32_t bs_dist = trailing_zeroes(helper.bs_bits);
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return false;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return false; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += helper.bytes_processed();
      dst += helper.bytes_processed();
    }
  }
  /* can't be reached */
  return true;
}

} // namespace simdjson::westmere
UNTARGET_REGION

#endif // IS_X86_64

#endif
/* end file src/westmere/stringparsing.h */
/* begin file src/arm64/stage2_build_tape.h */
#ifndef SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
#define SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H


#ifdef IS_ARM64


namespace simdjson::arm64 {

// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2_build_tape.h" (this simplifies amalgation)

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
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
  uint32_t i = 0; /* index of the structural character (0,1,2,3...) */
  uint32_t idx; /* location of the structural character in the input (buf)   */
  uint8_t c;    /* used to track the (structural) character we are looking at,
                   updated */
  /* by UPDATE_CHAR macro */
  uint32_t depth = 0; /* could have an arbitrary starting depth */
  pj.init();          /* sets is_valid to false          */
  if (pj.byte_capacity < len) {
    pj.error_code = simdjson::CAPACITY;
    return pj.error_code;
  }

  /*//////////////////////////// START STATE /////////////////////////////
   */
  SET_GOTO_START_CONTINUE()
  pj.containing_scope_offset[depth] = pj.get_current_loc();
  pj.write_tape(0, 'r'); /* r for root, 0 is going to get overwritten */
  /* the root is used, if nothing else, to capture the size of the tape */
  depth++; /* everything starts at depth = 1, depth = 0 is just for the
              root, the root may contain an object, an array or something
              else. */
  if (depth >= pj.depth_capacity) {
    goto fail;
  }

  UPDATE_CHAR();
  switch (c) {
  case '{':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    SET_GOTO_START_CONTINUE();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(
        0, c); /* strangely, moving this to object_begin slows things down */
    goto object_begin;
  case '[':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    SET_GOTO_START_CONTINUE();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(0, c);
    goto array_begin;
    /* #define SIMDJSON_ALLOWANYTHINGINROOT
     * A JSON text is a serialized value.  Note that certain previous
     * specifications of JSON constrained a JSON text to be an object or an
     * array.  Implementations that generate only objects or arrays where a
     * JSON text is called for will be interoperable in the sense that all
     * implementations will accept these as conforming JSON texts.
     * https://tools.ietf.org/html/rfc8259
     * #ifdef SIMDJSON_ALLOWANYTHINGINROOT */
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the true value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!is_valid_true_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'f': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the false
     * value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!is_valid_false_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'n': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the null value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!is_valid_null_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this is done only for JSON documents made of a sole number
     * this will almost never be called in practice. We terminate with a
     * space
     * because we do not want to allow NULLs in the middle of a number
     * (whereas a
     * space in the middle of a number would be identified in stage 1). */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx,
                      false)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  case '-': {
    /* we need to make a copy to make sure that the string is NULL
     * terminated.
     * this is done only for JSON documents made of a sole number
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx, true)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  default:
    goto fail;
  }
start_continue:
  /* the string might not be NULL terminated. */
  if (i + 1 == pj.n_structural_indexes) {
    goto succeed;
  } else {
    goto fail;
  }
  /*//////////////////////////// OBJECT STATES ///////////////////////////*/

object_begin:
  UPDATE_CHAR();
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    goto object_key_state;
  }
  case '}':
    goto scope_end; /* could also go to object_continue */
  default:
    goto fail;
  }

object_key_state:
  UPDATE_CHAR();
  if (c != ':') {
    goto fail;
  }
  UPDATE_CHAR();
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't':
    if (!is_valid_true_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'f':
    if (!is_valid_false_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'n':
    if (!is_valid_null_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    if (!parse_number(buf, pj, idx, false)) {
      goto fail;
    }
    break;
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break;
  }
  case '{': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an object inside an object, so we need to increment the
     * depth                                                             */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an array inside an object, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

object_continue:
  UPDATE_CHAR();
  switch (c) {
  case ',':
    UPDATE_CHAR();
    if (c != '"') {
      goto fail;
    } else {
      if (!parse_string(buf, len, pj, depth, idx)) {
        goto fail;
      }
      goto object_key_state;
    }
  case '}':
    goto scope_end;
  default:
    goto fail;
  }

  /*//////////////////////////// COMMON STATE ///////////////////////////*/

scope_end:
  /* write our tape location to the header scope */
  depth--;
  pj.write_tape(pj.containing_scope_offset[depth], c);
  pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                           pj.get_current_loc());
  /* goto saved_state */
  GOTO_CONTINUE()

  /*//////////////////////////// ARRAY STATES ///////////////////////////*/
array_begin:
  UPDATE_CHAR();
  if (c == ']') {
    goto scope_end; /* could also go to array_continue */
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at c on the
   * on paths that can accept a close square brace (post-, and at start) */
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't':
    if (!is_valid_true_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'f':
    if (!is_valid_false_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'n':
    if (!is_valid_null_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break; /* goto array_continue; */

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    if (!parse_number(buf, pj, idx, false)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '{': {
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an object inside an array, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an array inside an array, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

array_continue:
  UPDATE_CHAR();
  switch (c) {
  case ',':
    UPDATE_CHAR();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto fail;
  }

  /*//////////////////////////// FINAL STATES ///////////////////////////*/

succeed:
  depth--;
  if (depth != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  if (pj.containing_scope_offset[depth] != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                           pj.get_current_loc());
  pj.write_tape(pj.containing_scope_offset[depth], 'r'); /* r is root */

  pj.valid = true;
  pj.error_code = simdjson::SUCCESS;
  return pj.error_code;
fail:
  /* we do not need the next line because this is done by pj.init(),
   * pessimistically.
   * pj.is_valid  = false;
   * At this point in the code, we have all the time in the world.
   * Note that we know exactly where we are in the document so we could,
   * without any overhead on the processing code, report a specific
   * location.
   * We could even trigger special code paths to assess what happened
   * carefully,
   * all without any added cost. */
  if (depth >= pj.depth_capacity) {
    pj.error_code = simdjson::DEPTH_ERROR;
    return pj.error_code;
  }
  switch (c) {
  case '"':
    pj.error_code = simdjson::STRING_ERROR;
    return pj.error_code;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case '-':
    pj.error_code = simdjson::NUMBER_ERROR;
    return pj.error_code;
  case 't':
    pj.error_code = simdjson::T_ATOM_ERROR;
    return pj.error_code;
  case 'n':
    pj.error_code = simdjson::N_ATOM_ERROR;
    return pj.error_code;
  case 'f':
    pj.error_code = simdjson::F_ATOM_ERROR;
    return pj.error_code;
  default:
    break;
  }
  pj.error_code = simdjson::TAPE_ERROR;
  return pj.error_code;
}

} // namespace simdjson::arm64

namespace simdjson {

template <>
WARN_UNUSED int
unified_machine<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  return arm64::unified_machine(buf, len, pj);
}

} // namespace simdjson

#endif // IS_ARM64

#endif // SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
/* end file src/arm64/stage2_build_tape.h */
/* begin file src/haswell/stage2_build_tape.h */
#ifndef SIMDJSON_HASWELL_STAGE2_BUILD_TAPE_H
#define SIMDJSON_HASWELL_STAGE2_BUILD_TAPE_H


#ifdef IS_X86_64


TARGET_HASWELL
namespace simdjson::haswell {

// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2_build_tape.h" (this simplifies amalgation)

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
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
  uint32_t i = 0; /* index of the structural character (0,1,2,3...) */
  uint32_t idx; /* location of the structural character in the input (buf)   */
  uint8_t c;    /* used to track the (structural) character we are looking at,
                   updated */
  /* by UPDATE_CHAR macro */
  uint32_t depth = 0; /* could have an arbitrary starting depth */
  pj.init();          /* sets is_valid to false          */
  if (pj.byte_capacity < len) {
    pj.error_code = simdjson::CAPACITY;
    return pj.error_code;
  }

  /*//////////////////////////// START STATE /////////////////////////////
   */
  SET_GOTO_START_CONTINUE()
  pj.containing_scope_offset[depth] = pj.get_current_loc();
  pj.write_tape(0, 'r'); /* r for root, 0 is going to get overwritten */
  /* the root is used, if nothing else, to capture the size of the tape */
  depth++; /* everything starts at depth = 1, depth = 0 is just for the
              root, the root may contain an object, an array or something
              else. */
  if (depth >= pj.depth_capacity) {
    goto fail;
  }

  UPDATE_CHAR();
  switch (c) {
  case '{':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    SET_GOTO_START_CONTINUE();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(
        0, c); /* strangely, moving this to object_begin slows things down */
    goto object_begin;
  case '[':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    SET_GOTO_START_CONTINUE();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(0, c);
    goto array_begin;
    /* #define SIMDJSON_ALLOWANYTHINGINROOT
     * A JSON text is a serialized value.  Note that certain previous
     * specifications of JSON constrained a JSON text to be an object or an
     * array.  Implementations that generate only objects or arrays where a
     * JSON text is called for will be interoperable in the sense that all
     * implementations will accept these as conforming JSON texts.
     * https://tools.ietf.org/html/rfc8259
     * #ifdef SIMDJSON_ALLOWANYTHINGINROOT */
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the true value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!is_valid_true_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'f': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the false
     * value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!is_valid_false_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'n': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the null value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!is_valid_null_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this is done only for JSON documents made of a sole number
     * this will almost never be called in practice. We terminate with a
     * space
     * because we do not want to allow NULLs in the middle of a number
     * (whereas a
     * space in the middle of a number would be identified in stage 1). */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx,
                      false)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  case '-': {
    /* we need to make a copy to make sure that the string is NULL
     * terminated.
     * this is done only for JSON documents made of a sole number
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx, true)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  default:
    goto fail;
  }
start_continue:
  /* the string might not be NULL terminated. */
  if (i + 1 == pj.n_structural_indexes) {
    goto succeed;
  } else {
    goto fail;
  }
  /*//////////////////////////// OBJECT STATES ///////////////////////////*/

object_begin:
  UPDATE_CHAR();
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    goto object_key_state;
  }
  case '}':
    goto scope_end; /* could also go to object_continue */
  default:
    goto fail;
  }

object_key_state:
  UPDATE_CHAR();
  if (c != ':') {
    goto fail;
  }
  UPDATE_CHAR();
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't':
    if (!is_valid_true_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'f':
    if (!is_valid_false_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'n':
    if (!is_valid_null_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    if (!parse_number(buf, pj, idx, false)) {
      goto fail;
    }
    break;
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break;
  }
  case '{': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an object inside an object, so we need to increment the
     * depth                                                             */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an array inside an object, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

object_continue:
  UPDATE_CHAR();
  switch (c) {
  case ',':
    UPDATE_CHAR();
    if (c != '"') {
      goto fail;
    } else {
      if (!parse_string(buf, len, pj, depth, idx)) {
        goto fail;
      }
      goto object_key_state;
    }
  case '}':
    goto scope_end;
  default:
    goto fail;
  }

  /*//////////////////////////// COMMON STATE ///////////////////////////*/

scope_end:
  /* write our tape location to the header scope */
  depth--;
  pj.write_tape(pj.containing_scope_offset[depth], c);
  pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                           pj.get_current_loc());
  /* goto saved_state */
  GOTO_CONTINUE()

  /*//////////////////////////// ARRAY STATES ///////////////////////////*/
array_begin:
  UPDATE_CHAR();
  if (c == ']') {
    goto scope_end; /* could also go to array_continue */
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at c on the
   * on paths that can accept a close square brace (post-, and at start) */
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't':
    if (!is_valid_true_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'f':
    if (!is_valid_false_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'n':
    if (!is_valid_null_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break; /* goto array_continue; */

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    if (!parse_number(buf, pj, idx, false)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '{': {
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an object inside an array, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an array inside an array, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

array_continue:
  UPDATE_CHAR();
  switch (c) {
  case ',':
    UPDATE_CHAR();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto fail;
  }

  /*//////////////////////////// FINAL STATES ///////////////////////////*/

succeed:
  depth--;
  if (depth != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  if (pj.containing_scope_offset[depth] != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                           pj.get_current_loc());
  pj.write_tape(pj.containing_scope_offset[depth], 'r'); /* r is root */

  pj.valid = true;
  pj.error_code = simdjson::SUCCESS;
  return pj.error_code;
fail:
  /* we do not need the next line because this is done by pj.init(),
   * pessimistically.
   * pj.is_valid  = false;
   * At this point in the code, we have all the time in the world.
   * Note that we know exactly where we are in the document so we could,
   * without any overhead on the processing code, report a specific
   * location.
   * We could even trigger special code paths to assess what happened
   * carefully,
   * all without any added cost. */
  if (depth >= pj.depth_capacity) {
    pj.error_code = simdjson::DEPTH_ERROR;
    return pj.error_code;
  }
  switch (c) {
  case '"':
    pj.error_code = simdjson::STRING_ERROR;
    return pj.error_code;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case '-':
    pj.error_code = simdjson::NUMBER_ERROR;
    return pj.error_code;
  case 't':
    pj.error_code = simdjson::T_ATOM_ERROR;
    return pj.error_code;
  case 'n':
    pj.error_code = simdjson::N_ATOM_ERROR;
    return pj.error_code;
  case 'f':
    pj.error_code = simdjson::F_ATOM_ERROR;
    return pj.error_code;
  default:
    break;
  }
  pj.error_code = simdjson::TAPE_ERROR;
  return pj.error_code;
}

} // namespace simdjson::haswell
UNTARGET_REGION

TARGET_HASWELL
namespace simdjson {

template <>
WARN_UNUSED int
unified_machine<Architecture::HASWELL>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  return haswell::unified_machine(buf, len, pj);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64

#endif // SIMDJSON_HASWELL_STAGE2_BUILD_TAPE_H
/* end file src/haswell/stage2_build_tape.h */
/* begin file src/westmere/stage2_build_tape.h */
#ifndef SIMDJSON_WESTMERE_STAGE2_BUILD_TAPE_H
#define SIMDJSON_WESTMERE_STAGE2_BUILD_TAPE_H


#ifdef IS_X86_64


TARGET_WESTMERE
namespace simdjson::westmere {

// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2_build_tape.h" (this simplifies amalgation)

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
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
  uint32_t i = 0; /* index of the structural character (0,1,2,3...) */
  uint32_t idx; /* location of the structural character in the input (buf)   */
  uint8_t c;    /* used to track the (structural) character we are looking at,
                   updated */
  /* by UPDATE_CHAR macro */
  uint32_t depth = 0; /* could have an arbitrary starting depth */
  pj.init();          /* sets is_valid to false          */
  if (pj.byte_capacity < len) {
    pj.error_code = simdjson::CAPACITY;
    return pj.error_code;
  }

  /*//////////////////////////// START STATE /////////////////////////////
   */
  SET_GOTO_START_CONTINUE()
  pj.containing_scope_offset[depth] = pj.get_current_loc();
  pj.write_tape(0, 'r'); /* r for root, 0 is going to get overwritten */
  /* the root is used, if nothing else, to capture the size of the tape */
  depth++; /* everything starts at depth = 1, depth = 0 is just for the
              root, the root may contain an object, an array or something
              else. */
  if (depth >= pj.depth_capacity) {
    goto fail;
  }

  UPDATE_CHAR();
  switch (c) {
  case '{':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    SET_GOTO_START_CONTINUE();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(
        0, c); /* strangely, moving this to object_begin slows things down */
    goto object_begin;
  case '[':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    SET_GOTO_START_CONTINUE();
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    pj.write_tape(0, c);
    goto array_begin;
    /* #define SIMDJSON_ALLOWANYTHINGINROOT
     * A JSON text is a serialized value.  Note that certain previous
     * specifications of JSON constrained a JSON text to be an object or an
     * array.  Implementations that generate only objects or arrays where a
     * JSON text is called for will be interoperable in the sense that all
     * implementations will accept these as conforming JSON texts.
     * https://tools.ietf.org/html/rfc8259
     * #ifdef SIMDJSON_ALLOWANYTHINGINROOT */
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the true value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!is_valid_true_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'f': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the false
     * value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!is_valid_false_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'n': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this only applies to the JSON document made solely of the null value.
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!is_valid_null_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    /* we need to make a copy to make sure that the string is space
     * terminated.
     * this is done only for JSON documents made of a sole number
     * this will almost never be called in practice. We terminate with a
     * space
     * because we do not want to allow NULLs in the middle of a number
     * (whereas a
     * space in the middle of a number would be identified in stage 1). */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx,
                      false)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  case '-': {
    /* we need to make a copy to make sure that the string is NULL
     * terminated.
     * this is done only for JSON documents made of a sole number
     * this will almost never be called in practice */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      goto fail;
    }
    memcpy(copy, buf, len);
    copy[len] = ' ';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx, true)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  default:
    goto fail;
  }
start_continue:
  /* the string might not be NULL terminated. */
  if (i + 1 == pj.n_structural_indexes) {
    goto succeed;
  } else {
    goto fail;
  }
  /*//////////////////////////// OBJECT STATES ///////////////////////////*/

object_begin:
  UPDATE_CHAR();
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    goto object_key_state;
  }
  case '}':
    goto scope_end; /* could also go to object_continue */
  default:
    goto fail;
  }

object_key_state:
  UPDATE_CHAR();
  if (c != ':') {
    goto fail;
  }
  UPDATE_CHAR();
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't':
    if (!is_valid_true_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'f':
    if (!is_valid_false_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'n':
    if (!is_valid_null_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    if (!parse_number(buf, pj, idx, false)) {
      goto fail;
    }
    break;
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break;
  }
  case '{': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an object inside an object, so we need to increment the
     * depth                                                             */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    /* we have not yet encountered } so we need to come back for it */
    SET_GOTO_OBJECT_CONTINUE()
    /* we found an array inside an object, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

object_continue:
  UPDATE_CHAR();
  switch (c) {
  case ',':
    UPDATE_CHAR();
    if (c != '"') {
      goto fail;
    } else {
      if (!parse_string(buf, len, pj, depth, idx)) {
        goto fail;
      }
      goto object_key_state;
    }
  case '}':
    goto scope_end;
  default:
    goto fail;
  }

  /*//////////////////////////// COMMON STATE ///////////////////////////*/

scope_end:
  /* write our tape location to the header scope */
  depth--;
  pj.write_tape(pj.containing_scope_offset[depth], c);
  pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                           pj.get_current_loc());
  /* goto saved_state */
  GOTO_CONTINUE()

  /*//////////////////////////// ARRAY STATES ///////////////////////////*/
array_begin:
  UPDATE_CHAR();
  if (c == ']') {
    goto scope_end; /* could also go to array_continue */
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at c on the
   * on paths that can accept a close square brace (post-, and at start) */
  switch (c) {
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't':
    if (!is_valid_true_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'f':
    if (!is_valid_false_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break;
  case 'n':
    if (!is_valid_null_atom(buf + idx)) {
      goto fail;
    }
    pj.write_tape(0, c);
    break; /* goto array_continue; */

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    if (!parse_number(buf, pj, idx, false)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break; /* goto array_continue; */
  }
  case '{': {
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an object inside an array, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    /* we have not yet encountered ] so we need to come back for it */
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); /* here the compilers knows what c is so this gets
                            optimized */
    SET_GOTO_ARRAY_CONTINUE()
    /* we found an array inside an array, so we need to increment the depth
     */
    depth++;
    if (depth >= pj.depth_capacity) {
      goto fail;
    }
    goto array_begin;
  }
  default:
    goto fail;
  }

array_continue:
  UPDATE_CHAR();
  switch (c) {
  case ',':
    UPDATE_CHAR();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto fail;
  }

  /*//////////////////////////// FINAL STATES ///////////////////////////*/

succeed:
  depth--;
  if (depth != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  if (pj.containing_scope_offset[depth] != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  pj.annotate_previous_loc(pj.containing_scope_offset[depth],
                           pj.get_current_loc());
  pj.write_tape(pj.containing_scope_offset[depth], 'r'); /* r is root */

  pj.valid = true;
  pj.error_code = simdjson::SUCCESS;
  return pj.error_code;
fail:
  /* we do not need the next line because this is done by pj.init(),
   * pessimistically.
   * pj.is_valid  = false;
   * At this point in the code, we have all the time in the world.
   * Note that we know exactly where we are in the document so we could,
   * without any overhead on the processing code, report a specific
   * location.
   * We could even trigger special code paths to assess what happened
   * carefully,
   * all without any added cost. */
  if (depth >= pj.depth_capacity) {
    pj.error_code = simdjson::DEPTH_ERROR;
    return pj.error_code;
  }
  switch (c) {
  case '"':
    pj.error_code = simdjson::STRING_ERROR;
    return pj.error_code;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case '-':
    pj.error_code = simdjson::NUMBER_ERROR;
    return pj.error_code;
  case 't':
    pj.error_code = simdjson::T_ATOM_ERROR;
    return pj.error_code;
  case 'n':
    pj.error_code = simdjson::N_ATOM_ERROR;
    return pj.error_code;
  case 'f':
    pj.error_code = simdjson::F_ATOM_ERROR;
    return pj.error_code;
  default:
    break;
  }
  pj.error_code = simdjson::TAPE_ERROR;
  return pj.error_code;
}

} // namespace simdjson::westmere
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {

template <>
WARN_UNUSED int
unified_machine<Architecture::WESTMERE>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  return westmere::unified_machine(buf, len, pj);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64

#endif // SIMDJSON_WESTMERE_STAGE2_BUILD_TAPE_H
/* end file src/westmere/stage2_build_tape.h */
/* begin file src/stage2_build_tape.cpp */
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
  if ((len <= byte_capacity) && (max_depth <= depth_capacity)) {
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
bool ParsedJson::print_json(std::ostream &os) const {
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
    case 'u':
      if (tape_idx + 1 >= how_many) {
        delete[] in_object;
        delete[] in_object_idx;
        return false;
      }
      os << tape[++tape_idx];
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
bool ParsedJson::dump_raw_tape(std::ostream &os) const {
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
    case 'u': // we have a long uint
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "unsigned integer " << tape[++tape_idx] << "\n";
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

namespace simdjson {
template class ParsedJson::BasicIterator<DEFAULT_MAX_DEPTH>;
} // namespace simdjson
/* end file src/parsedjsoniterator.cpp */
