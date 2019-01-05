/* auto-generated on Fri  4 Jan 2019 20:12:48 EST. Do not edit! */
#include "simdjson.h"

/* used for http://dmalloc.com/ Dmalloc - Debug Malloc Library */
#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* begin file /Users/lemire/CVS/github/simdjson/src/jsonioutil.cpp */
#include <cstring>
#include <stdlib.h>

char * allocate_padded_buffer(size_t length) {
    // we could do a simple malloc
    //return (char *) malloc(length + SIMDJSON_PADDING);
    // However, we might as well align to cache lines...
    char *padded_buffer;
    size_t totalpaddedlength = length + SIMDJSON_PADDING;
#ifdef _MSC_VER
	padded_buffer = (char*) _aligned_malloc(totalpaddedlength, 64);
#elif defined(__MINGW32__) || defined(__MINGW64__)
	padded_buffer = __mingw_aligned_malloc(totalpaddedlength, 64);
#else
    if (posix_memalign((void **)&padded_buffer, 64, totalpaddedlength) != 0) return NULL;
#endif
	return padded_buffer;
}

std::string_view get_corpus(std::string filename) {
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  if (fp) {
    std::fseek(fp, 0, SEEK_END);
    size_t len = std::ftell(fp);
    char * buf = allocate_padded_buffer(len);
    if(buf == NULL) {
      std::fclose(fp);
      throw  std::runtime_error("could not allocate memory");
    }
    std::rewind(fp);
    size_t readb = std::fread(buf, 1, len, fp);
    std::fclose(fp);
    if(readb != len) {
      aligned_free(buf);
      throw  std::runtime_error("could not read the data");
    }
    return std::string_view(buf,len);
  }
  throw  std::runtime_error("could not load corpus");
}
/* end file /Users/lemire/CVS/github/simdjson/src/jsonioutil.cpp */
/* begin file /Users/lemire/CVS/github/simdjson/src/jsonminifier.cpp */
#include <cstdint>
#ifndef __AVX2__


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

