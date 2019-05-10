/* auto-generated on Thu May  9 20:55:13 EDT 2019. Do not edit! */
#include "simdjson.h"

/* used for http://dmalloc.com/ Dmalloc - Debug Malloc Library */
#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* begin file src/jsonioutil.cpp */
#include <cstring>
#include <cstdlib>

char * allocate_padded_buffer(size_t length) {
    // we could do a simple malloc
    //return (char *) malloc(length + SIMDJSON_PADDING);
    // However, we might as well align to cache lines...
    size_t totalpaddedlength = length + SIMDJSON_PADDING;
    char *padded_buffer = aligned_malloc_char(64, totalpaddedlength);
    return padded_buffer;
}

padded_string get_corpus(const std::string& filename) {
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  if (fp != nullptr) {
    std::fseek(fp, 0, SEEK_END);
    size_t len = std::ftell(fp);
    padded_string s(len);
    if(s.data() == nullptr) {
      std::fclose(fp);
      throw  std::runtime_error("could not allocate memory");
    }
    std::rewind(fp);
    size_t readb = std::fread(s.data(), 1, len, fp);
    std::fclose(fp);
    if(readb != len) {
      throw  std::runtime_error("could not read the data");
    }
    return s;
  }
  throw  std::runtime_error("could not load corpus");
}
/* end file src/jsonioutil.cpp */
/* begin file src/jsonminifier.cpp */
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
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
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
      __m256i input_lo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + idx + 0));
      __m256i input_hi = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + idx + 32));
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
      prev_iter_inside_quote = static_cast<uint64_t>(static_cast<int64_t>(quote_mask) >> 63);// might be undefined behavior, should be fully defined in C++20, ok according to John Regher from Utah University
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

      uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(tmp_ws_lo));
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
          _mm256_loadu2_m128i(reinterpret_cast<const __m128i *>(mask128_epi8) + (mask2 & 0x7FFF),
                              reinterpret_cast<const __m128i *>(mask128_epi8) + (mask1 & 0x7FFF));
      __m256i vmask2 =
          _mm256_loadu2_m128i(reinterpret_cast<const __m128i *>(mask128_epi8) + (mask4 & 0x7FFF),
                              reinterpret_cast<const __m128i *>(mask128_epi8) + (mask3 & 0x7FFF));
      __m256i result1 = _mm256_shuffle_epi8(input_lo, vmask1);
      __m256i result2 = _mm256_shuffle_epi8(input_hi, vmask2);
      _mm256_storeu2_m128i(reinterpret_cast<__m128i *>(out + pop1), reinterpret_cast<__m128i *>(out), result1);
      _mm256_storeu2_m128i(reinterpret_cast<__m128i *>(out + pop3), reinterpret_cast<__m128i *>(out + pop2),
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
    __m256i input_lo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buffer));
    __m256i input_hi = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buffer + 32));
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
    __m256i vmask1 =
        _mm256_loadu2_m128i(reinterpret_cast<const __m128i *>(mask128_epi8) + (mask2 & 0x7FFF),
                            reinterpret_cast<const __m128i *>(mask128_epi8) + (mask1 & 0x7FFF));
    __m256i vmask2 =
        _mm256_loadu2_m128i(reinterpret_cast<const __m128i *>(mask128_epi8) + (mask4 & 0x7FFF),
                            reinterpret_cast<const __m128i *>(mask128_epi8) + (mask3 & 0x7FFF));
    __m256i result1 = _mm256_shuffle_epi8(input_lo, vmask1);
    __m256i result2 = _mm256_shuffle_epi8(input_hi, vmask2);
    _mm256_storeu2_m128i(reinterpret_cast<__m128i *>(buffer + pop1), reinterpret_cast<__m128i *>(buffer),
                         result1);
    _mm256_storeu2_m128i(reinterpret_cast<__m128i *>(buffer + pop3), reinterpret_cast<__m128i *>(buffer + pop2),
                         result2);
    memcpy(out, buffer, pop4);
    out += pop4;
  }
  *out = '\0';// NULL termination
  return out - initout;
}

#endif
/* end file src/jsonminifier.cpp */
/* begin file src/jsonparser.cpp */
#ifdef _MSC_VER
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#endif