size_t jsonminify(const unsigned char *bytes, size_t howmany,
                  unsigned char *out) {
  size_t i = 0, pos = 0;
  uint8_t quote = 0;
  uint8_t nonescape = 1;

  while (i < howmany) {
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

#else

#include <cstring>

// a straightforward comparison of a mask against input.
static uint64_t cmp_mask_against_input_mini(__m256i input_lo, __m256i input_hi,
                                            __m256i mask) {
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(input_lo, mask);
  uint64_t res_0 = (uint32_t)_mm256_movemask_epi8(cmp_res_0);
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(input_hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

// take input from buf and remove useless whitespace, input and output can be
// the same, result is null terminated, return the string length (minus the null termination)
size_t jsonminify(const uint8_t *buf, size_t len, uint8_t *out) {
  // Useful constant masks
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint8_t *initout(out);
  uint64_t prev_iter_ends_odd_backslash =
      0ULL;                               // either 0 or 1, but a 64-bit value
  uint64_t prev_iter_inside_quote = 0ULL; // either all zeros or all ones
  size_t idx = 0;
  if (len >= 64) {
    size_t avxlen = len - 63;

    for (; idx < avxlen; idx += 64) {
      __m256i input_lo = _mm256_loadu_si256((const __m256i *)(buf + idx + 0));
      __m256i input_hi = _mm256_loadu_si256((const __m256i *)(buf + idx + 32));
      uint64_t bs_bits = cmp_mask_against_input_mini(input_lo, input_hi,
                                                     _mm256_set1_epi8('\\'));
      uint64_t start_edges = bs_bits & ~(bs_bits << 1);
      uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
      uint64_t even_starts = start_edges & even_start_mask;
      uint64_t odd_starts = start_edges & ~even_start_mask;
      uint64_t even_carries = bs_bits + even_starts;
      uint64_t odd_carries;
      bool iter_ends_odd_backslash = add_overflow(
          bs_bits, odd_starts, &odd_carries);
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
      prev_iter_inside_quote = (uint64_t)((int64_t)quote_mask >> 63);// might be undefined behavior, should be fully defined in C++20, ok according to John Regher from Utah University
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

      uint64_t ws_res_0 = (uint32_t)_mm256_movemask_epi8(tmp_ws_lo);
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
      __m256i vmask1 =
          _mm256_loadu2_m128i((const __m128i *)mask128_epi8 + (mask2 & 0x7FFF),
                              (const __m128i *)mask128_epi8 + (mask1 & 0x7FFF));
      __m256i vmask2 =
          _mm256_loadu2_m128i((const __m128i *)mask128_epi8 + (mask4 & 0x7FFF),
                              (const __m128i *)mask128_epi8 + (mask3 & 0x7FFF));
      __m256i result1 = _mm256_shuffle_epi8(input_lo, vmask1);
      __m256i result2 = _mm256_shuffle_epi8(input_hi, vmask2);
      _mm256_storeu2_m128i((__m128i *)(out + pop1), (__m128i *)out, result1);
      _mm256_storeu2_m128i((__m128i *)(out + pop3), (__m128i *)(out + pop2),
                           result2);
      out += pop4;
    }
  }
  // we finish off the job... copying and pasting the code is not ideal here,
  // but it gets the job done.
  if (idx < len) {
    uint8_t buffer[64];
    memset(buffer, 0, 64);
    memcpy(buffer, buf + idx, len - idx);
    __m256i input_lo = _mm256_loadu_si256((const __m256i *)(buffer));
    __m256i input_hi = _mm256_loadu_si256((const __m256i *)(buffer + 32));
    uint64_t bs_bits =
        cmp_mask_against_input_mini(input_lo, input_hi, _mm256_set1_epi8('\\'));
    uint64_t start_edges = bs_bits & ~(bs_bits << 1);
    uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
    uint64_t even_starts = start_edges & even_start_mask;
    uint64_t odd_starts = start_edges & ~even_start_mask;
    uint64_t even_carries = bs_bits + even_starts;
    uint64_t odd_carries;
    //bool iter_ends_odd_backslash = 
	add_overflow( bs_bits, odd_starts, &odd_carries);
    odd_carries |= prev_iter_ends_odd_backslash;
    //prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL; // we never use it
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
    // prev_iter_inside_quote = (uint64_t)((int64_t)quote_mask >> 63);// we don't need this anymore

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
    uint64_t ws_res_0 = (uint32_t)_mm256_movemask_epi8(tmp_ws_lo);
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
    __m256i vmask1 =
        _mm256_loadu2_m128i((const __m128i *)mask128_epi8 + (mask2 & 0x7FFF),
                            (const __m128i *)mask128_epi8 + (mask1 & 0x7FFF));
    __m256i vmask2 =
        _mm256_loadu2_m128i((const __m128i *)mask128_epi8 + (mask4 & 0x7FFF),
                            (const __m128i *)mask128_epi8 + (mask3 & 0x7FFF));
    __m256i result1 = _mm256_shuffle_epi8(input_lo, vmask1);
    __m256i result2 = _mm256_shuffle_epi8(input_hi, vmask2);
    _mm256_storeu2_m128i((__m128i *)(buffer + pop1), (__m128i *)buffer,
                         result1);
    _mm256_storeu2_m128i((__m128i *)(buffer + pop3), (__m128i *)(buffer + pop2),
                         result2);
    memcpy(out, buffer, pop4);
    out += pop4;
  }
  *out = '\0';// NULL termination
  return out - initout;
}

#endif
/* end file /Users/lemire/CVS/github/simdjson/src/jsonminifier.cpp */
/* begin file /Users/lemire/CVS/github/simdjson/src/jsonparser.cpp */
#ifdef _MSC_VER
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#endif


extern bool json_parse(const char * buf, size_t len, ParsedJson &pj, bool reallocifneeded);
extern bool json_parse(const std::string_view &s, ParsedJson &pj, bool reallocifneeded);
extern ParsedJson build_parsed_json(const char * buf, size_t len, bool reallocifneeded);
extern ParsedJson build_parsed_json(const std::string_view &s, bool reallocifneeded);


// parse a document found in buf, need to preallocate ParsedJson.
WARN_UNUSED
bool json_parse(const uint8_t *buf, size_t len, ParsedJson &pj, bool reallocifneeded) {
  if (pj.bytecapacity < len) {
    std::cerr << "Your ParsedJson cannot support documents that big: " << len
              << std::endl;
    return false;
  }
  bool reallocated = false;
  if(reallocifneeded) {
      // realloc is needed if the end of the memory crosses a page
#ifdef _MSC_VER
	  SYSTEM_INFO sysInfo; 
	  GetSystemInfo(&sysInfo); 
	  long pagesize = sysInfo.dwPageSize;
#else
     long pagesize = sysconf (_SC_PAGESIZE); 
#endif
	 if ( (reinterpret_cast<uintptr_t>(buf + len - 1) % pagesize ) < SIMDJSON_PADDING ) {
       const uint8_t *tmpbuf  = buf;
       buf = (uint8_t *) allocate_padded_buffer(len);
       if(buf == NULL) return false;
       memcpy((void*)buf,tmpbuf,len);
       reallocated = true;
     }
  }
  bool isok = find_structural_bits(buf, len, pj);
  /*if (isok) {
    isok = flatten_indexes(len, pj);
  } else {
    if(reallocated) free((void*)buf);
    return false;
  }*/
  if (isok) {
    isok = unified_machine(buf, len, pj);
  } else {
    if(reallocated) free((void*)buf);
    return false;
  }
  if(reallocated) free((void*)buf);
  return isok;
}

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len, bool reallocifneeded) {
  ParsedJson pj;
  bool ok = pj.allocateCapacity(len);
  if(ok) {
    ok = json_parse(buf, len, pj, reallocifneeded);
    assert(ok == pj.isValid());
  } else {
    std::cerr << "failure during memory allocation " << std::endl;
  }
  return pj;
}
/* end file /Users/lemire/CVS/github/simdjson/src/jsonparser.cpp */
/* begin file /Users/lemire/CVS/github/simdjson/src/stage1_find_marks.cpp */
#include <cassert>

#ifndef SIMDJSON_SKIPUTF8VALIDATION
#define SIMDJSON_UTF8VALIDATE
#endif

#ifndef NO_PDEP_WIDTH
#define NO_PDEP_WIDTH 8
#endif

#define SET_BIT(i)                                                             \
  base_ptr[base + i] = (uint32_t)idx - 64 + trailingzeroes(structurals);                          \
  structurals = structurals & (structurals - 1);

#define SET_BIT1 SET_BIT(0)
#define SET_BIT2 SET_BIT1 SET_BIT(1)
#define SET_BIT3 SET_BIT2 SET_BIT(2)
#define SET_BIT4 SET_BIT3 SET_BIT(3)
#define SET_BIT5 SET_BIT4 SET_BIT(4)
#define SET_BIT6 SET_BIT5 SET_BIT(5)
#define SET_BIT7 SET_BIT6 SET_BIT(6)
#define SET_BIT8 SET_BIT7 SET_BIT(7)
#define SET_BIT9 SET_BIT8 SET_BIT(8)
#define SET_BIT10 SET_BIT9 SET_BIT(9)
#define SET_BIT11 SET_BIT10 SET_BIT(10)
#define SET_BIT12 SET_BIT11 SET_BIT(11)
#define SET_BIT13 SET_BIT12 SET_BIT(12)
#define SET_BIT14 SET_BIT13 SET_BIT(13)
#define SET_BIT15 SET_BIT14 SET_BIT(14)
#define SET_BIT16 SET_BIT15 SET_BIT(15)

#define CALL(macro, ...) macro(__VA_ARGS__)

#define SET_BITLOOPN(n) SET_BIT##n

// It seems that many parsers do UTF-8 validation.
// RapidJSON does not do it by default, but a flag
// allows it.
#ifdef SIMDJSON_UTF8VALIDATE
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
      CALL(SET_BITLOOPN, NO_PDEP_WIDTH)
      /*for(size_t i = 0; i < NO_PDEP_WIDTH; i++) {
        base_ptr[base+i] = (uint32_t)idx + trailingzeroes(s);
        s = s & (s - 1);
      }*/
      base += NO_PDEP_WIDTH;
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
      CALL(SET_BITLOOPN, NO_PDEP_WIDTH)
      /*for(size_t i = 0; i < NO_PDEP_WIDTH; i++) {
        base_ptr[base+i] = (uint32_t)idx + trailingzeroes(s);
        s = s & (s - 1);
      }*/
      base += NO_PDEP_WIDTH;
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
      CALL(SET_BITLOOPN, NO_PDEP_WIDTH)
      /*for(size_t i = 0; i < NO_PDEP_WIDTH; i++) {
        base_ptr[base+i] = (uint32_t)idx + trailingzeroes(s);
        s = s & (s - 1);
      }*/
      base += NO_PDEP_WIDTH;
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
/* end file /Users/lemire/CVS/github/simdjson/src/stage1_find_marks.cpp */
/* begin file /Users/lemire/CVS/github/simdjson/src/stage2_build_tape.cpp */
#ifdef _MSC_VER
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <cassert>
#include <cstring>


#include <iostream>
#define PATH_SEP '/'


using namespace std;

WARN_UNUSED
really_inline bool is_valid_true_atom(const uint8_t *loc) {
  uint64_t tv = *(const uint64_t *)"true    ";
  uint64_t mask4 = 0x00000000ffffffff;
  uint32_t error = 0;
  uint64_t locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask4) ^ tv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_false_atom(const uint8_t *loc) {
  uint64_t fv = *(const uint64_t *)"false   ";
  uint64_t mask5 = 0x000000ffffffffff;
  uint32_t error = 0;
  uint64_t locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask5) ^ fv;
  error |= is_not_structural_or_whitespace(loc[5]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_null_atom(const uint8_t *loc) {
  uint64_t nv = *(const uint64_t *)"null    ";
  uint64_t mask4 = 0x00000000ffffffff;
  uint32_t error = 0;
  uint64_t locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask4) ^ nv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}


/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED
bool unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
  uint32_t i = 0; // index of the structural character (0,1,2,3...)
  uint32_t idx;   // location of the structural character in the input (buf)
  uint8_t c; // used to track the (structural) character we are looking at, updated
        // by UPDATE_CHAR macro
  uint32_t depth = 0; // could have an arbitrary starting depth
  pj.init();
  if(pj.bytecapacity < len) {
      fprintf(stderr, "insufficient capacity\n");
      return false;
  }
// this macro reads the next structural character, updating idx, i and c.
#define UPDATE_CHAR()                                                          \
  {                                                                            \
    idx = pj.structural_indexes[i++];                                          \
    c = buf[idx];                                                              \
  }


  ////////////////////////////// START STATE /////////////////////////////
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
  pj.ret_address[depth] = &&start_continue;
#else
  pj.ret_address[depth] = 's';
#endif
  pj.containing_scope_offset[depth] = pj.get_current_loc();
  pj.write_tape(0, 'r'); // r for root, 0 is going to get overwritten
  // the root is used, if nothing else, to capture the size of the tape
  depth++; // everything starts at depth = 1, depth = 0 is just for the root, the root may contain an object, an array or something else.
  if (depth > pj.depthcapacity) {
    goto fail;
  }

  UPDATE_CHAR();
  switch (c) {
  case '{':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&start_continue;
#else
    pj.ret_address[depth] = 's';
#endif
    depth++;
    if (depth > pj.depthcapacity) {
      goto fail;
    }
    pj.write_tape(0, c); // strangely, moving this to object_begin slows things down
    goto object_begin;
  case '[':
    pj.containing_scope_offset[depth] = pj.get_current_loc();
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&start_continue;
#else
    pj.ret_address[depth] = 's';
#endif    
    depth++;
    if (depth > pj.depthcapacity) {
      goto fail;
    }
    pj.write_tape(0, c);
    goto array_begin;
#define SIMDJSON_ALLOWANYTHINGINROOT
    // A JSON text is a serialized value.  Note that certain previous
    // specifications of JSON constrained a JSON text to be an object or an
    // array.  Implementations that generate only objects or arrays where a
    // JSON text is called for will be interoperable in the sense that all
    // implementations will accept these as conforming JSON texts.
    // https://tools.ietf.org/html/rfc8259
#ifdef SIMDJSON_ALLOWANYTHINGINROOT
  case '"': {
    if (!parse_string(buf, len, pj, depth, idx)) {
      goto fail;
    }
    break;
  }
  case 't': {
    // we need to make a copy to make sure that the string is NULL terminated.
    // this only applies to the JSON document made solely of the true value.
    // this will almost never be called in practice
    char * copy = (char *) malloc(len + SIMDJSON_PADDING);
    if(copy == NULL) goto fail;
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!is_valid_true_atom((const uint8_t *)copy + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'f': {
    // we need to make a copy to make sure that the string is NULL terminated.
    // this only applies to the JSON document made solely of the false value.
    // this will almost never be called in practice
    char * copy = (char *) malloc(len + SIMDJSON_PADDING);
    if(copy == NULL) goto fail;
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!is_valid_false_atom((const uint8_t *)copy + idx)) {
      free(copy);
      goto fail;
    }
    free(copy);
    pj.write_tape(0, c);
    break;
  }
  case 'n': {
    // we need to make a copy to make sure that the string is NULL terminated.
    // this only applies to the JSON document made solely of the null value.
    // this will almost never be called in practice
    char * copy = (char *) malloc(len + SIMDJSON_PADDING);
    if(copy == NULL) goto fail;
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!is_valid_null_atom((const uint8_t *)copy + idx)) {
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
    // we need to make a copy to make sure that the string is NULL terminated.
    // this is done only for JSON documents made of a sole number
    // this will almost never be called in practice
    char * copy = (char *) malloc(len + SIMDJSON_PADDING);
    if(copy == NULL) goto fail;
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!parse_number((const uint8_t *)copy, pj, idx, false)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
  case '-': {
    // we need to make a copy to make sure that the string is NULL terminated.
    // this is done only for JSON documents made of a sole number
    // this will almost never be called in practice
    char * copy = (char *) malloc(len + SIMDJSON_PADDING);
    if(copy == NULL) goto fail;
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!parse_number((const uint8_t *)copy, pj, idx, true)) {
      free(copy);
      goto fail;
    }
    free(copy);
    break;
  }
#endif // ALLOWANYTHINGINROOT
  default:
    goto fail;
  }
start_continue:
  // the string might not be NULL terminated.
  if(i + 1 == pj.n_structural_indexes) {
    goto succeed;
  } else {
    goto fail;
  }
  ////////////////////////////// OBJECT STATES /////////////////////////////

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
    goto scope_end; // could also go to object_continue
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
    pj.write_tape(0, c); // here the compilers knows what c is so this gets optimized
    // we have not yet encountered } so we need to come back for it
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&object_continue;
#else
    pj.ret_address[depth] = 'o';
#endif
    // we found an object inside an object, so we need to increment the depth
    depth++;
    if (depth > pj.depthcapacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c);  // here the compilers knows what c is so this gets optimized
    // we have not yet encountered } so we need to come back for it
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&object_continue;
#else
    pj.ret_address[depth] = 'o';
#endif    
    // we found an array inside an object, so we need to increment the depth
    depth++;
    if (depth > pj.depthcapacity) {
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

  ////////////////////////////// COMMON STATE /////////////////////////////

scope_end:
  // write our tape location to the header scope
  depth--;
  pj.write_tape(pj.containing_scope_offset[depth], c);
  pj.annotate_previousloc(pj.containing_scope_offset[depth],
                          pj.get_current_loc());
  // goto saved_state
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  goto *pj.ret_address[depth];
#else
  if(pj.ret_address[depth] == 'a') {
    goto array_continue;
  } else if (pj.ret_address[depth] == 'o') {
    goto object_continue;
  } else goto start_continue;
#endif

  ////////////////////////////// ARRAY STATES /////////////////////////////
array_begin:
  UPDATE_CHAR();
  if (c == ']') {
    goto scope_end; // could also go to array_continue
  }

main_array_switch:
  // we call update char on all paths in, so we can peek at c on the
  // on paths that can accept a close square brace (post-, and at start)
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
    break; // goto array_continue;

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
    break; // goto array_continue;
  }
  case '-': {
    if (!parse_number(buf, pj, idx, true)) {
      goto fail;
    }
    break; // goto array_continue;
  }
  case '{': {
    // we have not yet encountered ] so we need to come back for it
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); //  here the compilers knows what c is so this gets optimized
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&array_continue;
#else
    pj.ret_address[depth] = 'a';
#endif
    // we found an object inside an array, so we need to increment the depth
    depth++;
    if (depth > pj.depthcapacity) {
      goto fail;
    }

    goto object_begin;
  }
  case '[': {
    // we have not yet encountered ] so we need to come back for it
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); // here the compilers knows what c is so this gets optimized
#ifdef SIMDJSON_USE_COMPUTED_GOTO 
    pj.ret_address[depth] = &&array_continue;
#else
    pj.ret_address[depth] = 'a';
#endif
    // we found an array inside an array, so we need to increment the depth
    depth++;
    if (depth > pj.depthcapacity) {
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

  ////////////////////////////// FINAL STATES /////////////////////////////

succeed:
  depth --;
  if(depth != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  if(pj.containing_scope_offset[depth] != 0) {
    fprintf(stderr, "internal bug\n");
    abort();
  }
  pj.annotate_previousloc(pj.containing_scope_offset[depth],
                          pj.get_current_loc());
  pj.write_tape(pj.containing_scope_offset[depth], 'r'); // r is root



  pj.isvalid  = true;
  return true;

fail:
  return false;
}
/* end file /Users/lemire/CVS/github/simdjson/src/stage2_build_tape.cpp */