// parse a document found in buf, need to preallocate ParsedJson.
WARN_UNUSED
int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj, bool reallocifneeded) {
  if (pj.bytecapacity < len) {
    return simdjson::CAPACITY;
  }
  bool reallocated = false;
  if(reallocifneeded) {
#ifdef ALLOW_SAME_PAGE_BUFFER_OVERRUN
	  // realloc is needed if the end of the memory crosses a page
#ifdef _MSC_VER
	  SYSTEM_INFO sysInfo; 
	  GetSystemInfo(&sysInfo); 
	  long pagesize = sysInfo.dwPageSize;
#else
    long pagesize = sysconf (_SC_PAGESIZE); 
#endif
  //////////////
  // We want to check that buf + len - 1 and buf + len - 1 + SIMDJSON_PADDING
  // are in the same page.
  // That is, we want to check that  
  // (buf + len - 1) / pagesize == (buf + len - 1 + SIMDJSON_PADDING) / pagesize
  // That's true if (buf + len - 1) % pagesize + SIMDJSON_PADDING < pagesize.
  ///////////
	 if ( (reinterpret_cast<uintptr_t>(buf + len - 1) % pagesize ) + SIMDJSON_PADDING < static_cast<uintptr_t>(pagesize) ) {
#else // SIMDJSON_SAFE_SAME_PAGE_READ_OVERRUN
     if(true) { // if not SIMDJSON_SAFE_SAME_PAGE_READ_OVERRUN, we always reallocate
#endif
	   const uint8_t *tmpbuf  = buf;
       buf = (uint8_t *) allocate_padded_buffer(len);
       if(buf == NULL) return simdjson::MEMALLOC;
       memcpy((void*)buf,tmpbuf,len);
       reallocated = true;
     }
  }
  // find_structural_bits returns a boolean, not an int, we invert its result to keep consistent with res == 0 meaning success
  int res = !find_structural_bits(buf, len, pj);
  if (!res) {
    res = unified_machine(buf, len, pj);
  }
  if(reallocated) { aligned_free((void*)buf);}
  return res;
}

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len, bool reallocifneeded) {
  ParsedJson pj;
  bool ok = pj.allocateCapacity(len);
  if(ok) {
    int res = json_parse(buf, len, pj, reallocifneeded);
    ok = res == simdjson::SUCCESS;
    assert(ok == pj.isValid());
  } else {
    std::cerr << "failure during memory allocation " << std::endl;
  }
  return pj;
}
/* end file src/jsonparser.cpp */
/* begin file src/stage1_find_marks.cpp */
#include <cassert>


#ifdef __AVX2__

#ifndef SIMDJSON_SKIPUTF8VALIDATION
#define SIMDJSON_UTF8VALIDATE

#endif
#else
// currently we don't UTF8 validate for ARM
// also we assume that if you're not __AVX2__ 
// you're ARM, which is a bit dumb. TODO: Fix...
#include <arm_neon.h>
#endif

// It seems that many parsers do UTF-8 validation.
// RapidJSON does not do it by default, but a flag
// allows it.
#ifdef SIMDJSON_UTF8VALIDATE
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
#error "It's called SIMDjson for a reason, bro"
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

#endif
}

// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
really_inline void flatten_bits(uint32_t *base_ptr, uint32_t &base,
                                uint32_t idx, uint64_t bits) {
  uint32_t cnt = hamming(bits);
  uint32_t next_base = base + cnt;
  while (bits != 0u) {
    base_ptr[base + 0] = static_cast<uint32_t>(idx) - 64 + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[base + 1] = static_cast<uint32_t>(idx) - 64 + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[base + 2] = static_cast<uint32_t>(idx) - 64 + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[base + 3] = static_cast<uint32_t>(idx) - 64 + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[base + 4] = static_cast<uint32_t>(idx) - 64 + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[base + 5] = static_cast<uint32_t>(idx) - 64 + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[base + 6] = static_cast<uint32_t>(idx) - 64 + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[base + 7] = static_cast<uint32_t>(idx) - 64 + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base += 8;
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
/* end file src/stage1_find_marks.cpp */
/* begin file src/stage2_build_tape.cpp */
#include <cassert>
#include <cstring>


#include <iostream>
#define PATH_SEP '/'


WARN_UNUSED
really_inline bool is_valid_true_atom(const uint8_t *loc) {
  uint64_t tv = *reinterpret_cast<const uint64_t *>("true    ");
  uint64_t mask4 = 0x00000000ffffffff;
  uint32_t error = 0;
  uint64_t locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 7 bytes beyond the buffer size, but we require 
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint64_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask4) ^ tv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_false_atom(const uint8_t *loc) {
  // We have to use an integer constant because the space in the cast
  // below would lead to values illegally being qualified
  // uint64_t fv = *reinterpret_cast<const uint64_t *>("false   ");
  // using this constant (that is the same false) but nulls out the
  // unused bits solves that
  uint64_t fv = 0x00000065736c6166; // takes into account endianness
  uint64_t mask5 = 0x000000ffffffffff;
  // we can't use the 32 bit value for checking for errors otherwise
  // the last character of false (it being 5 byte long!) would be
  // ignored
  uint64_t error = 0;
  uint64_t locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 7 bytes beyond the buffer size, but we require 
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint64_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask5) ^ fv;
  error |= is_not_structural_or_whitespace(loc[5]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_null_atom(const uint8_t *loc) {
  uint64_t nv = *reinterpret_cast<const uint64_t *>("null    ");
  uint64_t mask4 = 0x00000000ffffffff;
  uint32_t error = 0;
  uint64_t locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 7 bytes beyond the buffer size, but we require 
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint64_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint64_t));
  error = (locval & mask4) ^ nv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}


/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER
int unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
  uint32_t i = 0; // index of the structural character (0,1,2,3...)
  uint32_t idx;   // location of the structural character in the input (buf)
  uint8_t c; // used to track the (structural) character we are looking at, updated
        // by UPDATE_CHAR macro
  uint32_t depth = 0; // could have an arbitrary starting depth
  pj.init();
  if(pj.bytecapacity < len) {
      return simdjson::CAPACITY;
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
  if (depth >= pj.depthcapacity) {
    return simdjson::DEPTH_ERROR;
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
    if (depth >= pj.depthcapacity) {
      return simdjson::DEPTH_ERROR;
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
    if (depth >= pj.depthcapacity) {
      return simdjson::DEPTH_ERROR;
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
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { goto fail;
}
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!is_valid_true_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
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
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { goto fail;
}
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!is_valid_false_atom(reinterpret_cast<const uint8_t *>(copy) + idx)) {
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
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { goto fail;
}
    memcpy(copy, buf, len);
    copy[len] = '\0';
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
    // we need to make a copy to make sure that the string is NULL terminated.
    // this is done only for JSON documents made of a sole number
    // this will almost never be called in practice
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { goto fail;
}
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx, false)) {
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
    char * copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if(copy == nullptr) { goto fail;
}
    memcpy(copy, buf, len);
    copy[len] = '\0';
    if (!parse_number(reinterpret_cast<const uint8_t *>(copy), pj, idx, true)) {
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
    if (depth >= pj.depthcapacity) {
      return simdjson::DEPTH_ERROR;
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
    if (depth >= pj.depthcapacity) {
      return simdjson::DEPTH_ERROR;
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
    if (depth >= pj.depthcapacity) {
      return simdjson::DEPTH_ERROR;
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
    if (depth >= pj.depthcapacity) {
      return simdjson::DEPTH_ERROR;
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
  return simdjson::SUCCESS;

fail:
  return simdjson::TAPE_ERROR;
}

int unified_machine(const char *buf, size_t len, ParsedJson &pj) {
  return unified_machine(reinterpret_cast<const uint8_t*>(buf), len, pj);
}
/* end file src/stage2_build_tape.cpp */
/* begin file src/parsedjson.cpp */

ParsedJson::ParsedJson() : 
        structural_indexes(nullptr), tape(nullptr), containing_scope_offset(nullptr),
        ret_address(nullptr), string_buf(nullptr), current_string_buf_loc(nullptr) {}

ParsedJson::~ParsedJson() {
    deallocate();
}

ParsedJson::ParsedJson(ParsedJson && p)
    : bytecapacity(p.bytecapacity),
    depthcapacity(p.depthcapacity),
    tapecapacity(p.tapecapacity),
    stringcapacity(p.stringcapacity),
    current_loc(p.current_loc),
    n_structural_indexes(p.n_structural_indexes),
    structural_indexes(p.structural_indexes),
    tape(p.tape),
    containing_scope_offset(p.containing_scope_offset),
    ret_address(p.ret_address),
    string_buf(p.string_buf),
    current_string_buf_loc(p.current_string_buf_loc),
    isvalid(p.isvalid) {
        p.structural_indexes=nullptr;
        p.tape=nullptr;
        p.containing_scope_offset=nullptr;
        p.ret_address=nullptr;
        p.string_buf=nullptr;
        p.current_string_buf_loc=nullptr;
    }



WARN_UNUSED
bool ParsedJson::allocateCapacity(size_t len, size_t maxdepth) {
    if ((maxdepth == 0) || (len == 0)) {
      std::cerr << "capacities must be non-zero " << std::endl;
      return false;
    }
    if(len > SIMDJSON_MAXSIZE_BYTES) {
      return false;
    }
    if ((len <= bytecapacity) && (depthcapacity < maxdepth)) {
      return true;
    }
    deallocate();
    isvalid = false;
    bytecapacity = 0; // will only set it to len after allocations are a success
    n_structural_indexes = 0;
    uint32_t max_structures = ROUNDUP_N(len, 64) + 2 + 7;
    structural_indexes = new (std::nothrow) uint32_t[max_structures];
    // a pathological input like "[[[[..." would generate len tape elements, so need a capacity of len + 1
    size_t localtapecapacity = ROUNDUP_N(len + 1, 64);
    // a document with only zero-length strings... could have len/3 string
    // and we would need len/3 * 5 bytes on the string buffer 
    size_t localstringcapacity = ROUNDUP_N(5 * len / 3 + 32, 64);
    string_buf = new (std::nothrow) uint8_t[localstringcapacity];
    tape = new (std::nothrow) uint64_t[localtapecapacity];
    containing_scope_offset = new (std::nothrow) uint32_t[maxdepth];
#ifdef SIMDJSON_USE_COMPUTED_GOTO
    ret_address = new (std::nothrow) void *[maxdepth];
#else
    ret_address = new (std::nothrow) char[maxdepth];
#endif
    if ((string_buf == nullptr) || (tape == nullptr) ||
        (containing_scope_offset == nullptr) || (ret_address == nullptr) || (structural_indexes == nullptr)) {
      std::cerr << "Could not allocate memory" << std::endl;
      delete[] ret_address;
      delete[] containing_scope_offset;
      delete[] tape;
      delete[] string_buf;
      delete[] structural_indexes;

      return false;
    }

    bytecapacity = len;
    depthcapacity = maxdepth;
    tapecapacity = localtapecapacity;
    stringcapacity = localstringcapacity;
    return true;
}

bool ParsedJson::isValid() const {
    return isvalid;
}

void ParsedJson::deallocate() {
    bytecapacity = 0;
    depthcapacity = 0;
    tapecapacity = 0;
    stringcapacity = 0;
    delete[] ret_address;
    delete[] containing_scope_offset;
    delete[] tape;
    delete[] string_buf;
    delete[] structural_indexes;
    isvalid = false;
}

void ParsedJson::init() {
    current_string_buf_loc = string_buf;
    current_loc = 0;
    isvalid = false;
}

WARN_UNUSED
bool ParsedJson::printjson(std::ostream &os) {
    if(!isvalid) { 
      return false;
    }
    uint32_t string_length;
    size_t tapeidx = 0;
    uint64_t tape_val = tape[tapeidx];
    uint8_t type = (tape_val >> 56);
    size_t howmany = 0;
    if (type == 'r') {
      howmany = tape_val & JSONVALUEMASK;
    } else {
      fprintf(stderr, "Error: no starting root node?");
      return false;
    }
    if (howmany > tapecapacity) {
      fprintf(stderr,
          "We may be exceeding the tape capacity. Is this a valid document?\n");
      return false;
    }
    tapeidx++;
    bool *inobject = new bool[depthcapacity];
    auto *inobjectidx = new size_t[depthcapacity];
    int depth = 1; // only root at level 0
    inobjectidx[depth] = 0;
    inobject[depth] = false;
    for (; tapeidx < howmany; tapeidx++) {
      tape_val = tape[tapeidx];
      uint64_t payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      if (!inobject[depth]) {
        if ((inobjectidx[depth] > 0) && (type != ']')) {
          os << ",";
        }
        inobjectidx[depth]++;
      } else { // if (inobject) {
        if ((inobjectidx[depth] > 0) && ((inobjectidx[depth] & 1) == 0) &&
            (type != '}')) {
          os << ",";
        }
        if (((inobjectidx[depth] & 1) == 1)) {
          os << ":";
        }
        inobjectidx[depth]++;
      }
      switch (type) {
      case '"': // we have a string
        os << '"';
        memcpy(&string_length,string_buf + payload, sizeof(uint32_t));
        print_with_escapes((const unsigned char *)(string_buf + payload + sizeof(uint32_t)), string_length); 
        os << '"';
        break;
      case 'l': // we have a long int
        if (tapeidx + 1 >= howmany) {
          delete[] inobject;
          delete[] inobjectidx;
          return false;
        }
        os <<  static_cast<int64_t>(tape[++tapeidx]);
        break;
      case 'd': // we have a double
        if (tapeidx + 1 >= howmany){
          delete[] inobject;
          delete[] inobjectidx;
          return false;
        }
        double answer;
        memcpy(&answer, &tape[++tapeidx], sizeof(answer));
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
        inobject[depth] = true;
        inobjectidx[depth] = 0;
        break;
      case '}': // we end an object
        depth--;
        os << '}';
        break;
      case '[': // we start an array
        os << '[';
        depth++;
        inobject[depth] = false;
        inobjectidx[depth] = 0;
        break;
      case ']': // we end an array
        depth--;
        os << ']';
        break;
      case 'r': // we start and end with the root node
        fprintf(stderr, "should we be hitting the root node?\n");
        delete[] inobject;
        delete[] inobjectidx;
        return false;
      default:
        fprintf(stderr, "bug %c\n", type);
        delete[] inobject;
        delete[] inobjectidx;
        return false;
      }
    }
    delete[] inobject;
    delete[] inobjectidx;
    return true;
}

WARN_UNUSED
bool ParsedJson::dump_raw_tape(std::ostream &os) {
    if(!isvalid) { 
      return false;
    }
    uint32_t string_length;
    size_t tapeidx = 0;
    uint64_t tape_val = tape[tapeidx];
    uint8_t type = (tape_val >> 56);
    os << tapeidx << " : " << type;
    tapeidx++;
    size_t howmany = 0;
    if (type == 'r') {
      howmany = tape_val & JSONVALUEMASK;
    } else {
      fprintf(stderr, "Error: no starting root node?");
      return false;
    }
    os << "\t// pointing to " << howmany <<" (right after last node)\n";
    uint64_t payload;
    for (; tapeidx < howmany; tapeidx++) {
      os << tapeidx << " : ";
      tape_val = tape[tapeidx];
      payload = tape_val & JSONVALUEMASK;
      type = (tape_val >> 56);
      switch (type) {
      case '"': // we have a string
        os << "string \"";
        memcpy(&string_length,string_buf + payload, sizeof(uint32_t));
        print_with_escapes((const unsigned char *)(string_buf + payload + sizeof(uint32_t)), string_length);
        os << '"';
        os << '\n';
        break;
      case 'l': // we have a long int
        if (tapeidx + 1 >= howmany) {
          return false;
        }
        os << "integer " << static_cast<int64_t>(tape[++tapeidx]) << "\n";
        break;
      case 'd': // we have a double
        os << "float ";
        if (tapeidx + 1 >= howmany) {
          return false;
        }
        double answer;
        memcpy(&answer, &tape[++tapeidx], sizeof(answer));
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
        os << "{\t// pointing to next tape location " << payload << " (first node after the scope) \n";
        break;
      case '}': // we end an object
        os << "}\t// pointing to previous tape location " << payload << " (start of the scope) \n";
        break;
      case '[': // we start an array
        os << "[\t// pointing to next tape location " << payload << " (first node after the scope) \n";
        break;
      case ']': // we end an array
        os << "]\t// pointing to previous tape location " << payload << " (start of the scope) \n";
        break;
      case 'r': // we start and end with the root node
        printf("end of root\n");
        return false;
      default:
        return false;
      }
    }
    tape_val = tape[tapeidx];
    payload = tape_val & JSONVALUEMASK;
    type = (tape_val >> 56);
    os << tapeidx << " : "<< type <<"\t// pointing to " << payload <<" (start root)\n";
    return true;
}
/* end file src/parsedjson.cpp */
/* begin file src/parsedjsoniterator.cpp */
#include <iterator>

ParsedJson::iterator::iterator(ParsedJson &pj_) : pj(pj_), depth(0), location(0), tape_length(0), depthindex(nullptr) {
        if(!pj.isValid()) {
            throw InvalidJSON();
        }
        depthindex = new scopeindex_t[pj.depthcapacity];
        // memory allocation would throw
        //if(depthindex == nullptr) { 
        //    return;
        //}
        depthindex[0].start_of_scope = location;
        current_val = pj.tape[location++];
        current_type = (current_val >> 56);
        depthindex[0].scope_type = current_type;
        if (current_type == 'r') {
            tape_length = current_val & JSONVALUEMASK;
            if(location < tape_length) {
                current_val = pj.tape[location];
                current_type = (current_val >> 56);
                depth++;
                depthindex[depth].start_of_scope = location;
                depthindex[depth].scope_type = current_type;
              }
        } else {
            // should never happen
            throw InvalidJSON();
        }
}

ParsedJson::iterator::~iterator() {
      delete[] depthindex;
}

ParsedJson::iterator::iterator(const iterator &o):
    pj(o.pj), depth(o.depth), location(o.location),
    tape_length(0), current_type(o.current_type),
    current_val(o.current_val), depthindex(nullptr) {
    depthindex = new scopeindex_t[pj.depthcapacity];
    // allocation might throw
    memcpy(depthindex, o.depthindex, pj.depthcapacity * sizeof(depthindex[0]));
    tape_length = o.tape_length;
}

ParsedJson::iterator::iterator(iterator &&o):
      pj(o.pj), depth(o.depth), location(o.location),
      tape_length(o.tape_length), current_type(o.current_type),
      current_val(o.current_val), depthindex(o.depthindex) {
        o.depthindex = nullptr;// we take ownership
}

WARN_UNUSED
bool ParsedJson::iterator::isOk() const {
      return location < tape_length;
}

// useful for debuging purposes
size_t ParsedJson::iterator::get_tape_location() const {
    return location;
}

// useful for debuging purposes
size_t ParsedJson::iterator::get_tape_length() const {
    return tape_length;
}

// returns the current depth (start at 1 with 0 reserved for the fictitious root node)
size_t ParsedJson::iterator::get_depth() const {
    return depth;
}

// A scope is a series of nodes at the same depth, typically it is either an object ({) or an array ([).
// The root node has type 'r'.
uint8_t ParsedJson::iterator::get_scope_type() const {
    return depthindex[depth].scope_type;
}

bool ParsedJson::iterator::move_forward() {
    if(location + 1 >= tape_length) {
        return false; // we are at the end!
    }

    if ((current_type == '[') || (current_type == '{')){
        // We are entering a new scope
        depth++;
        depthindex[depth].start_of_scope = location;
        depthindex[depth].scope_type = current_type;
    } else if ((current_type == ']') || (current_type == '}')) {
        // Leaving a scope.
        depth--;
        if(depth == 0) {
            // Should not be necessary
            return false;
        }
    } else if ((current_type == 'd') || (current_type == 'l')) {
        // d and l types use 2 locations on the tape, not just one.
        location += 1;
    }

    location += 1;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
    return true;
}

uint8_t ParsedJson::iterator::get_type()  const {
    return current_type;
}


int64_t ParsedJson::iterator::get_integer()  const {
    if(location + 1 >= tape_length) { 
      return 0;// default value in case of error
    }
    return static_cast<int64_t>(pj.tape[location + 1]);
}

double ParsedJson::iterator::get_double()  const {
    if(location + 1 >= tape_length) { 
      return NAN;// default value in case of error
    }
    double answer;
    memcpy(&answer, & pj.tape[location + 1], sizeof(answer));
    return answer;
}

const char * ParsedJson::iterator::get_string() const {
   return  reinterpret_cast<const char *>(pj.string_buf + (current_val & JSONVALUEMASK) + sizeof(uint32_t)) ;
}


uint32_t ParsedJson::iterator::get_string_length() const {
    uint32_t answer;
    memcpy(&answer, reinterpret_cast<const char *>(pj.string_buf + (current_val & JSONVALUEMASK)), sizeof(uint32_t));
    return answer;
}

bool ParsedJson::iterator::is_object_or_array() const {
    return is_object_or_array(get_type());
}

bool ParsedJson::iterator::is_object() const {
    return get_type() == '{';
}

bool ParsedJson::iterator::is_array() const {
    return get_type() == '[';
}

bool ParsedJson::iterator::is_string() const {
    return get_type() == '"';
}

bool ParsedJson::iterator::is_integer() const {
    return get_type() == 'l';
}

bool ParsedJson::iterator::is_double() const {
    return get_type() == 'd';
}

bool ParsedJson::iterator::is_true() const {
    return get_type() == 't';
}

bool ParsedJson::iterator::is_false() const {
    return get_type() == 'f';
}

bool ParsedJson::iterator::is_null() const {
    return get_type() == 'n';
}

bool ParsedJson::iterator::is_object_or_array(uint8_t type) {
    return (type == '[' || (type == '{'));
}

bool ParsedJson::iterator::move_to_key(const char * key) {
    if(down()) {
      do {
        assert(is_string());
        bool rightkey = (strcmp(get_string(),key)==0);// null chars would fool this
        next();
        if(rightkey) { 
          return true;
        }
      } while(next());
      assert(up());// not found
    }
    return false;
}


 bool ParsedJson::iterator::next() {
    if ((current_type == '[') || (current_type == '{')){
    // we need to jump
    size_t npos = ( current_val & JSONVALUEMASK);
    if(npos >= tape_length) {
        return false; // shoud never happen unless at the root
    }
    uint64_t nextval = pj.tape[npos];
    uint8_t nexttype = (nextval >> 56);
    if((nexttype == ']') || (nexttype == '}')) {
        return false; // we reached the end of the scope
    }
    location = npos;
    current_val = nextval;
    current_type = nexttype;
    return true;
    } 
    size_t increment = (current_type == 'd' || current_type == 'l') ? 2 : 1;
    if(location + increment >= tape_length) { return false;
}
    uint64_t nextval = pj.tape[location + increment];
    uint8_t nexttype = (nextval >> 56);
    if((nexttype == ']') || (nexttype == '}')) {
        return false; // we reached the end of the scope
    }
    location = location + increment;
    current_val = nextval;
    current_type = nexttype;
    return true;
    
}


 bool ParsedJson::iterator::prev() {
    if(location - 1 < depthindex[depth].start_of_scope) { return false;
}
    location -= 1;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
    if ((current_type == ']') || (current_type == '}')){
    // we need to jump
    size_t new_location = ( current_val & JSONVALUEMASK);
    if(new_location < depthindex[depth].start_of_scope) {
        return false; // shoud never happen
    }
    location = new_location;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
    }
    return true;
}


 bool ParsedJson::iterator::up() {
    if(depth == 1) {
    return false; // don't allow moving back to root
    }
    to_start_scope();
    // next we just move to the previous value
    depth--;
    location -= 1;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
    return true;
}


 bool ParsedJson::iterator::down() {
    if(location + 1 >= tape_length) { return false;
}
    if ((current_type == '[') || (current_type == '{')) {
    size_t npos = (current_val & JSONVALUEMASK);
    if(npos == location + 2) {
        return false; // we have an empty scope
    }
    depth++;
    location = location + 1;
    depthindex[depth].start_of_scope = location;
    depthindex[depth].scope_type = current_type;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
    return true;
    }
    return false;
}

void ParsedJson::iterator::to_start_scope()  {
    location = depthindex[depth].start_of_scope;
    current_val = pj.tape[location];
    current_type = (current_val >> 56);
}

bool ParsedJson::iterator::print(std::ostream &os, bool escape_strings) const {
    if(!isOk()) { 
      return false;
    }
    switch (current_type) {
    case '"': // we have a string
    os << '"';
    if(escape_strings) {
        print_with_escapes(get_string(), os, get_string_length());
    } else {
        // was: os << get_string();, but given that we can include null chars, we have to do something crazier:
        std::copy(get_string(), get_string() + get_string_length(), std::ostream_iterator<char>(os));
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
/* end file src/parsedjsoniterator.cpp */
