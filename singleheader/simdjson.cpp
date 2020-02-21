/* auto-generated on Sun Feb  2 15:10:09 PST 2020. Do not edit! */
#include "simdjson.h"

/* used for http://dmalloc.com/ Dmalloc - Debug Malloc Library */
#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* begin file src/simdjson.cpp */
#include <map>

namespace simdjson {
const std::map<int, const std::string> error_strings = {
    {SUCCESS, "No error"},
    {SUCCESS_AND_HAS_MORE, "No error and buffer still has more data"},
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
    {UNINITIALIZED, "Uninitialized"},
    {EMPTY, "Empty: no JSON found"},
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
#include <climits>

namespace simdjson {

padded_string get_corpus(const std::string &filename) {
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  if (fp != nullptr) {
    if(std::fseek(fp, 0, SEEK_END) < 0) {
      std::fclose(fp);
      throw std::runtime_error("cannot seek in the file");
    }
    long llen = std::ftell(fp);
    if((llen < 0) || (llen == LONG_MAX)) {
      std::fclose(fp);
      throw std::runtime_error("cannot tell where we are in the file");
    }
    size_t len = (size_t) llen;
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

#ifndef SIMDJSON_ISSUE384RESOLVED // to avoid tripping users

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

//
// This fast code is disabled.
// See issue https://github.com/lemire/simdjson/issues/384
//
/* begin file src/simdprune_tables.h */
#ifndef SIMDJSON_SIMDPRUNE_TABLES_H
#define SIMDJSON_SIMDPRUNE_TABLES_H
#include <cstdint>

namespace simdjson { // table modified and copied from
                     // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetTable
static const unsigned char BitsSetTable256mul2[256] = {
    0,  2,  2,  4,  2,  4,  4,  6,  2,  4,  4,  6,  4,  6,  6,  8,  2,  4,  4,
    6,  4,  6,  6,  8,  4,  6,  6,  8,  6,  8,  8,  10, 2,  4,  4,  6,  4,  6,
    6,  8,  4,  6,  6,  8,  6,  8,  8,  10, 4,  6,  6,  8,  6,  8,  8,  10, 6,
    8,  8,  10, 8,  10, 10, 12, 2,  4,  4,  6,  4,  6,  6,  8,  4,  6,  6,  8,
    6,  8,  8,  10, 4,  6,  6,  8,  6,  8,  8,  10, 6,  8,  8,  10, 8,  10, 10,
    12, 4,  6,  6,  8,  6,  8,  8,  10, 6,  8,  8,  10, 8,  10, 10, 12, 6,  8,
    8,  10, 8,  10, 10, 12, 8,  10, 10, 12, 10, 12, 12, 14, 2,  4,  4,  6,  4,
    6,  6,  8,  4,  6,  6,  8,  6,  8,  8,  10, 4,  6,  6,  8,  6,  8,  8,  10,
    6,  8,  8,  10, 8,  10, 10, 12, 4,  6,  6,  8,  6,  8,  8,  10, 6,  8,  8,
    10, 8,  10, 10, 12, 6,  8,  8,  10, 8,  10, 10, 12, 8,  10, 10, 12, 10, 12,
    12, 14, 4,  6,  6,  8,  6,  8,  8,  10, 6,  8,  8,  10, 8,  10, 10, 12, 6,
    8,  8,  10, 8,  10, 10, 12, 8,  10, 10, 12, 10, 12, 12, 14, 6,  8,  8,  10,
    8,  10, 10, 12, 8,  10, 10, 12, 10, 12, 12, 14, 8,  10, 10, 12, 10, 12, 12,
    14, 10, 12, 12, 14, 12, 14, 14, 16};

static const uint8_t pshufb_combine_table[272] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x80, 0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x80, 0x80,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x80, 0x80, 0x80, 0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x80, 0x80, 0x80, 0x80, 0x00, 0x01, 0x02, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x00, 0x01, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x00, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
};

// 256 * 8 bytes = 2kB, easily fits in cache.
static const uint64_t thintable_epi8[256] = {
    0x0706050403020100, 0x0007060504030201, 0x0007060504030200,
    0x0000070605040302, 0x0007060504030100, 0x0000070605040301,
    0x0000070605040300, 0x0000000706050403, 0x0007060504020100,
    0x0000070605040201, 0x0000070605040200, 0x0000000706050402,
    0x0000070605040100, 0x0000000706050401, 0x0000000706050400,
    0x0000000007060504, 0x0007060503020100, 0x0000070605030201,
    0x0000070605030200, 0x0000000706050302, 0x0000070605030100,
    0x0000000706050301, 0x0000000706050300, 0x0000000007060503,
    0x0000070605020100, 0x0000000706050201, 0x0000000706050200,
    0x0000000007060502, 0x0000000706050100, 0x0000000007060501,
    0x0000000007060500, 0x0000000000070605, 0x0007060403020100,
    0x0000070604030201, 0x0000070604030200, 0x0000000706040302,
    0x0000070604030100, 0x0000000706040301, 0x0000000706040300,
    0x0000000007060403, 0x0000070604020100, 0x0000000706040201,
    0x0000000706040200, 0x0000000007060402, 0x0000000706040100,
    0x0000000007060401, 0x0000000007060400, 0x0000000000070604,
    0x0000070603020100, 0x0000000706030201, 0x0000000706030200,
    0x0000000007060302, 0x0000000706030100, 0x0000000007060301,
    0x0000000007060300, 0x0000000000070603, 0x0000000706020100,
    0x0000000007060201, 0x0000000007060200, 0x0000000000070602,
    0x0000000007060100, 0x0000000000070601, 0x0000000000070600,
    0x0000000000000706, 0x0007050403020100, 0x0000070504030201,
    0x0000070504030200, 0x0000000705040302, 0x0000070504030100,
    0x0000000705040301, 0x0000000705040300, 0x0000000007050403,
    0x0000070504020100, 0x0000000705040201, 0x0000000705040200,
    0x0000000007050402, 0x0000000705040100, 0x0000000007050401,
    0x0000000007050400, 0x0000000000070504, 0x0000070503020100,
    0x0000000705030201, 0x0000000705030200, 0x0000000007050302,
    0x0000000705030100, 0x0000000007050301, 0x0000000007050300,
    0x0000000000070503, 0x0000000705020100, 0x0000000007050201,
    0x0000000007050200, 0x0000000000070502, 0x0000000007050100,
    0x0000000000070501, 0x0000000000070500, 0x0000000000000705,
    0x0000070403020100, 0x0000000704030201, 0x0000000704030200,
    0x0000000007040302, 0x0000000704030100, 0x0000000007040301,
    0x0000000007040300, 0x0000000000070403, 0x0000000704020100,
    0x0000000007040201, 0x0000000007040200, 0x0000000000070402,
    0x0000000007040100, 0x0000000000070401, 0x0000000000070400,
    0x0000000000000704, 0x0000000703020100, 0x0000000007030201,
    0x0000000007030200, 0x0000000000070302, 0x0000000007030100,
    0x0000000000070301, 0x0000000000070300, 0x0000000000000703,
    0x0000000007020100, 0x0000000000070201, 0x0000000000070200,
    0x0000000000000702, 0x0000000000070100, 0x0000000000000701,
    0x0000000000000700, 0x0000000000000007, 0x0006050403020100,
    0x0000060504030201, 0x0000060504030200, 0x0000000605040302,
    0x0000060504030100, 0x0000000605040301, 0x0000000605040300,
    0x0000000006050403, 0x0000060504020100, 0x0000000605040201,
    0x0000000605040200, 0x0000000006050402, 0x0000000605040100,
    0x0000000006050401, 0x0000000006050400, 0x0000000000060504,
    0x0000060503020100, 0x0000000605030201, 0x0000000605030200,
    0x0000000006050302, 0x0000000605030100, 0x0000000006050301,
    0x0000000006050300, 0x0000000000060503, 0x0000000605020100,
    0x0000000006050201, 0x0000000006050200, 0x0000000000060502,
    0x0000000006050100, 0x0000000000060501, 0x0000000000060500,
    0x0000000000000605, 0x0000060403020100, 0x0000000604030201,
    0x0000000604030200, 0x0000000006040302, 0x0000000604030100,
    0x0000000006040301, 0x0000000006040300, 0x0000000000060403,
    0x0000000604020100, 0x0000000006040201, 0x0000000006040200,
    0x0000000000060402, 0x0000000006040100, 0x0000000000060401,
    0x0000000000060400, 0x0000000000000604, 0x0000000603020100,
    0x0000000006030201, 0x0000000006030200, 0x0000000000060302,
    0x0000000006030100, 0x0000000000060301, 0x0000000000060300,
    0x0000000000000603, 0x0000000006020100, 0x0000000000060201,
    0x0000000000060200, 0x0000000000000602, 0x0000000000060100,
    0x0000000000000601, 0x0000000000000600, 0x0000000000000006,
    0x0000050403020100, 0x0000000504030201, 0x0000000504030200,
    0x0000000005040302, 0x0000000504030100, 0x0000000005040301,
    0x0000000005040300, 0x0000000000050403, 0x0000000504020100,
    0x0000000005040201, 0x0000000005040200, 0x0000000000050402,
    0x0000000005040100, 0x0000000000050401, 0x0000000000050400,
    0x0000000000000504, 0x0000000503020100, 0x0000000005030201,
    0x0000000005030200, 0x0000000000050302, 0x0000000005030100,
    0x0000000000050301, 0x0000000000050300, 0x0000000000000503,
    0x0000000005020100, 0x0000000000050201, 0x0000000000050200,
    0x0000000000000502, 0x0000000000050100, 0x0000000000000501,
    0x0000000000000500, 0x0000000000000005, 0x0000000403020100,
    0x0000000004030201, 0x0000000004030200, 0x0000000000040302,
    0x0000000004030100, 0x0000000000040301, 0x0000000000040300,
    0x0000000000000403, 0x0000000004020100, 0x0000000000040201,
    0x0000000000040200, 0x0000000000000402, 0x0000000000040100,
    0x0000000000000401, 0x0000000000000400, 0x0000000000000004,
    0x0000000003020100, 0x0000000000030201, 0x0000000000030200,
    0x0000000000000302, 0x0000000000030100, 0x0000000000000301,
    0x0000000000000300, 0x0000000000000003, 0x0000000000020100,
    0x0000000000000201, 0x0000000000000200, 0x0000000000000002,
    0x0000000000000100, 0x0000000000000001, 0x0000000000000000,
    0x0000000000000000,
}; //static uint64_t thintable_epi8[256]

} // namespace simdjson 

#endif // SIMDJSON_SIMDPRUNE_TABLES_H
/* end file src/simdprune_tables.h */
#include <cstring>
#include <x86intrin.h> // currently, there is no runtime dispatch for the minifier

namespace simdjson {

// a straightforward comparison of a mask against input.
static uint64_t cmp_mask_against_input_mini(__m256i input_lo, __m256i input_hi,
                                            __m256i mask) {
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(input_lo, mask);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(input_hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

// Write up to 16 bytes, only the bytes corresponding to a 1-bit are written
// out. credit: Anime Tosho
static __m128i skinnycleanm128(__m128i x, int mask) {
  int mask1 = mask & 0xFF;
  int mask2 = (mask >> 8) & 0xFF;
  __m128i shufmask = _mm_castps_si128(
      _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64(
                       (const __m128i *)(thintable_epi8 + mask1))),
                   (const __m64 *)(thintable_epi8 + mask2)));
  shufmask =
      _mm_add_epi8(shufmask, _mm_set_epi32(0x08080808, 0x08080808, 0, 0));
  __m128i pruned = _mm_shuffle_epi8(x, shufmask);
  intptr_t popx2 = BitsSetTable256mul2[mask1];
  __m128i compactmask =
      _mm_loadu_si128((const __m128i *)(pshufb_combine_table + popx2 * 8));
  return _mm_shuffle_epi8(pruned, compactmask);
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

      uint64_t non_whitespace = ~whitespace;

      __m128i x1 = _mm256_extracti128_si256(input_lo, 0);
      __m128i x2 = _mm256_extracti128_si256(input_lo, 1);
      __m128i x3 = _mm256_extracti128_si256(input_hi, 0);
      __m128i x4 = _mm256_extracti128_si256(input_hi, 1);

      int mask1 = non_whitespace & 0xFFFF;
      int mask2 = (non_whitespace >> 16) & 0xFFFF;
      int mask3 = (non_whitespace >> 32) & 0xFFFF;
      int mask4 = (non_whitespace >> 48) & 0xFFFF;

      x1 = skinnycleanm128(x1, mask1);
      x2 = skinnycleanm128(x2, mask2);
      x3 = skinnycleanm128(x3, mask3);
      x4 = skinnycleanm128(x4, mask4);
      int pop1 = hamming(non_whitespace & 0xFFFF);
      int pop2 = hamming(non_whitespace & UINT64_C(0xFFFFFFFF));
      int pop3 = hamming(non_whitespace & UINT64_C(0xFFFFFFFFFFFF));
      int pop4 = hamming(non_whitespace);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out), x1);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop1), x2);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop2), x3);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop3), x4);
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
    int mask1 = non_whitespace & 0xFFFF;
    int mask2 = (non_whitespace >> 16) & 0xFFFF;
    int mask3 = (non_whitespace >> 32) & 0xFFFF;
    int mask4 = (non_whitespace >> 48) & 0xFFFF;

    x1 = skinnycleanm128(x1, mask1);
    x2 = skinnycleanm128(x2, mask2);
    x3 = skinnycleanm128(x3, mask3);
    x4 = skinnycleanm128(x4, mask4);
    int pop1 = hamming(non_whitespace & 0xFFFF);
    int pop2 = hamming(non_whitespace & UINT64_C(0xFFFFFFFF));
    int pop3 = hamming(non_whitespace & UINT64_C(0xFFFFFFFFFFFF));
    int pop4 = hamming(non_whitespace);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out), x1);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop1), x2);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop2), x3);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop3), x4);
    out += pop4;
  }
  *out = '\0'; // NULL termination
  return out - initout;
}

size_t oldjson_minify(const uint8_t *buf, size_t len, uint8_t *out) {
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
      __m128i x1 = _mm256_extracti128_si256(input_lo, 0);
      __m128i x2 = _mm256_extracti128_si256(input_lo, 1);
      __m128i x3 = _mm256_extracti128_si256(input_hi, 0);
      __m128i x4 = _mm256_extracti128_si256(input_hi, 1);
      x1 = skinnycleanm128(x1, mask1);
      x2 = skinnycleanm128(x2, mask2);
      x3 = skinnycleanm128(x3, mask3);
      x4 = skinnycleanm128(x4, mask4);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out), x1);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop1), x2);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop2), x3);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop3), x4);
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
    __m128i x1 = _mm256_extracti128_si256(input_lo, 0);
    __m128i x2 = _mm256_extracti128_si256(input_lo, 1);
    __m128i x3 = _mm256_extracti128_si256(input_hi, 0);
    __m128i x4 = _mm256_extracti128_si256(input_hi, 1);
    x1 = skinnycleanm128(x1, mask1);
    x2 = skinnycleanm128(x2, mask2);
    x3 = skinnycleanm128(x3, mask3);
    x4 = skinnycleanm128(x4, mask4);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer), x1);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer + pop1), x2);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer + pop2), x3);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer + pop3), x4);
    memcpy(out, buffer, pop4);
    out += pop4;
  }
  *out = '\0'; // NULL termination
  return out - initout;
}

} // namespace simdjson
#endif
/* end file src/simdprune_tables.h */
/* begin file src/jsonparser.cpp */
#include <atomic>

namespace simdjson {

// The function that users are expected to call is json_parse.
// We have more than one such function because we want to support several
// instruction sets.

// function pointer type for json_parse
using json_parse_functype = int(const uint8_t *buf, size_t len, ParsedJson &pj, bool realloc);

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

Architecture find_best_supported_architecture() {
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
  if (supports & instruction_set::NEON)
    return Architecture::ARM64;

  return Architecture::UNSUPPORTED;
}

Architecture parse_architecture(char *architecture) {
  if (!strcmp(architecture, "HASWELL")) { return Architecture::HASWELL; }
  if (!strcmp(architecture, "WESTMERE")) { return Architecture::WESTMERE; }
  if (!strcmp(architecture, "ARM64")) { return Architecture::ARM64; }
  return Architecture::UNSUPPORTED;
}

// Responsible to select the best json_parse implementation
int json_parse_dispatch(const uint8_t *buf, size_t len, ParsedJson &pj,
                        bool realloc) {
  Architecture best_implementation = find_best_supported_architecture();
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
    // The processor is not supported by simdjson.
    return simdjson::UNEXPECTED_ERROR;
  }

  return json_parse_ptr.load(std::memory_order_relaxed)(buf, len, pj, realloc);
}

std::atomic<json_parse_functype *> json_parse_ptr{&json_parse_dispatch};

WARN_UNUSED
ParsedJson build_parsed_json(const uint8_t *buf, size_t len,
                             bool realloc) {
  ParsedJson pj;
  bool ok = pj.allocate_capacity(len);
  if (ok) {
    json_parse(buf, len, pj, realloc);
  } 
  return pj;
}
} // namespace simdjson
/* end file src/jsonparser.cpp */
/* begin file src/stage1_find_marks.cpp */
/* begin file src/arm64/stage1_find_marks.h */
#ifndef SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
#define SIMDJSON_ARM64_STAGE1_FIND_MARKS_H


#ifdef IS_ARM64

/* begin file src/arm64/bitmask.h */
#ifndef SIMDJSON_ARM64_BITMASK_H
#define SIMDJSON_ARM64_BITMASK_H


#ifdef IS_ARM64

/* begin file src/arm64/intrinsics.h */
#ifndef SIMDJSON_ARM64_INTRINSICS_H
#define SIMDJSON_ARM64_INTRINSICS_H
#ifdef IS_ARM64

// This should be the correct header whether
// you use visual studio or other compilers.
#include <arm_neon.h>
#endif //   IS_ARM64
#endif //  SIMDJSON_ARM64_INTRINSICS_H
/* end file src/arm64/intrinsics.h */

namespace simdjson::arm64 {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
really_inline uint64_t prefix_xor(uint64_t bitmask) {
  /////////////
  // We could do this with PMULL, but it is apparently slow.
  //  
  //#ifdef __ARM_FEATURE_CRYPTO // some ARM processors lack this extension
  //return vmull_p64(-1ULL, bitmask);
  //#else
  // Analysis by @sebpop:
  // When diffing the assembly for src/stage1_find_marks.cpp I see that the eors are all spread out
  // in between other vector code, so effectively the extra cycles of the sequence do not matter 
  // because the GPR units are idle otherwise and the critical path is on the FP side.
  // Also the PMULL requires two extra fmovs: GPR->FP (3 cycles in N1, 5 cycles in A72 ) 
  // and FP->GPR (2 cycles on N1 and 5 cycles on A72.)
  ///////////
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
}

} // namespace simdjson::arm64
UNTARGET_REGION

#endif // IS_ARM64
#endif
/* end file src/arm64/intrinsics.h */
/* begin file src/arm64/simd.h */
#ifndef SIMDJSON_ARM64_SIMD_H
#define SIMDJSON_ARM64_SIMD_H


#ifdef IS_ARM64

/* arm64/intrinsics.h already included: #include "arm64/intrinsics.h" */

namespace simdjson::arm64::simd {

  template<typename T>
  struct simd8;

  //
  // Base class of simd8<uint8_t> and simd8<bool>, both of which use uint8x16_t internally.
  //
  template<typename T, typename Mask=simd8<bool>>
  struct base_u8 {
    uint8x16_t value;
    static const int SIZE = sizeof(value);

    // Conversion from/to SIMD register
    really_inline base_u8(const uint8x16_t _value) : value(_value) {}
    really_inline operator const uint8x16_t&() const { return this->value; }
    really_inline operator uint8x16_t&() { return this->value; }

    // Bit operations
    really_inline simd8<T> operator|(const simd8<T> other) const { return vorrq_u8(*this, other); }
    really_inline simd8<T> operator&(const simd8<T> other) const { return vandq_u8(*this, other); }
    really_inline simd8<T> operator^(const simd8<T> other) const { return veorq_u8(*this, other); }
    really_inline simd8<T> bit_andnot(const simd8<T> other) const { return vbicq_u8(*this, other); }
    really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }
    really_inline simd8<T>& operator|=(const simd8<T> other) { auto this_cast = (simd8<T>*)this; *this_cast = *this_cast | other; return *this_cast; }
    really_inline simd8<T>& operator&=(const simd8<T> other) { auto this_cast = (simd8<T>*)this; *this_cast = *this_cast & other; return *this_cast; }
    really_inline simd8<T>& operator^=(const simd8<T> other) { auto this_cast = (simd8<T>*)this; *this_cast = *this_cast ^ other; return *this_cast; }

    really_inline Mask operator==(const simd8<T> other) const { return vceqq_u8(*this, other); }

    template<int N=1>
    really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return vextq_u8(prev_chunk, *this, 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base_u8<bool> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    static really_inline simd8<bool> splat(bool _value) { return vmovq_n_u8(-(!!_value)); }

    really_inline simd8(const uint8x16_t _value) : base_u8<bool>(_value) {}
    // False constructor
    really_inline simd8() : simd8(vdupq_n_u8(0)) {}
    // Splat constructor
    really_inline simd8(bool _value) : simd8(splat(_value)) {}

    // We return uint32_t instead of uint16_t because that seems to be more efficient for most
    // purposes (cutting it down to uint16_t costs performance in some compilers).
    really_inline uint32_t to_bitmask() const {
      const uint8x16_t bit_mask = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                   0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
      auto minput = *this & bit_mask;
      uint8x16_t tmp = vpaddq_u8(minput, minput);
      tmp = vpaddq_u8(tmp, tmp);
      tmp = vpaddq_u8(tmp, tmp);
      return vgetq_lane_u16(vreinterpretq_u16_u8(tmp), 0);
    }
    really_inline bool any() const { return vmaxvq_u8(*this) != 0; }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base_u8<uint8_t> {
    static really_inline uint8x16_t splat(uint8_t _value) { return vmovq_n_u8(_value); }
    static really_inline uint8x16_t zero() { return vdupq_n_u8(0); }
    static really_inline uint8x16_t load(const uint8_t* values) { return vld1q_u8(values); }

    really_inline simd8(const uint8x16_t _value) : base_u8<uint8_t>(_value) {}
    // Zero constructor
    really_inline simd8() : simd8(zero()) {}
    // Array constructor
    really_inline simd8(const uint8_t values[16]) : simd8(load(values)) {}
    // Splat constructor
    really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Member-by-member initialization
    really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(uint8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Store to array
    really_inline void store(uint8_t dst[16]) const { return vst1q_u8(dst, *this); }

    // Saturated math
    really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return vqaddq_u8(*this, other); }
    really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return vqsubq_u8(*this, other); }

    // Addition/subtraction are the same for signed and unsigned
    really_inline simd8<uint8_t> operator+(const simd8<uint8_t> other) const { return vaddq_u8(*this, other); }
    really_inline simd8<uint8_t> operator-(const simd8<uint8_t> other) const { return vsubq_u8(*this, other); }
    really_inline simd8<uint8_t>& operator+=(const simd8<uint8_t> other) { *this = *this + other; return *this; }
    really_inline simd8<uint8_t>& operator-=(const simd8<uint8_t> other) { *this = *this - other; return *this; }

    // Order-specific operations
    really_inline uint8_t max() const { return vmaxvq_u8(*this); }
    really_inline uint8_t min() const { return vminvq_u8(*this); }
    really_inline simd8<uint8_t> max(const simd8<uint8_t> other) const { return vmaxq_u8(*this, other); }
    really_inline simd8<uint8_t> min(const simd8<uint8_t> other) const { return vminq_u8(*this, other); }
    really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return vcleq_u8(*this, other); }
    really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return vcgeq_u8(*this, other); }
    really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return vcltq_u8(*this, other); }
    really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return vcgtq_u8(*this, other); }
    // Same as >, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this > other); }
    // Same as <, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this < other); }

    // Bit-specific operations
    really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return vtstq_u8(*this, bits); }
    really_inline bool any_bits_set_anywhere() const { return this->max() != 0; }
    really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return (*this & bits).any_bits_set_anywhere(); }
    template<int N>
    really_inline simd8<uint8_t> shr() const { return vshrq_n_u8(*this, N); }
    template<int N>
    really_inline simd8<uint8_t> shl() const { return vshlq_n_u8(*this, N); }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
    }
    template<typename L>
    really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }

    template<typename T>
    really_inline simd8<uint8_t> apply_lookup_16_to(const simd8<T> original) {
      return vqtbl1q_u8(*this, simd8<uint8_t>(original));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> {
    int8x16_t value;

    static really_inline simd8<int8_t> splat(int8_t _value) { return vmovq_n_s8(_value); }
    static really_inline simd8<int8_t> zero() { return vdupq_n_s8(0); }
    static really_inline simd8<int8_t> load(const int8_t values[16]) { return vld1q_s8(values); }

    // Conversion from/to SIMD register
    really_inline simd8(const int8x16_t _value) : value{_value} {}
    really_inline operator const int8x16_t&() const { return this->value; }
    really_inline operator int8x16_t&() { return this->value; }

    // Zero constructor
    really_inline simd8() : simd8(zero()) {}
    // Splat constructor
    really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const int8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3, int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(int8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Store to array
    really_inline void store(int8_t dst[16]) const { return vst1q_s8(dst, *this); }

    // Explicit conversion to/from unsigned
    really_inline explicit simd8(const uint8x16_t other): simd8(vreinterpretq_s8_u8(other)) {}
    really_inline explicit operator simd8<uint8_t>() const { return vreinterpretq_u8_s8(*this); }

    // Math
    really_inline simd8<int8_t> operator+(const simd8<int8_t> other) const { return vaddq_s8(*this, other); }
    really_inline simd8<int8_t> operator-(const simd8<int8_t> other) const { return vsubq_s8(*this, other); }
    really_inline simd8<int8_t>& operator+=(const simd8<int8_t> other) { *this = *this + other; return *this; }
    really_inline simd8<int8_t>& operator-=(const simd8<int8_t> other) { *this = *this - other; return *this; }

    // Order-sensitive comparisons
    really_inline simd8<int8_t> max(const simd8<int8_t> other) const { return vmaxq_s8(*this, other); }
    really_inline simd8<int8_t> min(const simd8<int8_t> other) const { return vminq_s8(*this, other); }
    really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return vcgtq_s8(*this, other); }
    really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return vcltq_s8(*this, other); }
    really_inline simd8<bool> operator==(const simd8<int8_t> other) const { return vceqq_s8(*this, other); }

    template<int N=1>
    really_inline simd8<int8_t> prev(const simd8<int8_t> prev_chunk) const {
      return vextq_s8(prev_chunk, *this, 16 - N);
    }

    // Perform a lookup assuming no value is larger than 16
    template<typename L>
    really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
    }
    template<typename L>
    really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }

    template<typename T>
    really_inline simd8<int8_t> apply_lookup_16_to(const simd8<T> original) {
      return vqtbl1q_s8(*this, simd8<uint8_t>(original));
    }
  };

  template<typename T>
  struct simd8x64 {
    static const int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    const simd8<T> chunks[NUM_CHUNKS];

    really_inline simd8x64() : chunks{simd8<T>(), simd8<T>(), simd8<T>(), simd8<T>()} {}
    really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2, const simd8<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+16), simd8<T>::load(ptr+32), simd8<T>::load(ptr+48)} {}

    really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store(ptr+sizeof(simd8<T>)*3);
    }

    template <typename F>
    static really_inline void each_index(F const& each) {
      each(0);
      each(1);
      each(2);
      each(3);
    }

    template <typename F>
    really_inline void each(F const& each_chunk) const
    {
      each_chunk(this->chunks[0]);
      each_chunk(this->chunks[1]);
      each_chunk(this->chunks[2]);
      each_chunk(this->chunks[3]);
    }

    template <typename R=bool, typename F>
    really_inline simd8x64<R> map(F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0]),
        map_chunk(this->chunks[1]),
        map_chunk(this->chunks[2]),
        map_chunk(this->chunks[3])
      );
    }

    template <typename R=bool, typename F>
    really_inline simd8x64<R> map(const simd8x64<T> b, F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0], b.chunks[0]),
        map_chunk(this->chunks[1], b.chunks[1]),
        map_chunk(this->chunks[2], b.chunks[2]),
        map_chunk(this->chunks[3], b.chunks[3])
      );
    }

    template <typename F>
    really_inline simd8<T> reduce(F const& reduce_pair) const {
      return reduce_pair(
        reduce_pair(this->chunks[0], this->chunks[1]),
        reduce_pair(this->chunks[2], this->chunks[3])
      );
    }

    really_inline uint64_t to_bitmask() const {
      const uint8x16_t bit_mask = {
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      };
      // Add each of the elements next to each other, successively, to stuff each 8 byte mask into one.
      uint8x16_t sum0 = vpaddq_u8(this->chunks[0] & bit_mask, this->chunks[1] & bit_mask);
      uint8x16_t sum1 = vpaddq_u8(this->chunks[2] & bit_mask, this->chunks[3] & bit_mask);
      sum0 = vpaddq_u8(sum0, sum1);
      sum0 = vpaddq_u8(sum0, sum0);
      return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
    }

    really_inline simd8x64<T> bit_or(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a | mask; } );
    }

    really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a == mask; } ).to_bitmask();
    }

    really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a <= mask; } ).to_bitmask();
    }

  }; // struct simd8x64<T>

} // namespace simdjson::arm64::simd

#endif // IS_ARM64
#endif // SIMDJSON_ARM64_SIMD_H
/* end file src/arm64/simd.h */
/* begin file src/arm64/bitmanipulation.h */
#ifndef SIMDJSON_ARM64_BITMANIPULATION_H
#define SIMDJSON_ARM64_BITMANIPULATION_H


#ifdef IS_ARM64

/* arm64/intrinsics.h already included: #include "arm64/intrinsics.h" */

namespace simdjson::arm64 {

#ifndef _MSC_VER
// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out. 
__attribute__((no_sanitize("undefined"))) // this is deliberate
#endif
/* result might be undefined when input_num is zero */
really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  unsigned long ret;
  // Search the mask data from least significant bit (LSB) 
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else
  return __builtin_ctzll(input_num);
#endif// _MSC_VER
}

/* result might be undefined when input_num is zero */
really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
}

/* result might be undefined when input_num is zero */
really_inline int leading_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB) 
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif// _MSC_VER
}

/* result might be undefined when input_num is zero */
really_inline int hamming(uint64_t input_num) {
   return vaddv_u8(vcnt_u8((uint8x8_t)input_num));
}

really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef _MSC_VER
  // todo: this might fail under visual studio for ARM
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

#ifdef _MSC_VER
#pragma intrinsic(_umul128) // todo: this might fail under visual studio for ARM
#endif

really_inline bool mul_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef _MSC_VER
  // todo: this might fail under visual studio for ARM
  uint64_t high;
  *result = _umul128(value1, value2, &high);
  return high;
#else
  return __builtin_umulll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

}// namespace simdjson::arm64

#endif //IS_ARM64
#endif //  SIMDJSON_ARM64_BITMANIPULATION_H
/* end file src/arm64/bitmanipulation.h */

namespace simdjson::arm64 {

using namespace simd;

really_inline void find_whitespace_and_operators(
  const simd::simd8x64<uint8_t> in,
  uint64_t &whitespace, uint64_t &op) {

  auto v = in.map<uint8_t>([&](simd8<uint8_t> chunk) {
    auto nib_lo = chunk & 0xf;
    auto nib_hi = chunk.shr<4>();
    auto shuf_lo = nib_lo.lookup_16<uint8_t>(16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
    auto shuf_hi = nib_hi.lookup_16<uint8_t>(8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0);
    return shuf_lo & shuf_hi;
  });

  op = v.map([&](simd8<uint8_t> _v) { return _v.any_bits_set(0x7); }).to_bitmask();
  whitespace = v.map([&](simd8<uint8_t> _v) { return _v.any_bits_set(0x18); }).to_bitmask();
}

really_inline bool is_ascii(simd8x64<uint8_t> input) {
    simd8<uint8_t> bits = input.reduce([&](auto a,auto b) { return a|b; });
    return bits.max() < 0b10000000u;
}

really_inline simd8<bool> must_be_continuation(simd8<uint8_t> prev1, simd8<uint8_t> prev2, simd8<uint8_t> prev3) {
    simd8<bool> is_second_byte = prev1 >= uint8_t(0b11000000u);
    simd8<bool> is_third_byte  = prev2 >= uint8_t(0b11100000u);
    simd8<bool> is_fourth_byte = prev3 >= uint8_t(0b11110000u);
    // Use ^ instead of | for is_*_byte, because ^ is commutative, and the caller is using ^ as well.
    // This will work fine because we only have to report errors for cases with 0-1 lead bytes.
    // Multiple lead bytes implies 2 overlapping multibyte characters, and if that happens, there is
    // guaranteed to be at least *one* lead byte that is part of only 1 other multibyte character.
    // The error will be detected there.
    return is_second_byte ^ is_third_byte ^ is_fourth_byte;
}

/* begin file src/generic/utf8_lookup2_algorithm.h */
//
// Detect Unicode errors.
//
// UTF-8 is designed to allow multiple bytes and be compatible with ASCII. It's a fairly basic
// encoding that uses the first few bits on each byte to denote a "byte type", and all other bits
// are straight up concatenated into the final value. The first byte of a multibyte character is a
// "leading byte" and starts with N 1's, where N is the total number of bytes (110_____ = 2 byte
// lead). The remaining bytes of a multibyte character all start with 10. 1-byte characters just
// start with 0, because that's what ASCII looks like. Here's what each size 
//
// - ASCII (7 bits):              0_______
// - 2 byte character (11 bits):  110_____ 10______
// - 3 byte character (17 bits):  1110____ 10______ 10______
// - 4 byte character (23 bits):  11110___ 10______ 10______ 10______
// - 5+ byte character (illegal): 11111___ <illegal>
//
// There are 5 classes of error that can happen in Unicode:
//
// - TOO_SHORT: when you have a multibyte character with too few bytes (i.e. missing continuation).
//   We detect this by looking for new characters (lead bytes) inside the range of a multibyte
//   character.
//
//   e.g. 11000000 01100001 (2-byte character where second byte is ASCII)
//
// - TOO_LONG: when there are more bytes in your character than you need (i.e. extra continuation).
//   We detect this by requiring that the next byte after your multibyte character be a new
//   character--so a continuation after your character is wrong.
//
//   e.g. 11011111 10111111 10111111 (2-byte character followed by *another* continuation byte)
//
// - TOO_LARGE: Unicode only goes up to U+10FFFF. These characters are too large.
//
//   e.g. 11110111 10111111 10111111 10111111 (bigger than 10FFFF).
//
// - OVERLONG: multibyte characters with a bunch of leading zeroes, where you could have
//   used fewer bytes to make the same character. Like encoding an ASCII character in 4 bytes is
//   technically possible, but UTF-8 disallows it so that there is only one way to write an "a".
//
//   e.g. 11000001 10100001 (2-byte encoding of "a", which only requires 1 byte: 01100001)
//
// - SURROGATE: Unicode U+D800-U+DFFF is a *surrogate* character, reserved for use in UCS-2 and
//   WTF-8 encodings for characters with > 2 bytes. These are illegal in pure UTF-8.
//
//   e.g. 11101101 10100000 10000000 (U+D800)
//
// - INVALID_5_BYTE: 5-byte, 6-byte, 7-byte and 8-byte characters are unsupported; Unicode does not
//   support values with more than 23 bits (which a 4-byte character supports).
//
//   e.g. 11111000 10100000 10000000 10000000 10000000 (U+800000)
//   
// Legal utf-8 byte sequences per  http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94:
// 
//   Code Points        1st       2s       3s       4s
//  U+0000..U+007F     00..7F
//  U+0080..U+07FF     C2..DF   80..BF
//  U+0800..U+0FFF     E0       A0..BF   80..BF
//  U+1000..U+CFFF     E1..EC   80..BF   80..BF
//  U+D000..U+D7FF     ED       80..9F   80..BF
//  U+E000..U+FFFF     EE..EF   80..BF   80..BF
//  U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
//  U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
//  U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
//
using namespace simd;

namespace utf8_validation {

  //
  // Find special case UTF-8 errors where the character is technically readable (has the right length)
  // but the *value* is disallowed.
  //
  // This includes overlong encodings, surrogates and values too large for Unicode.
  //
  // It turns out the bad character ranges can all be detected by looking at the first 12 bits of the
  // UTF-8 encoded character (i.e. all of byte 1, and the high 4 bits of byte 2). This algorithm does a
  // 3 4-bit table lookups, identifying which errors that 4 bits could match, and then &'s them together.
  // If all 3 lookups detect the same error, it's an error.
  //
  really_inline simd8<uint8_t> check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1) {
    //
    // These are the errors we're going to match for bytes 1-2, by looking at the first three
    // nibbles of the character: <high bits of byte 1>> & <low bits of byte 1> & <high bits of byte 2>
    //
    static const int OVERLONG_2  = 0x01; // 1100000_ 10______ (technically we match 10______ but we could match ________, they both yield errors either way)
    static const int OVERLONG_3  = 0x02; // 11100000 100_____ ________
    static const int OVERLONG_4  = 0x04; // 11110000 1000____ ________ ________
    static const int SURROGATE   = 0x08; // 11101101 [101_]____
    static const int TOO_LARGE   = 0x10; // 11110100 (1001|101_)____
    static const int TOO_LARGE_2 = 0x20; // 1111(1___|011_|0101) 10______

    // After processing the rest of byte 1 (the low bits), we're still not done--we have to check
    // byte 2 to be sure which things are errors and which aren't.
    // Since high_bits is byte 5, byte 2 is high_bits.prev<3>
    static const int CARRY = OVERLONG_2 | TOO_LARGE_2;
    const simd8<uint8_t> byte_2_high = input.shr<4>().lookup_16<uint8_t>(
        // ASCII: ________ [0___]____
        CARRY, CARRY, CARRY, CARRY,
        // ASCII: ________ [0___]____
        CARRY, CARRY, CARRY, CARRY,
        // Continuations: ________ [10__]____
        CARRY | OVERLONG_3 | OVERLONG_4, // ________ [1000]____
        CARRY | OVERLONG_3 | TOO_LARGE,  // ________ [1001]____
        CARRY | TOO_LARGE  | SURROGATE,  // ________ [1010]____
        CARRY | TOO_LARGE  | SURROGATE,  // ________ [1011]____
        // Multibyte Leads: ________ [11__]____
        CARRY, CARRY, CARRY, CARRY
    );

    const simd8<uint8_t> byte_1_high = prev1.shr<4>().lookup_16<uint8_t>(
      // [0___]____ (ASCII)
      0, 0, 0, 0,                          
      0, 0, 0, 0,
      // [10__]____ (continuation)
      0, 0, 0, 0,
      // [11__]____ (2+-byte leads)
      OVERLONG_2, 0,                       // [110_]____ (2-byte lead)
      OVERLONG_3 | SURROGATE,              // [1110]____ (3-byte lead)
      OVERLONG_4 | TOO_LARGE | TOO_LARGE_2 // [1111]____ (4+-byte lead)
    );

    const simd8<uint8_t> byte_1_low = (prev1 & 0x0F).lookup_16<uint8_t>(
      // ____[00__] ________
      OVERLONG_2 | OVERLONG_3 | OVERLONG_4, // ____[0000] ________
      OVERLONG_2,                           // ____[0001] ________
      0, 0,
      // ____[01__] ________
      TOO_LARGE,                            // ____[0100] ________
      TOO_LARGE_2,
      TOO_LARGE_2,
      TOO_LARGE_2,
      // ____[10__] ________
      TOO_LARGE_2, TOO_LARGE_2, TOO_LARGE_2, TOO_LARGE_2,
      // ____[11__] ________
      TOO_LARGE_2,
      TOO_LARGE_2 | SURROGATE,                            // ____[1101] ________
      TOO_LARGE_2, TOO_LARGE_2
    );

    return byte_1_high & byte_1_low & byte_2_high;
  }

  //
  // Validate the length of multibyte characters (that each multibyte character has the right number
  // of continuation characters, and that all continuation characters are part of a multibyte
  // character).
  //
  // Algorithm
  // =========
  //
  // This algorithm compares *expected* continuation characters with *actual* continuation bytes,
  // and emits an error anytime there is a mismatch.
  //
  // For example, in the string "𝄞₿֏ab", which has a 4-, 3-, 2- and 1-byte
  // characters, the file will look like this:
  //
  // | Character             | 𝄞  |    |    |    | ₿  |    |    | ֏  |    | a  | b  |
  // |-----------------------|----|----|----|----|----|----|----|----|----|----|----|
  // | Character Length      |  4 |    |    |    |  3 |    |    |  2 |    |  1 |  1 |
  // | Byte                  | F0 | 9D | 84 | 9E | E2 | 82 | BF | D6 | 8F | 61 | 62 |
  // | is_second_byte        |    |  X |    |    |    |  X |    |    |  X |    |    |
  // | is_third_byte         |    |    |  X |    |    |    |  X |    |    |    |    |
  // | is_fourth_byte        |    |    |    |  X |    |    |    |    |    |    |    |
  // | expected_continuation |    |  X |  X |  X |    |  X |  X |    |  X |    |    |
  // | is_continuation       |    |  X |  X |  X |    |  X |  X |    |  X |    |    |
  //
  // The errors here are basically (Second Byte OR Third Byte OR Fourth Byte == Continuation):
  //
  // - **Extra Continuations:** Any continuation that is not a second, third or fourth byte is not
  //   part of a valid 2-, 3- or 4-byte character and is thus an error. It could be that it's just
  //   floating around extra outside of any character, or that there is an illegal 5-byte character,
  //   or maybe it's at the beginning of the file before any characters have started; but it's an
  //   error in all these cases.
  // - **Missing Continuations:** Any second, third or fourth byte that *isn't* a continuation is an error, because that means
  //   we started a new character before we were finished with the current one.
  //
  // Getting the Previous Bytes
  // --------------------------
  //
  // Because we want to know if a byte is the *second* (or third, or fourth) byte of a multibyte
  // character, we need to "shift the bytes" to find that out. This is what they mean:
  //
  // - `is_continuation`: if the current byte is a continuation.
  // - `is_second_byte`: if 1 byte back is the start of a 2-, 3- or 4-byte character.
  // - `is_third_byte`: if 2 bytes back is the start of a 3- or 4-byte character.
  // - `is_fourth_byte`: if 3 bytes back is the start of a 4-byte character.
  //
  // We use shuffles to go n bytes back, selecting part of the current `input` and part of the
  // `prev_input` (search for `.prev<1>`, `.prev<2>`, etc.). These are passed in by the caller
  // function, because the 1-byte-back data is used by other checks as well.
  //
  // Getting the Continuation Mask
  // -----------------------------
  //
  // Once we have the right bytes, we have to get the masks. To do this, we treat UTF-8 bytes as
  // numbers, using signed `<` and `>` operations to check if they are continuations or leads.
  // In fact, we treat the numbers as *signed*, partly because it helps us, and partly because
  // Intel's SIMD presently only offers signed `<` and `>` operations (not unsigned ones).
  //
  // In UTF-8, bytes that start with the bits 110, 1110 and 11110 are 2-, 3- and 4-byte "leads,"
  // respectively, meaning they expect to have 1, 2 and 3 "continuation bytes" after them.
  // Continuation bytes start with 10, and ASCII (1-byte characters) starts with 0.
  //
  // When treated as signed numbers, they look like this:
  //
  // | Type         | High Bits  | Binary Range | Signed |
  // |--------------|------------|--------------|--------|
  // | ASCII        | `0`        | `01111111`   |   127  |
  // |              |            | `00000000`   |     0  |
  // | 4+-Byte Lead | `1111`     | `11111111`   |    -1  |
  // |              |            | `11110000    |   -16  |
  // | 3-Byte Lead  | `1110`     | `11101111`   |   -17  |
  // |              |            | `11100000    |   -32  |
  // | 2-Byte Lead  | `110`      | `11011111`   |   -33  |
  // |              |            | `11000000    |   -64  |
  // | Continuation | `10`       | `10111111`   |   -65  |
  // |              |            | `10000000    |  -128  |
  //
  // This makes it pretty easy to get the continuation mask! It's just a single comparison:
  //
  // ```
  // is_continuation = input < -64`
  // ```
  //
  // We can do something similar for the others, but it takes two comparisons instead of one: "is
  // the start of a 4-byte character" is `< -32` and `> -65`, for example. And 2+ bytes is `< 0` and
  // `> -64`. Surely we can do better, they're right next to each other!
  //
  // Getting the is_xxx Masks: Shifting the Range
  // --------------------------------------------
  //
  // Notice *why* continuations were a single comparison. The actual *range* would require two
  // comparisons--`< -64` and `> -129`--but all characters are always greater than -128, so we get
  // that for free. In fact, if we had *unsigned* comparisons, 2+, 3+ and 4+ comparisons would be
  // just as easy: 4+ would be `> 239`, 3+ would be `> 223`, and 2+ would be `> 191`.
  //
  // Instead, we add 128 to each byte, shifting the range up to make comparison easy. This wraps
  // ASCII down into the negative, and puts 4+-Byte Lead at the top:
  //
  // | Type                 | High Bits  | Binary Range | Signed |
  // |----------------------|------------|--------------|-------|
  // | 4+-Byte Lead (+ 127) | `0111`     | `01111111`   |   127 |
  // |                      |            | `01110000    |   112 |
  // |----------------------|------------|--------------|-------|
  // | 3-Byte Lead (+ 127)  | `0110`     | `01101111`   |   111 |
  // |                      |            | `01100000    |    96 |
  // |----------------------|------------|--------------|-------|
  // | 2-Byte Lead (+ 127)  | `010`      | `01011111`   |    95 |
  // |                      |            | `01000000    |    64 |
  // |----------------------|------------|--------------|-------|
  // | Continuation (+ 127) | `00`       | `00111111`   |    63 |
  // |                      |            | `00000000    |     0 |
  // |----------------------|------------|--------------|-------|
  // | ASCII (+ 127)        | `1`        | `11111111`   |    -1 |
  // |                      |            | `10000000`   |  -128 |
  // |----------------------|------------|--------------|-------|
  // 
  // *Now* we can use signed `>` on all of them:
  //
  // ```
  // prev1 = input.prev<1>
  // prev2 = input.prev<2>
  // prev3 = input.prev<3>
  // prev1_flipped = input.prev<1>(prev_input) ^ 0x80; // Same as `+ 128`
  // prev2_flipped = input.prev<2>(prev_input) ^ 0x80; // Same as `+ 128`
  // prev3_flipped = input.prev<3>(prev_input) ^ 0x80; // Same as `+ 128`
  // is_second_byte = prev1_flipped > 63;  // 2+-byte lead
  // is_third_byte  = prev2_flipped > 95;  // 3+-byte lead
  // is_fourth_byte = prev3_flipped > 111; // 4+-byte lead
  // ```
  //
  // NOTE: we use `^ 0x80` instead of `+ 128` in the code, which accomplishes the same thing, and even takes the same number
  // of cycles as `+`, but on many Intel architectures can be parallelized better (you can do 3
  // `^`'s at a time on Haswell, but only 2 `+`'s).
  //
  // That doesn't look like it saved us any instructions, did it? Well, because we're adding the
  // same number to all of them, we can save one of those `+ 128` operations by assembling
  // `prev2_flipped` out of prev 1 and prev 3 instead of assembling it from input and adding 128
  // to it. One more instruction saved!
  //
  // ```
  // prev1 = input.prev<1>
  // prev3 = input.prev<3>
  // prev1_flipped = prev1 ^ 0x80; // Same as `+ 128`
  // prev3_flipped = prev3 ^ 0x80; // Same as `+ 128`
  // prev2_flipped = prev1_flipped.concat<2>(prev3_flipped): // <shuffle: take the first 2 bytes from prev1 and the rest from prev3  
  // ```
  //
  // ### Bringing It All Together: Detecting the Errors
  //
  // At this point, we have `is_continuation`, `is_first_byte`, `is_second_byte` and `is_third_byte`.
  // All we have left to do is check if they match!
  //
  // ```
  // return (is_second_byte | is_third_byte | is_fourth_byte) ^ is_continuation;
  // ```
  //
  // But wait--there's more. The above statement is only 3 operations, but they *cannot be done in
  // parallel*. You have to do 2 `|`'s and then 1 `&`. Haswell, at least, has 3 ports that can do
  // bitwise operations, and we're only using 1!
  //
  // Epilogue: Addition For Booleans
  // -------------------------------
  //
  // There is one big case the above code doesn't explicitly talk about--what if is_second_byte
  // and is_third_byte are BOTH true? That means there is a 3-byte and 2-byte character right next
  // to each other (or any combination), and the continuation could be part of either of them!
  // Our algorithm using `&` and `|` won't detect that the continuation byte is problematic.
  //
  // Never fear, though. If that situation occurs, we'll already have detected that the second
  // leading byte was an error, because it was supposed to be a part of the preceding multibyte
  // character, but it *wasn't a continuation*.
  //
  // We could stop here, but it turns out that we can fix it using `+` and `-` instead of `|` and
  // `&`, which is both interesting and possibly useful (even though we're not using it here). It
  // exploits the fact that in SIMD, a *true* value is -1, and a *false* value is 0. So those
  // comparisons were giving us numbers!
  //
  // Given that, if you do `is_second_byte + is_third_byte + is_fourth_byte`, under normal
  // circumstances you will either get 0 (0 + 0 + 0) or -1 (-1 + 0 + 0, etc.). Thus,
  // `(is_second_byte + is_third_byte + is_fourth_byte) - is_continuation` will yield 0 only if
  // *both* or *neither* are 0 (0-0 or -1 - -1). You'll get 1 or -1 if they are different. Because
  // *any* nonzero value is treated as an error (not just -1), we're just fine here :)
  //
  // Further, if *more than one* multibyte character overlaps,
  // `is_second_byte + is_third_byte + is_fourth_byte` will be -2 or -3! Subtracting `is_continuation`
  // from *that* is guaranteed to give you a nonzero value (-1, -2 or -3). So it'll always be
  // considered an error.
  //
  // One reason you might want to do this is parallelism. ^ and | are not associative, so
  // (A | B | C) ^ D will always be three operations in a row: either you do A | B -> | C -> ^ D, or
  // you do B | C -> | A -> ^ D. But addition and subtraction *are* associative: (A + B + C) - D can
  // be written as `(A + B) + (C - D)`. This means you can do A + B and C - D at the same time, and
  // then adds the result together. Same number of operations, but if the processor can run
  // independent things in parallel (which most can), it runs faster.
  //
  // This doesn't help us on Intel, but might help us elsewhere: on Haswell, at least, | and ^ have
  // a super nice advantage in that more of them can be run at the same time (they can run on 3
  // ports, while + and - can run on 2)! This means that we can do A | B while we're still doing C,
  // saving us the cycle we would have earned by using +. Even more, using an instruction with a
  // wider array of ports can help *other* code run ahead, too, since these instructions can "get
  // out of the way," running on a port other instructions can't.
  // 
  // Epilogue II: One More Trick
  // ---------------------------
  //
  // There's one more relevant trick up our sleeve, it turns out: it turns out on Intel we can "pay
  // for" the (prev<1> + 128) instruction, because it can be used to save an instruction in
  // check_special_cases()--but we'll talk about that there :)
  //
  really_inline simd8<uint8_t> check_multibyte_lengths(simd8<uint8_t> input, simd8<uint8_t> prev_input, simd8<uint8_t> prev1) {
    simd8<uint8_t> prev2 = input.prev<2>(prev_input);
    simd8<uint8_t> prev3 = input.prev<3>(prev_input);

    // Cont is 10000000-101111111 (-65...-128)
    simd8<bool> is_continuation = simd8<int8_t>(input) < int8_t(-64);
    // must_be_continuation is architecture-specific because Intel doesn't have unsigned comparisons
    return simd8<uint8_t>(must_be_continuation(prev1, prev2, prev3) ^ is_continuation);
  }

  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  really_inline simd8<uint8_t> is_incomplete(simd8<uint8_t> input) {
    // If the previous input's last 3 bytes match this, they're too short (they ended at EOF):
    // ... 1111____ 111_____ 11______
    static const uint8_t max_array[32] = {
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 0b11110000u-1, 0b11100000u-1, 0b11000000u-1
    };
    const simd8<uint8_t> max_value(&max_array[sizeof(max_array)-sizeof(simd8<uint8_t>)]);
    return input.gt_bits(max_value);
  }

  struct utf8_checker {
    // If this is nonzero, there has been a UTF-8 error.
    simd8<uint8_t> error;
    // The last input we received
    simd8<uint8_t> prev_input_block;
    // Whether the last input we received was incomplete (used for ASCII fast path)
    simd8<uint8_t> prev_incomplete;

    //
    // Check whether the current bytes are valid UTF-8.
    //
    really_inline void check_utf8_bytes(const simd8<uint8_t> input, const simd8<uint8_t> prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      simd8<uint8_t> prev1 = input.prev<1>(prev_input);
      this->error |= check_special_cases(input, prev1);
      this->error |= check_multibyte_lengths(input, prev_input, prev1);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short.
    really_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error |= this->prev_incomplete;
    }

    really_inline void check_next_input(simd8x64<uint8_t> input) {
      if (likely(is_ascii(input))) {
        // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
        // possibly finish them.
        this->error |= this->prev_incomplete;
      } else {
        this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
        for (int i=1; i<simd8x64<uint8_t>::NUM_CHUNKS; i++) {
          this->check_utf8_bytes(input.chunks[i], input.chunks[i-1]);
        }
        this->prev_incomplete = is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1]);
        this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1];
      }
    }

    really_inline ErrorValues errors() {
      return this->error.any_bits_set_anywhere() ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
    }

  }; // struct utf8_checker
}

using utf8_validation::utf8_checker;
/* end file src/generic/utf8_lookup2_algorithm.h */
/* begin file src/generic/stage1_find_marks.h */
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

namespace stage1 {

class bit_indexer {
public:
  uint32_t *tail;

  bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // base_ptr[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so base_ptr
  // needs to be large enough to handle this
  really_inline void write_indexes(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
        return;
    uint32_t cnt = hamming(bits);

    // Do the first 8 all together
    for (int i=0; i<8; i++) {
      this->tail[i] = idx + trailing_zeroes(bits);
      bits = clear_lowest_bit(bits);
    }

    // Do the next 8 all together (we hope in most cases it won't happen at all
    // and the branch is easily predicted).
    if (unlikely(cnt > 8)) {
      for (int i=8; i<16; i++) {
        this->tail[i] = idx + trailing_zeroes(bits);
        bits = clear_lowest_bit(bits);
      }

      // Most files don't have 16+ structurals per block, so we take several basically guaranteed
      // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
      // or the start of a value ("abc" true 123) every four characters.
      if (unlikely(cnt > 16)) {
        uint32_t i = 16;
        do {
          this->tail[i] = idx + trailing_zeroes(bits);
          bits = clear_lowest_bit(bits);
          i++;
        } while (i < cnt);
      }
    }

    this->tail += cnt;
  }
};

class json_structural_scanner {
public:
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
  uint64_t prev_structurals = 0;
  // Errors with unescaped characters in strings (ASCII codepoints < 0x20)
  uint64_t unescaped_chars_error = 0;
  bit_indexer structural_indexes;

  json_structural_scanner(uint32_t *_structural_indexes) : structural_indexes{_structural_indexes} {}

  //
  // Finish the scan and return any errors.
  //
  // This may detect errors as well, such as unclosed string and certain UTF-8 errors.
  // if streaming is set to true, an unclosed string is allowed.
  //
  really_inline ErrorValues detect_errors_on_eof(bool streaming = false);

  //
  // Return a mask of all string characters plus end quotes.
  //
  // prev_escaped is overflow saying whether the next character is escaped.
  // prev_in_string is overflow saying whether we're still in a string.
  //
  // Backslash sequences outside of quotes will be detected in stage 2.
  //
  really_inline uint64_t find_strings(const simd::simd8x64<uint8_t> in);

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
  really_inline uint64_t find_potential_structurals(const simd::simd8x64<uint8_t> in);

  //
  // Find the important bits of JSON in a STEP_SIZE-byte chunk, and add them to structural_indexes.
  //
  template<size_t STEP_SIZE>
  really_inline void scan_step(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker);

  //
  // Parse the entire input in STEP_SIZE-byte chunks.
  //
  template<size_t STEP_SIZE>
  really_inline void scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker);
};

// Routines to print masks and text for debugging bitmask operations
UNUSED static char * format_input_text(const simd8x64<uint8_t> in) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  in.store((uint8_t*)buf);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    if (buf[i] < ' ') { buf[i] = '_'; }
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

UNUSED static char * format_mask(uint64_t mask) {
  static char *buf = (char*)malloc(64 + 1);
  for (size_t i=0; i<64; i++) {
    buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
  }
  buf[64] = '\0';
  return buf;
}

//
// Finds escaped characters (characters following \).
//
// Handles runs of backslashes like \\\" and \\\\" correctly (yielding 0101 and 01010, respectively).
//
// Does this by:
// - Shift the escape mask to get potentially escaped characters (characters after backslashes).
// - Mask escaped sequences that start on *even* bits with 1010101010 (odd bits are escaped, even bits are not)
// - Mask escaped sequences that start on *odd* bits with 0101010101 (even bits are escaped, odd bits are not)
//
// To distinguish between escaped sequences starting on even/odd bits, it finds the start of all
// escape sequences, filters out the ones that start on even bits, and adds that to the mask of
// escape sequences. This causes the addition to clear out the sequences starting on odd bits (since
// the start bit causes a carry), and leaves even-bit sequences alone.
//
// Example:
//
// text           |  \\\ | \\\"\\\" \\\" \\"\\" |
// escape         |  xxx |  xx xxx  xxx  xx xx  | Removed overflow backslash; will | it into follows_escape
// odd_starts     |  x   |  x       x       x   | escape & ~even_bits & ~follows_escape
// even_seq       |     c|    cxxx     c xx   c | c = carry bit -- will be masked out later
// invert_mask    |      |     cxxx     c xx   c| even_seq << 1
// follows_escape |   xx | x xx xxx  xxx  xx xx | Includes overflow bit
// escaped        |   x  | x x  x x  x x  x  x  |
// desired        |   x  | x x  x x  x x  x  x  |
// text           |  \\\ | \\\"\\\" \\\" \\"\\" |
//
really_inline uint64_t find_escaped(uint64_t escape, uint64_t &escaped_overflow) {
  // If there was overflow, pretend the first character isn't a backslash
  escape &= ~escaped_overflow;
  uint64_t follows_escape = escape << 1 | escaped_overflow;

  // Get sequences starting on even bits by clearing out the odd series using +
  const uint64_t even_bits = 0x5555555555555555ULL;
  uint64_t odd_sequence_starts = escape & ~even_bits & ~follows_escape;
  uint64_t sequences_starting_on_even_bits;
  escaped_overflow = add_overflow(odd_sequence_starts, escape, &sequences_starting_on_even_bits);
  uint64_t invert_mask = sequences_starting_on_even_bits << 1; // The mask we want to return is the *escaped* bits, not escapes.

  // Mask every other backslashed character as an escaped character
  // Flip the mask for sequences that start on even bits, to correct them
  return (even_bits ^ invert_mask) & follows_escape;
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
really_inline uint64_t follows(const uint64_t match, const uint64_t filler, uint64_t &overflow) {
  uint64_t follows_match = follows(match, overflow);
  uint64_t result;
  overflow |= add_overflow(follows_match, filler, &result);
  return result;
}

really_inline ErrorValues json_structural_scanner::detect_errors_on_eof(bool streaming) {
  if ((prev_in_string) and (not streaming)) {
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
really_inline uint64_t json_structural_scanner::find_strings(const simd::simd8x64<uint8_t> in) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = find_escaped(backslash, prev_escaped);
  const uint64_t quote = in.eq('"') & ~escaped;
  // prefix_xor flips on bits inside the string (and flips off the end quote).
  const uint64_t in_string = prefix_xor(quote) ^ prev_in_string;
  /* right shift of a signed value expected to be well-defined and standard
  * compliant as of C++20,
  * John Regher from Utah U. says this is fine code */
  prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
  // Use ^ to turn the beginning quote off, and the end quote on.
  return in_string ^ quote;
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
really_inline uint64_t json_structural_scanner::find_potential_structurals(const simd::simd8x64<uint8_t> in) {
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

//
// Find the important bits of JSON in a 128-byte chunk, and add them to structural_indexes.
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
template<>
really_inline void json_structural_scanner::scan_step<128>(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker) {
  //
  // Load up all 128 bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);
  simd::simd8x64<uint8_t> in_2(buf+64);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = this->find_strings(in_1);
  uint64_t structurals_1 = this->find_potential_structurals(in_1);
  uint64_t string_2 = this->find_strings(in_2);
  uint64_t structurals_2 = this->find_potential_structurals(in_2);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_1 & ~string_1;
  this->unescaped_chars_error |= unescaped_1 & string_1;

  uint64_t unescaped_2 = in_2.lteq(0x1F);
  utf8_checker.check_next_input(in_2);
  this->structural_indexes.write_indexes(idx, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_2 & ~string_2;
  this->unescaped_chars_error |= unescaped_2 & string_2;
}

//
// Find the important bits of JSON in a 64-byte chunk, and add them to structural_indexes.
//
template<>
really_inline void json_structural_scanner::scan_step<64>(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker) {
  //
  // Load up bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = this->find_strings(in_1);
  uint64_t structurals_1 = this->find_potential_structurals(in_1);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_1 & ~string_1;
  this->unescaped_chars_error |= unescaped_1 & string_1;
}

template<size_t STEP_SIZE>
really_inline void json_structural_scanner::scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker) {
  size_t lenminusstep = len < STEP_SIZE ? 0 : len - STEP_SIZE;
  size_t idx = 0;

  for (; idx < lenminusstep; idx += STEP_SIZE) {
    this->scan_step<STEP_SIZE>(&buf[idx], idx, utf8_checker);
  }

  /* If we have a final chunk of less than STEP_SIZE bytes, pad it to STEP_SIZE with
  * spaces  before processing it (otherwise, we risk invalidating the UTF-8
  * checks). */
  if (likely(idx < len)) {
    uint8_t tmp_buf[STEP_SIZE];
    memset(tmp_buf, 0x20, STEP_SIZE);
    memcpy(tmp_buf, buf + idx, len - idx);
    this->scan_step<STEP_SIZE>(&tmp_buf[0], idx, utf8_checker);
    idx += STEP_SIZE;
  }

  /* finally, flatten out the remaining structurals from the last iteration */
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals);
}

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
template<size_t STEP_SIZE>
int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  if (unlikely(len > pj.byte_capacity)) {
    return simdjson::CAPACITY;
  }
  utf8_checker utf8_checker{};
  json_structural_scanner scanner{pj.structural_indexes.get()};
  scanner.scan<STEP_SIZE>(buf, len, utf8_checker);
  // we might tolerate an unclosed string if streaming is true
  simdjson::ErrorValues error = scanner.detect_errors_on_eof(streaming);
  if (unlikely(error != simdjson::SUCCESS)) {
    return error;
  }
  pj.n_structural_indexes = scanner.structural_indexes.tail - pj.structural_indexes.get();
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
  return utf8_checker.errors();
}

} // namespace stage1
/* end file src/generic/stage1_find_marks.h */

} // namespace simdjson::arm64

namespace simdjson {

template <>
int find_structural_bits<Architecture::ARM64>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  return arm64::stage1::find_structural_bits<64>(buf, len, pj, streaming);
}

} // namespace simdjson

#endif // IS_ARM64
#endif // SIMDJSON_ARM64_STAGE1_FIND_MARKS_H
/* end file src/generic/stage1_find_marks.h */
/* begin file src/haswell/stage1_find_marks.h */
#ifndef SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H
#define SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H


#ifdef IS_X86_64

/* begin file src/haswell/bitmask.h */
#ifndef SIMDJSON_HASWELL_BITMASK_H
#define SIMDJSON_HASWELL_BITMASK_H


#ifdef IS_X86_64

/* begin file src/haswell/intrinsics.h */
#ifndef SIMDJSON_HASWELL_INTRINSICS_H
#define SIMDJSON_HASWELL_INTRINSICS_H

#ifdef IS_X86_64

#ifdef _MSC_VER
#include <intrin.h> // visual studio
#else
#include <x86intrin.h> // elsewhere
#endif //  _MSC_VER
#endif //  IS_X86_64
#endif //  SIMDJSON_HASWELL_INTRINSICS_H
/* end file src/haswell/intrinsics.h */

TARGET_HASWELL
namespace simdjson::haswell {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
really_inline uint64_t prefix_xor(const uint64_t bitmask) {
  // There should be no such thing with a processor supporting avx2
  // but not clmul.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return _mm_cvtsi128_si64(result);
}

} // namespace simdjson::haswell
UNTARGET_REGION

#endif // IS_X86_64
#endif
/* end file src/haswell/intrinsics.h */
/* begin file src/haswell/simd.h */
#ifndef SIMDJSON_HASWELL_SIMD_H
#define SIMDJSON_HASWELL_SIMD_H


#ifdef IS_X86_64

/* haswell/intrinsics.h already included: #include "haswell/intrinsics.h" */

TARGET_HASWELL
namespace simdjson::haswell::simd {

  // Forward-declared so they can be used by splat and friends.
  template<typename Child>
  struct base {
    __m256i value;

    // Zero constructor
    really_inline base() : value{__m256i()} {}

    // Conversion from SIMD register
    really_inline base(const __m256i _value) : value(_value) {}

    // Conversion to SIMD register
    really_inline operator const __m256i&() const { return this->value; }
    really_inline operator __m256i&() { return this->value; }

    // Bit operations
    really_inline Child operator|(const Child other) const { return _mm256_or_si256(*this, other); }
    really_inline Child operator&(const Child other) const { return _mm256_and_si256(*this, other); }
    really_inline Child operator^(const Child other) const { return _mm256_xor_si256(*this, other); }
    really_inline Child bit_andnot(const Child other) const { return _mm256_andnot_si256(other, *this); }
    really_inline Child operator~() const { return *this ^ 0xFFu; }
    really_inline Child& operator|=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast | other; return *this_cast; }
    really_inline Child& operator&=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast & other; return *this_cast; }
    really_inline Child& operator^=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    typedef uint32_t bitmask_t;
    typedef uint64_t bitmask2_t;

    really_inline base8() : base<simd8<T>>() {}
    really_inline base8(const __m256i _value) : base<simd8<T>>(_value) {}

    really_inline Mask operator==(const simd8<T> other) const { return _mm256_cmpeq_epi8(*this, other); }

    static const int SIZE = sizeof(base<T>::value);

    template<int N=1>
    really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return _mm256_alignr_epi8(*this, _mm256_permute2x128_si256(prev_chunk, *this, 0x21), 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static really_inline simd8<bool> splat(bool _value) { return _mm256_set1_epi8(-(!!_value)); }

    really_inline simd8<bool>() : base8() {}
    really_inline simd8<bool>(const __m256i _value) : base8<bool>(_value) {}
    // Splat constructor
    really_inline simd8<bool>(bool _value) : base8<bool>(splat(_value)) {}

    really_inline int to_bitmask() const { return _mm256_movemask_epi8(*this); }
    really_inline bool any() const { return !_mm256_testz_si256(*this, *this); }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static really_inline simd8<T> splat(T _value) { return _mm256_set1_epi8(_value); }
    static really_inline simd8<T> zero() { return _mm256_setzero_si256(); }
    static really_inline simd8<T> load(const T values[32]) {
      return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(values));
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static really_inline simd8<T> repeat_16(
      T v0,  T v1,  T v2,  T v3,  T v4,  T v5,  T v6,  T v7,
      T v8,  T v9,  T v10, T v11, T v12, T v13, T v14, T v15
    ) {
      return simd8<T>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    really_inline base8_numeric() : base8<T>() {}
    really_inline base8_numeric(const __m256i _value) : base8<T>(_value) {}

    // Store to array
    really_inline void store(T dst[32]) const { return _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), *this); }

    // Addition/subtraction are the same for signed and unsigned
    really_inline simd8<T> operator+(const simd8<T> other) const { return _mm256_add_epi8(*this, other); }
    really_inline simd8<T> operator-(const simd8<T> other) const { return _mm256_sub_epi8(*this, other); }
    really_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *(simd8<T>*)this; }
    really_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *(simd8<T>*)this; }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return _mm256_shuffle_epi8(lookup_table, *this);
    }
    template<typename L>
    really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> : base8_numeric<int8_t> {
    really_inline simd8() : base8_numeric<int8_t>() {}
    really_inline simd8(const __m256i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const int8_t values[32]) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15,
      int8_t v16, int8_t v17, int8_t v18, int8_t v19, int8_t v20, int8_t v21, int8_t v22, int8_t v23,
      int8_t v24, int8_t v25, int8_t v26, int8_t v27, int8_t v28, int8_t v29, int8_t v30, int8_t v31
    ) : simd8(_mm256_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15,
      v16,v17,v18,v19,v20,v21,v22,v23,
      v24,v25,v26,v27,v28,v29,v30,v31
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Order-sensitive comparisons
    really_inline simd8<int8_t> max(const simd8<int8_t> other) const { return _mm256_max_epi8(*this, other); }
    really_inline simd8<int8_t> min(const simd8<int8_t> other) const { return _mm256_min_epi8(*this, other); }
    really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return _mm256_cmpgt_epi8(*this, other); }
    really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return _mm256_cmpgt_epi8(other, *this); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    really_inline simd8() : base8_numeric<uint8_t>() {}
    really_inline simd8(const __m256i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const uint8_t values[32]) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15,
      uint8_t v16, uint8_t v17, uint8_t v18, uint8_t v19, uint8_t v20, uint8_t v21, uint8_t v22, uint8_t v23,
      uint8_t v24, uint8_t v25, uint8_t v26, uint8_t v27, uint8_t v28, uint8_t v29, uint8_t v30, uint8_t v31
    ) : simd8(_mm256_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15,
      v16,v17,v18,v19,v20,v21,v22,v23,
      v24,v25,v26,v27,v28,v29,v30,v31
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Saturated math
    really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return _mm256_adds_epu8(*this, other); }
    really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return _mm256_subs_epu8(*this, other); }

    // Order-specific operations
    really_inline simd8<uint8_t> max(const simd8<uint8_t> other) const { return _mm256_max_epu8(*this, other); }
    really_inline simd8<uint8_t> min(const simd8<uint8_t> other) const { return _mm256_min_epu8(other, *this); }
    // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
    really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return this->saturating_sub(other); }
    // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
    really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return other.saturating_sub(*this); }
    really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return other.max(*this) == other; }
    really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return other.min(*this) == other; }
    really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }
    really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return this->lt_bits(other).any_bits_set(); }

    // Bit-specific operations
    really_inline simd8<bool> bits_not_set() const { return *this == uint8_t(0); }
    really_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const { return (*this & bits).bits_not_set(); }
    really_inline simd8<bool> any_bits_set() const { return ~this->bits_not_set(); }
    really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return ~this->bits_not_set(bits); }
    really_inline bool bits_not_set_anywhere() const { return _mm256_testz_si256(*this, *this); }
    really_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const { return _mm256_testz_si256(*this, bits); }
    really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    really_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(_mm256_srli_epi16(*this, N)) & uint8_t(0xFFu >> N); }
    template<int N>
    really_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(_mm256_slli_epi16(*this, N)) & uint8_t(0xFFu << N); }
    // Get one of the bits and make a bitmask out of it.
    // e.g. value.get_bit<7>() gets the high bit
    template<int N>
    really_inline int get_bit() const { return _mm256_movemask_epi8(_mm256_slli_epi16(*this, 7-N)); }
  };

  template<typename T>
  struct simd8x64 {
    static const int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    const simd8<T> chunks[NUM_CHUNKS];

    really_inline simd8x64() : chunks{simd8<T>(), simd8<T>()} {}
    really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1) : chunks{chunk0, chunk1} {}
    really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+32)} {}

    template <typename F>
    static really_inline void each_index(F const& each) {
      each(0);
      each(1);
    }

    really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
    }

    template <typename F>
    really_inline void each(F const& each_chunk) const
    {
      each_chunk(this->chunks[0]);
      each_chunk(this->chunks[1]);
    }

    template <typename R=bool, typename F>
    really_inline simd8x64<R> map(F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0]),
        map_chunk(this->chunks[1])
      );
    }

    template <typename R=bool, typename F>
    really_inline simd8x64<R> map(const simd8x64<uint8_t> b, F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0], b.chunks[0]),
        map_chunk(this->chunks[1], b.chunks[1])
      );
    }

    template <typename F>
    really_inline simd8<T> reduce(F const& reduce_pair) const {
      return reduce_pair(this->chunks[0], this->chunks[1]);
    }

    really_inline uint64_t to_bitmask() const {
      uint64_t r_lo = static_cast<uint32_t>(this->chunks[0].to_bitmask());
      uint64_t r_hi =                       this->chunks[1].to_bitmask();
      return r_lo | (r_hi << 32);
    }

    really_inline simd8x64<T> bit_or(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a | mask; } );
    }

    really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a == mask; } ).to_bitmask();
    }

    really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a <= mask; } ).to_bitmask();
    }

  }; // struct simd8x64<T>

} // namespace simdjson::haswell::simd
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_HASWELL_SIMD_H
/* end file src/haswell/simd.h */
/* begin file src/haswell/bitmanipulation.h */
#ifndef SIMDJSON_HASWELL_BITMANIPULATION_H
#define SIMDJSON_HASWELL_BITMANIPULATION_H


#ifdef IS_X86_64
/* haswell/intrinsics.h already included: #include "haswell/intrinsics.h" */

TARGET_HASWELL
namespace simdjson::haswell {

#ifndef _MSC_VER
// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
__attribute__((no_sanitize("undefined")))  // this is deliberate
#endif
really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  return (int)_tzcnt_u64(input_num);
#else
  ////////
  // You might expect the next line to be equivalent to 
  // return (int)_tzcnt_u64(input_num);
  // but the generated code differs and might be less efficient?
  ////////
  return __builtin_ctzll(input_num);
#endif// _MSC_VER
}

/* result might be undefined when input_num is zero */
really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return _blsr_u64(input_num);
}

/* result might be undefined when input_num is zero */
really_inline int leading_zeroes(uint64_t input_num) {
  return static_cast<int>(_lzcnt_u64(input_num));
}

really_inline int hamming(uint64_t input_num) {
#ifdef _MSC_VER
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num);// Visual Studio wants two underscores
#else
  return _popcnt64(input_num);
#endif
}

really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef _MSC_VER
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

#ifdef _MSC_VER
#pragma intrinsic(_umul128)
#endif
really_inline bool mul_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef _MSC_VER
  uint64_t high;
  *result = _umul128(value1, value2, &high);
  return high;
#else
  return __builtin_umulll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}
}// namespace simdjson::haswell
UNTARGET_REGION
#endif
#endif //  SIMDJSON_HASWELL_BITMANIPULATION_H
/* end file src/haswell/bitmanipulation.h */

TARGET_HASWELL
namespace simdjson::haswell {

using namespace simd;

really_inline void find_whitespace_and_operators(simd8x64<uint8_t> in, uint64_t &whitespace, uint64_t &op) {

  // These lookups rely on the fact that anything < 127 will match the lower 4 bits, which is why
  // we can't use the generic lookup_16.
  auto whitespace_table = simd8<uint8_t>::repeat_16(' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100);
  auto op_table = simd8<uint8_t>::repeat_16(',', '}', 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{');

  whitespace = in.map([&](simd8<uint8_t> _in) {
    return _in == simd8<uint8_t>(_mm256_shuffle_epi8(whitespace_table, _in));
  }).to_bitmask();

  op = in.map([&](simd8<uint8_t> _in) {
    // | 32 handles the fact that { } and [ ] are exactly 32 bytes apart
    return (_in | 32) == simd8<uint8_t>(_mm256_shuffle_epi8(op_table, _in-','));
  }).to_bitmask();
}

really_inline bool is_ascii(simd8x64<uint8_t> input) {
  simd8<uint8_t> bits = input.reduce([&](auto a,auto b) { return a|b; });
  return !bits.any_bits_set_anywhere(0b10000000u);
}

really_inline simd8<bool> must_be_continuation(simd8<uint8_t> prev1, simd8<uint8_t> prev2, simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0b11000000u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

/* begin file src/generic/utf8_lookup2_algorithm.h */
//
// Detect Unicode errors.
//
// UTF-8 is designed to allow multiple bytes and be compatible with ASCII. It's a fairly basic
// encoding that uses the first few bits on each byte to denote a "byte type", and all other bits
// are straight up concatenated into the final value. The first byte of a multibyte character is a
// "leading byte" and starts with N 1's, where N is the total number of bytes (110_____ = 2 byte
// lead). The remaining bytes of a multibyte character all start with 10. 1-byte characters just
// start with 0, because that's what ASCII looks like. Here's what each size 
//
// - ASCII (7 bits):              0_______
// - 2 byte character (11 bits):  110_____ 10______
// - 3 byte character (17 bits):  1110____ 10______ 10______
// - 4 byte character (23 bits):  11110___ 10______ 10______ 10______
// - 5+ byte character (illegal): 11111___ <illegal>
//
// There are 5 classes of error that can happen in Unicode:
//
// - TOO_SHORT: when you have a multibyte character with too few bytes (i.e. missing continuation).
//   We detect this by looking for new characters (lead bytes) inside the range of a multibyte
//   character.
//
//   e.g. 11000000 01100001 (2-byte character where second byte is ASCII)
//
// - TOO_LONG: when there are more bytes in your character than you need (i.e. extra continuation).
//   We detect this by requiring that the next byte after your multibyte character be a new
//   character--so a continuation after your character is wrong.
//
//   e.g. 11011111 10111111 10111111 (2-byte character followed by *another* continuation byte)
//
// - TOO_LARGE: Unicode only goes up to U+10FFFF. These characters are too large.
//
//   e.g. 11110111 10111111 10111111 10111111 (bigger than 10FFFF).
//
// - OVERLONG: multibyte characters with a bunch of leading zeroes, where you could have
//   used fewer bytes to make the same character. Like encoding an ASCII character in 4 bytes is
//   technically possible, but UTF-8 disallows it so that there is only one way to write an "a".
//
//   e.g. 11000001 10100001 (2-byte encoding of "a", which only requires 1 byte: 01100001)
//
// - SURROGATE: Unicode U+D800-U+DFFF is a *surrogate* character, reserved for use in UCS-2 and
//   WTF-8 encodings for characters with > 2 bytes. These are illegal in pure UTF-8.
//
//   e.g. 11101101 10100000 10000000 (U+D800)
//
// - INVALID_5_BYTE: 5-byte, 6-byte, 7-byte and 8-byte characters are unsupported; Unicode does not
//   support values with more than 23 bits (which a 4-byte character supports).
//
//   e.g. 11111000 10100000 10000000 10000000 10000000 (U+800000)
//   
// Legal utf-8 byte sequences per  http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94:
// 
//   Code Points        1st       2s       3s       4s
//  U+0000..U+007F     00..7F
//  U+0080..U+07FF     C2..DF   80..BF
//  U+0800..U+0FFF     E0       A0..BF   80..BF
//  U+1000..U+CFFF     E1..EC   80..BF   80..BF
//  U+D000..U+D7FF     ED       80..9F   80..BF
//  U+E000..U+FFFF     EE..EF   80..BF   80..BF
//  U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
//  U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
//  U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
//
using namespace simd;

namespace utf8_validation {

  //
  // Find special case UTF-8 errors where the character is technically readable (has the right length)
  // but the *value* is disallowed.
  //
  // This includes overlong encodings, surrogates and values too large for Unicode.
  //
  // It turns out the bad character ranges can all be detected by looking at the first 12 bits of the
  // UTF-8 encoded character (i.e. all of byte 1, and the high 4 bits of byte 2). This algorithm does a
  // 3 4-bit table lookups, identifying which errors that 4 bits could match, and then &'s them together.
  // If all 3 lookups detect the same error, it's an error.
  //
  really_inline simd8<uint8_t> check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1) {
    //
    // These are the errors we're going to match for bytes 1-2, by looking at the first three
    // nibbles of the character: <high bits of byte 1>> & <low bits of byte 1> & <high bits of byte 2>
    //
    static const int OVERLONG_2  = 0x01; // 1100000_ 10______ (technically we match 10______ but we could match ________, they both yield errors either way)
    static const int OVERLONG_3  = 0x02; // 11100000 100_____ ________
    static const int OVERLONG_4  = 0x04; // 11110000 1000____ ________ ________
    static const int SURROGATE   = 0x08; // 11101101 [101_]____
    static const int TOO_LARGE   = 0x10; // 11110100 (1001|101_)____
    static const int TOO_LARGE_2 = 0x20; // 1111(1___|011_|0101) 10______

    // After processing the rest of byte 1 (the low bits), we're still not done--we have to check
    // byte 2 to be sure which things are errors and which aren't.
    // Since high_bits is byte 5, byte 2 is high_bits.prev<3>
    static const int CARRY = OVERLONG_2 | TOO_LARGE_2;
    const simd8<uint8_t> byte_2_high = input.shr<4>().lookup_16<uint8_t>(
        // ASCII: ________ [0___]____
        CARRY, CARRY, CARRY, CARRY,
        // ASCII: ________ [0___]____
        CARRY, CARRY, CARRY, CARRY,
        // Continuations: ________ [10__]____
        CARRY | OVERLONG_3 | OVERLONG_4, // ________ [1000]____
        CARRY | OVERLONG_3 | TOO_LARGE,  // ________ [1001]____
        CARRY | TOO_LARGE  | SURROGATE,  // ________ [1010]____
        CARRY | TOO_LARGE  | SURROGATE,  // ________ [1011]____
        // Multibyte Leads: ________ [11__]____
        CARRY, CARRY, CARRY, CARRY
    );

    const simd8<uint8_t> byte_1_high = prev1.shr<4>().lookup_16<uint8_t>(
      // [0___]____ (ASCII)
      0, 0, 0, 0,                          
      0, 0, 0, 0,
      // [10__]____ (continuation)
      0, 0, 0, 0,
      // [11__]____ (2+-byte leads)
      OVERLONG_2, 0,                       // [110_]____ (2-byte lead)
      OVERLONG_3 | SURROGATE,              // [1110]____ (3-byte lead)
      OVERLONG_4 | TOO_LARGE | TOO_LARGE_2 // [1111]____ (4+-byte lead)
    );

    const simd8<uint8_t> byte_1_low = (prev1 & 0x0F).lookup_16<uint8_t>(
      // ____[00__] ________
      OVERLONG_2 | OVERLONG_3 | OVERLONG_4, // ____[0000] ________
      OVERLONG_2,                           // ____[0001] ________
      0, 0,
      // ____[01__] ________
      TOO_LARGE,                            // ____[0100] ________
      TOO_LARGE_2,
      TOO_LARGE_2,
      TOO_LARGE_2,
      // ____[10__] ________
      TOO_LARGE_2, TOO_LARGE_2, TOO_LARGE_2, TOO_LARGE_2,
      // ____[11__] ________
      TOO_LARGE_2,
      TOO_LARGE_2 | SURROGATE,                            // ____[1101] ________
      TOO_LARGE_2, TOO_LARGE_2
    );

    return byte_1_high & byte_1_low & byte_2_high;
  }

  //
  // Validate the length of multibyte characters (that each multibyte character has the right number
  // of continuation characters, and that all continuation characters are part of a multibyte
  // character).
  //
  // Algorithm
  // =========
  //
  // This algorithm compares *expected* continuation characters with *actual* continuation bytes,
  // and emits an error anytime there is a mismatch.
  //
  // For example, in the string "𝄞₿֏ab", which has a 4-, 3-, 2- and 1-byte
  // characters, the file will look like this:
  //
  // | Character             | 𝄞  |    |    |    | ₿  |    |    | ֏  |    | a  | b  |
  // |-----------------------|----|----|----|----|----|----|----|----|----|----|----|
  // | Character Length      |  4 |    |    |    |  3 |    |    |  2 |    |  1 |  1 |
  // | Byte                  | F0 | 9D | 84 | 9E | E2 | 82 | BF | D6 | 8F | 61 | 62 |
  // | is_second_byte        |    |  X |    |    |    |  X |    |    |  X |    |    |
  // | is_third_byte         |    |    |  X |    |    |    |  X |    |    |    |    |
  // | is_fourth_byte        |    |    |    |  X |    |    |    |    |    |    |    |
  // | expected_continuation |    |  X |  X |  X |    |  X |  X |    |  X |    |    |
  // | is_continuation       |    |  X |  X |  X |    |  X |  X |    |  X |    |    |
  //
  // The errors here are basically (Second Byte OR Third Byte OR Fourth Byte == Continuation):
  //
  // - **Extra Continuations:** Any continuation that is not a second, third or fourth byte is not
  //   part of a valid 2-, 3- or 4-byte character and is thus an error. It could be that it's just
  //   floating around extra outside of any character, or that there is an illegal 5-byte character,
  //   or maybe it's at the beginning of the file before any characters have started; but it's an
  //   error in all these cases.
  // - **Missing Continuations:** Any second, third or fourth byte that *isn't* a continuation is an error, because that means
  //   we started a new character before we were finished with the current one.
  //
  // Getting the Previous Bytes
  // --------------------------
  //
  // Because we want to know if a byte is the *second* (or third, or fourth) byte of a multibyte
  // character, we need to "shift the bytes" to find that out. This is what they mean:
  //
  // - `is_continuation`: if the current byte is a continuation.
  // - `is_second_byte`: if 1 byte back is the start of a 2-, 3- or 4-byte character.
  // - `is_third_byte`: if 2 bytes back is the start of a 3- or 4-byte character.
  // - `is_fourth_byte`: if 3 bytes back is the start of a 4-byte character.
  //
  // We use shuffles to go n bytes back, selecting part of the current `input` and part of the
  // `prev_input` (search for `.prev<1>`, `.prev<2>`, etc.). These are passed in by the caller
  // function, because the 1-byte-back data is used by other checks as well.
  //
  // Getting the Continuation Mask
  // -----------------------------
  //
  // Once we have the right bytes, we have to get the masks. To do this, we treat UTF-8 bytes as
  // numbers, using signed `<` and `>` operations to check if they are continuations or leads.
  // In fact, we treat the numbers as *signed*, partly because it helps us, and partly because
  // Intel's SIMD presently only offers signed `<` and `>` operations (not unsigned ones).
  //
  // In UTF-8, bytes that start with the bits 110, 1110 and 11110 are 2-, 3- and 4-byte "leads,"
  // respectively, meaning they expect to have 1, 2 and 3 "continuation bytes" after them.
  // Continuation bytes start with 10, and ASCII (1-byte characters) starts with 0.
  //
  // When treated as signed numbers, they look like this:
  //
  // | Type         | High Bits  | Binary Range | Signed |
  // |--------------|------------|--------------|--------|
  // | ASCII        | `0`        | `01111111`   |   127  |
  // |              |            | `00000000`   |     0  |
  // | 4+-Byte Lead | `1111`     | `11111111`   |    -1  |
  // |              |            | `11110000    |   -16  |
  // | 3-Byte Lead  | `1110`     | `11101111`   |   -17  |
  // |              |            | `11100000    |   -32  |
  // | 2-Byte Lead  | `110`      | `11011111`   |   -33  |
  // |              |            | `11000000    |   -64  |
  // | Continuation | `10`       | `10111111`   |   -65  |
  // |              |            | `10000000    |  -128  |
  //
  // This makes it pretty easy to get the continuation mask! It's just a single comparison:
  //
  // ```
  // is_continuation = input < -64`
  // ```
  //
  // We can do something similar for the others, but it takes two comparisons instead of one: "is
  // the start of a 4-byte character" is `< -32` and `> -65`, for example. And 2+ bytes is `< 0` and
  // `> -64`. Surely we can do better, they're right next to each other!
  //
  // Getting the is_xxx Masks: Shifting the Range
  // --------------------------------------------
  //
  // Notice *why* continuations were a single comparison. The actual *range* would require two
  // comparisons--`< -64` and `> -129`--but all characters are always greater than -128, so we get
  // that for free. In fact, if we had *unsigned* comparisons, 2+, 3+ and 4+ comparisons would be
  // just as easy: 4+ would be `> 239`, 3+ would be `> 223`, and 2+ would be `> 191`.
  //
  // Instead, we add 128 to each byte, shifting the range up to make comparison easy. This wraps
  // ASCII down into the negative, and puts 4+-Byte Lead at the top:
  //
  // | Type                 | High Bits  | Binary Range | Signed |
  // |----------------------|------------|--------------|-------|
  // | 4+-Byte Lead (+ 127) | `0111`     | `01111111`   |   127 |
  // |                      |            | `01110000    |   112 |
  // |----------------------|------------|--------------|-------|
  // | 3-Byte Lead (+ 127)  | `0110`     | `01101111`   |   111 |
  // |                      |            | `01100000    |    96 |
  // |----------------------|------------|--------------|-------|
  // | 2-Byte Lead (+ 127)  | `010`      | `01011111`   |    95 |
  // |                      |            | `01000000    |    64 |
  // |----------------------|------------|--------------|-------|
  // | Continuation (+ 127) | `00`       | `00111111`   |    63 |
  // |                      |            | `00000000    |     0 |
  // |----------------------|------------|--------------|-------|
  // | ASCII (+ 127)        | `1`        | `11111111`   |    -1 |
  // |                      |            | `10000000`   |  -128 |
  // |----------------------|------------|--------------|-------|
  // 
  // *Now* we can use signed `>` on all of them:
  //
  // ```
  // prev1 = input.prev<1>
  // prev2 = input.prev<2>
  // prev3 = input.prev<3>
  // prev1_flipped = input.prev<1>(prev_input) ^ 0x80; // Same as `+ 128`
  // prev2_flipped = input.prev<2>(prev_input) ^ 0x80; // Same as `+ 128`
  // prev3_flipped = input.prev<3>(prev_input) ^ 0x80; // Same as `+ 128`
  // is_second_byte = prev1_flipped > 63;  // 2+-byte lead
  // is_third_byte  = prev2_flipped > 95;  // 3+-byte lead
  // is_fourth_byte = prev3_flipped > 111; // 4+-byte lead
  // ```
  //
  // NOTE: we use `^ 0x80` instead of `+ 128` in the code, which accomplishes the same thing, and even takes the same number
  // of cycles as `+`, but on many Intel architectures can be parallelized better (you can do 3
  // `^`'s at a time on Haswell, but only 2 `+`'s).
  //
  // That doesn't look like it saved us any instructions, did it? Well, because we're adding the
  // same number to all of them, we can save one of those `+ 128` operations by assembling
  // `prev2_flipped` out of prev 1 and prev 3 instead of assembling it from input and adding 128
  // to it. One more instruction saved!
  //
  // ```
  // prev1 = input.prev<1>
  // prev3 = input.prev<3>
  // prev1_flipped = prev1 ^ 0x80; // Same as `+ 128`
  // prev3_flipped = prev3 ^ 0x80; // Same as `+ 128`
  // prev2_flipped = prev1_flipped.concat<2>(prev3_flipped): // <shuffle: take the first 2 bytes from prev1 and the rest from prev3  
  // ```
  //
  // ### Bringing It All Together: Detecting the Errors
  //
  // At this point, we have `is_continuation`, `is_first_byte`, `is_second_byte` and `is_third_byte`.
  // All we have left to do is check if they match!
  //
  // ```
  // return (is_second_byte | is_third_byte | is_fourth_byte) ^ is_continuation;
  // ```
  //
  // But wait--there's more. The above statement is only 3 operations, but they *cannot be done in
  // parallel*. You have to do 2 `|`'s and then 1 `&`. Haswell, at least, has 3 ports that can do
  // bitwise operations, and we're only using 1!
  //
  // Epilogue: Addition For Booleans
  // -------------------------------
  //
  // There is one big case the above code doesn't explicitly talk about--what if is_second_byte
  // and is_third_byte are BOTH true? That means there is a 3-byte and 2-byte character right next
  // to each other (or any combination), and the continuation could be part of either of them!
  // Our algorithm using `&` and `|` won't detect that the continuation byte is problematic.
  //
  // Never fear, though. If that situation occurs, we'll already have detected that the second
  // leading byte was an error, because it was supposed to be a part of the preceding multibyte
  // character, but it *wasn't a continuation*.
  //
  // We could stop here, but it turns out that we can fix it using `+` and `-` instead of `|` and
  // `&`, which is both interesting and possibly useful (even though we're not using it here). It
  // exploits the fact that in SIMD, a *true* value is -1, and a *false* value is 0. So those
  // comparisons were giving us numbers!
  //
  // Given that, if you do `is_second_byte + is_third_byte + is_fourth_byte`, under normal
  // circumstances you will either get 0 (0 + 0 + 0) or -1 (-1 + 0 + 0, etc.). Thus,
  // `(is_second_byte + is_third_byte + is_fourth_byte) - is_continuation` will yield 0 only if
  // *both* or *neither* are 0 (0-0 or -1 - -1). You'll get 1 or -1 if they are different. Because
  // *any* nonzero value is treated as an error (not just -1), we're just fine here :)
  //
  // Further, if *more than one* multibyte character overlaps,
  // `is_second_byte + is_third_byte + is_fourth_byte` will be -2 or -3! Subtracting `is_continuation`
  // from *that* is guaranteed to give you a nonzero value (-1, -2 or -3). So it'll always be
  // considered an error.
  //
  // One reason you might want to do this is parallelism. ^ and | are not associative, so
  // (A | B | C) ^ D will always be three operations in a row: either you do A | B -> | C -> ^ D, or
  // you do B | C -> | A -> ^ D. But addition and subtraction *are* associative: (A + B + C) - D can
  // be written as `(A + B) + (C - D)`. This means you can do A + B and C - D at the same time, and
  // then adds the result together. Same number of operations, but if the processor can run
  // independent things in parallel (which most can), it runs faster.
  //
  // This doesn't help us on Intel, but might help us elsewhere: on Haswell, at least, | and ^ have
  // a super nice advantage in that more of them can be run at the same time (they can run on 3
  // ports, while + and - can run on 2)! This means that we can do A | B while we're still doing C,
  // saving us the cycle we would have earned by using +. Even more, using an instruction with a
  // wider array of ports can help *other* code run ahead, too, since these instructions can "get
  // out of the way," running on a port other instructions can't.
  // 
  // Epilogue II: One More Trick
  // ---------------------------
  //
  // There's one more relevant trick up our sleeve, it turns out: it turns out on Intel we can "pay
  // for" the (prev<1> + 128) instruction, because it can be used to save an instruction in
  // check_special_cases()--but we'll talk about that there :)
  //
  really_inline simd8<uint8_t> check_multibyte_lengths(simd8<uint8_t> input, simd8<uint8_t> prev_input, simd8<uint8_t> prev1) {
    simd8<uint8_t> prev2 = input.prev<2>(prev_input);
    simd8<uint8_t> prev3 = input.prev<3>(prev_input);

    // Cont is 10000000-101111111 (-65...-128)
    simd8<bool> is_continuation = simd8<int8_t>(input) < int8_t(-64);
    // must_be_continuation is architecture-specific because Intel doesn't have unsigned comparisons
    return simd8<uint8_t>(must_be_continuation(prev1, prev2, prev3) ^ is_continuation);
  }

  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  really_inline simd8<uint8_t> is_incomplete(simd8<uint8_t> input) {
    // If the previous input's last 3 bytes match this, they're too short (they ended at EOF):
    // ... 1111____ 111_____ 11______
    static const uint8_t max_array[32] = {
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 0b11110000u-1, 0b11100000u-1, 0b11000000u-1
    };
    const simd8<uint8_t> max_value(&max_array[sizeof(max_array)-sizeof(simd8<uint8_t>)]);
    return input.gt_bits(max_value);
  }

  struct utf8_checker {
    // If this is nonzero, there has been a UTF-8 error.
    simd8<uint8_t> error;
    // The last input we received
    simd8<uint8_t> prev_input_block;
    // Whether the last input we received was incomplete (used for ASCII fast path)
    simd8<uint8_t> prev_incomplete;

    //
    // Check whether the current bytes are valid UTF-8.
    //
    really_inline void check_utf8_bytes(const simd8<uint8_t> input, const simd8<uint8_t> prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      simd8<uint8_t> prev1 = input.prev<1>(prev_input);
      this->error |= check_special_cases(input, prev1);
      this->error |= check_multibyte_lengths(input, prev_input, prev1);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short.
    really_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error |= this->prev_incomplete;
    }

    really_inline void check_next_input(simd8x64<uint8_t> input) {
      if (likely(is_ascii(input))) {
        // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
        // possibly finish them.
        this->error |= this->prev_incomplete;
      } else {
        this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
        for (int i=1; i<simd8x64<uint8_t>::NUM_CHUNKS; i++) {
          this->check_utf8_bytes(input.chunks[i], input.chunks[i-1]);
        }
        this->prev_incomplete = is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1]);
        this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1];
      }
    }

    really_inline ErrorValues errors() {
      return this->error.any_bits_set_anywhere() ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
    }

  }; // struct utf8_checker
}

using utf8_validation::utf8_checker;
/* end file src/generic/utf8_lookup2_algorithm.h */
/* begin file src/generic/stage1_find_marks.h */
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

namespace stage1 {

class bit_indexer {
public:
  uint32_t *tail;

  bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // base_ptr[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so base_ptr
  // needs to be large enough to handle this
  really_inline void write_indexes(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
        return;
    uint32_t cnt = hamming(bits);

    // Do the first 8 all together
    for (int i=0; i<8; i++) {
      this->tail[i] = idx + trailing_zeroes(bits);
      bits = clear_lowest_bit(bits);
    }

    // Do the next 8 all together (we hope in most cases it won't happen at all
    // and the branch is easily predicted).
    if (unlikely(cnt > 8)) {
      for (int i=8; i<16; i++) {
        this->tail[i] = idx + trailing_zeroes(bits);
        bits = clear_lowest_bit(bits);
      }

      // Most files don't have 16+ structurals per block, so we take several basically guaranteed
      // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
      // or the start of a value ("abc" true 123) every four characters.
      if (unlikely(cnt > 16)) {
        uint32_t i = 16;
        do {
          this->tail[i] = idx + trailing_zeroes(bits);
          bits = clear_lowest_bit(bits);
          i++;
        } while (i < cnt);
      }
    }

    this->tail += cnt;
  }
};

class json_structural_scanner {
public:
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
  uint64_t prev_structurals = 0;
  // Errors with unescaped characters in strings (ASCII codepoints < 0x20)
  uint64_t unescaped_chars_error = 0;
  bit_indexer structural_indexes;

  json_structural_scanner(uint32_t *_structural_indexes) : structural_indexes{_structural_indexes} {}

  //
  // Finish the scan and return any errors.
  //
  // This may detect errors as well, such as unclosed string and certain UTF-8 errors.
  // if streaming is set to true, an unclosed string is allowed.
  //
  really_inline ErrorValues detect_errors_on_eof(bool streaming = false);

  //
  // Return a mask of all string characters plus end quotes.
  //
  // prev_escaped is overflow saying whether the next character is escaped.
  // prev_in_string is overflow saying whether we're still in a string.
  //
  // Backslash sequences outside of quotes will be detected in stage 2.
  //
  really_inline uint64_t find_strings(const simd::simd8x64<uint8_t> in);

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
  really_inline uint64_t find_potential_structurals(const simd::simd8x64<uint8_t> in);

  //
  // Find the important bits of JSON in a STEP_SIZE-byte chunk, and add them to structural_indexes.
  //
  template<size_t STEP_SIZE>
  really_inline void scan_step(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker);

  //
  // Parse the entire input in STEP_SIZE-byte chunks.
  //
  template<size_t STEP_SIZE>
  really_inline void scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker);
};

// Routines to print masks and text for debugging bitmask operations
UNUSED static char * format_input_text(const simd8x64<uint8_t> in) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  in.store((uint8_t*)buf);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    if (buf[i] < ' ') { buf[i] = '_'; }
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

UNUSED static char * format_mask(uint64_t mask) {
  static char *buf = (char*)malloc(64 + 1);
  for (size_t i=0; i<64; i++) {
    buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
  }
  buf[64] = '\0';
  return buf;
}

//
// Finds escaped characters (characters following \).
//
// Handles runs of backslashes like \\\" and \\\\" correctly (yielding 0101 and 01010, respectively).
//
// Does this by:
// - Shift the escape mask to get potentially escaped characters (characters after backslashes).
// - Mask escaped sequences that start on *even* bits with 1010101010 (odd bits are escaped, even bits are not)
// - Mask escaped sequences that start on *odd* bits with 0101010101 (even bits are escaped, odd bits are not)
//
// To distinguish between escaped sequences starting on even/odd bits, it finds the start of all
// escape sequences, filters out the ones that start on even bits, and adds that to the mask of
// escape sequences. This causes the addition to clear out the sequences starting on odd bits (since
// the start bit causes a carry), and leaves even-bit sequences alone.
//
// Example:
//
// text           |  \\\ | \\\"\\\" \\\" \\"\\" |
// escape         |  xxx |  xx xxx  xxx  xx xx  | Removed overflow backslash; will | it into follows_escape
// odd_starts     |  x   |  x       x       x   | escape & ~even_bits & ~follows_escape
// even_seq       |     c|    cxxx     c xx   c | c = carry bit -- will be masked out later
// invert_mask    |      |     cxxx     c xx   c| even_seq << 1
// follows_escape |   xx | x xx xxx  xxx  xx xx | Includes overflow bit
// escaped        |   x  | x x  x x  x x  x  x  |
// desired        |   x  | x x  x x  x x  x  x  |
// text           |  \\\ | \\\"\\\" \\\" \\"\\" |
//
really_inline uint64_t find_escaped(uint64_t escape, uint64_t &escaped_overflow) {
  // If there was overflow, pretend the first character isn't a backslash
  escape &= ~escaped_overflow;
  uint64_t follows_escape = escape << 1 | escaped_overflow;

  // Get sequences starting on even bits by clearing out the odd series using +
  const uint64_t even_bits = 0x5555555555555555ULL;
  uint64_t odd_sequence_starts = escape & ~even_bits & ~follows_escape;
  uint64_t sequences_starting_on_even_bits;
  escaped_overflow = add_overflow(odd_sequence_starts, escape, &sequences_starting_on_even_bits);
  uint64_t invert_mask = sequences_starting_on_even_bits << 1; // The mask we want to return is the *escaped* bits, not escapes.

  // Mask every other backslashed character as an escaped character
  // Flip the mask for sequences that start on even bits, to correct them
  return (even_bits ^ invert_mask) & follows_escape;
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
really_inline uint64_t follows(const uint64_t match, const uint64_t filler, uint64_t &overflow) {
  uint64_t follows_match = follows(match, overflow);
  uint64_t result;
  overflow |= add_overflow(follows_match, filler, &result);
  return result;
}

really_inline ErrorValues json_structural_scanner::detect_errors_on_eof(bool streaming) {
  if ((prev_in_string) and (not streaming)) {
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
really_inline uint64_t json_structural_scanner::find_strings(const simd::simd8x64<uint8_t> in) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = find_escaped(backslash, prev_escaped);
  const uint64_t quote = in.eq('"') & ~escaped;
  // prefix_xor flips on bits inside the string (and flips off the end quote).
  const uint64_t in_string = prefix_xor(quote) ^ prev_in_string;
  /* right shift of a signed value expected to be well-defined and standard
  * compliant as of C++20,
  * John Regher from Utah U. says this is fine code */
  prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
  // Use ^ to turn the beginning quote off, and the end quote on.
  return in_string ^ quote;
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
really_inline uint64_t json_structural_scanner::find_potential_structurals(const simd::simd8x64<uint8_t> in) {
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

//
// Find the important bits of JSON in a 128-byte chunk, and add them to structural_indexes.
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
template<>
really_inline void json_structural_scanner::scan_step<128>(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker) {
  //
  // Load up all 128 bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);
  simd::simd8x64<uint8_t> in_2(buf+64);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = this->find_strings(in_1);
  uint64_t structurals_1 = this->find_potential_structurals(in_1);
  uint64_t string_2 = this->find_strings(in_2);
  uint64_t structurals_2 = this->find_potential_structurals(in_2);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_1 & ~string_1;
  this->unescaped_chars_error |= unescaped_1 & string_1;

  uint64_t unescaped_2 = in_2.lteq(0x1F);
  utf8_checker.check_next_input(in_2);
  this->structural_indexes.write_indexes(idx, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_2 & ~string_2;
  this->unescaped_chars_error |= unescaped_2 & string_2;
}

//
// Find the important bits of JSON in a 64-byte chunk, and add them to structural_indexes.
//
template<>
really_inline void json_structural_scanner::scan_step<64>(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker) {
  //
  // Load up bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = this->find_strings(in_1);
  uint64_t structurals_1 = this->find_potential_structurals(in_1);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_1 & ~string_1;
  this->unescaped_chars_error |= unescaped_1 & string_1;
}

template<size_t STEP_SIZE>
really_inline void json_structural_scanner::scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker) {
  size_t lenminusstep = len < STEP_SIZE ? 0 : len - STEP_SIZE;
  size_t idx = 0;

  for (; idx < lenminusstep; idx += STEP_SIZE) {
    this->scan_step<STEP_SIZE>(&buf[idx], idx, utf8_checker);
  }

  /* If we have a final chunk of less than STEP_SIZE bytes, pad it to STEP_SIZE with
  * spaces  before processing it (otherwise, we risk invalidating the UTF-8
  * checks). */
  if (likely(idx < len)) {
    uint8_t tmp_buf[STEP_SIZE];
    memset(tmp_buf, 0x20, STEP_SIZE);
    memcpy(tmp_buf, buf + idx, len - idx);
    this->scan_step<STEP_SIZE>(&tmp_buf[0], idx, utf8_checker);
    idx += STEP_SIZE;
  }

  /* finally, flatten out the remaining structurals from the last iteration */
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals);
}

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
template<size_t STEP_SIZE>
int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  if (unlikely(len > pj.byte_capacity)) {
    return simdjson::CAPACITY;
  }
  utf8_checker utf8_checker{};
  json_structural_scanner scanner{pj.structural_indexes.get()};
  scanner.scan<STEP_SIZE>(buf, len, utf8_checker);
  // we might tolerate an unclosed string if streaming is true
  simdjson::ErrorValues error = scanner.detect_errors_on_eof(streaming);
  if (unlikely(error != simdjson::SUCCESS)) {
    return error;
  }
  pj.n_structural_indexes = scanner.structural_indexes.tail - pj.structural_indexes.get();
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
  return utf8_checker.errors();
}

} // namespace stage1
/* end file src/generic/stage1_find_marks.h */

} // namespace haswell
UNTARGET_REGION

TARGET_HASWELL
namespace simdjson {

template <>
int find_structural_bits<Architecture::HASWELL>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  return haswell::stage1::find_structural_bits<128>(buf, len, pj, streaming);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_HASWELL_STAGE1_FIND_MARKS_H
/* end file src/generic/stage1_find_marks.h */
/* begin file src/westmere/stage1_find_marks.h */
#ifndef SIMDJSON_WESTMERE_STAGE1_FIND_MARKS_H
#define SIMDJSON_WESTMERE_STAGE1_FIND_MARKS_H


#ifdef IS_X86_64

/* begin file src/westmere/bitmask.h */
#ifndef SIMDJSON_WESTMERE_BITMASK_H
#define SIMDJSON_WESTMERE_BITMASK_H


#ifdef IS_X86_64

/* begin file src/westmere/intrinsics.h */
#ifndef SIMDJSON_WESTMERE_INTRINSICS_H
#define SIMDJSON_WESTMERE_INTRINSICS_H

#ifdef IS_X86_64
#ifdef _MSC_VER
#include <intrin.h> // visual studio
#else
#include <x86intrin.h> // elsewhere
#endif //  _MSC_VER
#endif //  IS_X86_64
#endif //  SIMDJSON_WESTMERE_INTRINSICS_H
/* end file src/westmere/intrinsics.h */

TARGET_WESTMERE
namespace simdjson::westmere {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
really_inline uint64_t prefix_xor(const uint64_t bitmask) {
  // There should be no such thing with a processing supporting avx2
  // but not clmul.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return _mm_cvtsi128_si64(result);
}

} // namespace simdjson::westmere
UNTARGET_REGION

#endif // IS_X86_64
#endif
/* end file src/westmere/intrinsics.h */
/* begin file src/westmere/simd.h */
#ifndef SIMDJSON_WESTMERE_SIMD_H
#define SIMDJSON_WESTMERE_SIMD_H


#ifdef IS_X86_64

/* westmere/intrinsics.h already included: #include "westmere/intrinsics.h" */

TARGET_WESTMERE
namespace simdjson::westmere::simd {

  template<typename Child>
  struct base {
    __m128i value;

    // Zero constructor
    really_inline base() : value{__m128i()} {}

    // Conversion from SIMD register
    really_inline base(const __m128i _value) : value(_value) {}

    // Conversion to SIMD register
    really_inline operator const __m128i&() const { return this->value; }
    really_inline operator __m128i&() { return this->value; }

    // Bit operations
    really_inline Child operator|(const Child other) const { return _mm_or_si128(*this, other); }
    really_inline Child operator&(const Child other) const { return _mm_and_si128(*this, other); }
    really_inline Child operator^(const Child other) const { return _mm_xor_si128(*this, other); }
    really_inline Child bit_andnot(const Child other) const { return _mm_andnot_si128(other, *this); }
    really_inline Child operator~() const { return *this ^ 0xFFu; }
    really_inline Child& operator|=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast | other; return *this_cast; }
    really_inline Child& operator&=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast & other; return *this_cast; }
    really_inline Child& operator^=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    really_inline base8() : base<simd8<T>>() {}
    really_inline base8(const __m128i _value) : base<simd8<T>>(_value) {}

    really_inline Mask operator==(const simd8<T> other) const { return _mm_cmpeq_epi8(*this, other); }

    static const int SIZE = sizeof(base<simd8<T>>::value);

    template<int N=1>
    really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return _mm_alignr_epi8(*this, prev_chunk, 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static really_inline simd8<bool> splat(bool _value) { return _mm_set1_epi8(-(!!_value)); }

    really_inline simd8<bool>() : base8() {}
    really_inline simd8<bool>(const __m128i _value) : base8<bool>(_value) {}
    // Splat constructor
    really_inline simd8<bool>(bool _value) : base8<bool>(splat(_value)) {}

    really_inline int to_bitmask() const { return _mm_movemask_epi8(*this); }
    really_inline bool any() const { return !_mm_testz_si128(*this, *this); }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static really_inline simd8<T> splat(T _value) { return _mm_set1_epi8(_value); }
    static really_inline simd8<T> zero() { return _mm_setzero_si128(); }
    static really_inline simd8<T> load(const T values[16]) {
      return _mm_loadu_si128(reinterpret_cast<const __m128i *>(values));
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static really_inline simd8<T> repeat_16(
      T v0,  T v1,  T v2,  T v3,  T v4,  T v5,  T v6,  T v7,
      T v8,  T v9,  T v10, T v11, T v12, T v13, T v14, T v15
    ) {
      return simd8<T>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    really_inline base8_numeric() : base8<T>() {}
    really_inline base8_numeric(const __m128i _value) : base8<T>(_value) {}

    // Store to array
    really_inline void store(T dst[16]) const { return _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), *this); }

    // Addition/subtraction are the same for signed and unsigned
    really_inline simd8<T> operator+(const simd8<T> other) const { return _mm_add_epi8(*this, other); }
    really_inline simd8<T> operator-(const simd8<T> other) const { return _mm_sub_epi8(*this, other); }
    really_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *(simd8<T>*)this; }
    really_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *(simd8<T>*)this; }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return _mm_shuffle_epi8(lookup_table, *this);
    }
    template<typename L>
    really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> : base8_numeric<int8_t> {
    really_inline simd8() : base8_numeric<int8_t>() {}
    really_inline simd8(const __m128i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const int8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(_mm_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Order-sensitive comparisons
    really_inline simd8<int8_t> max(const simd8<int8_t> other) const { return _mm_max_epi8(*this, other); }
    really_inline simd8<int8_t> min(const simd8<int8_t> other) const { return _mm_min_epi8(*this, other); }
    really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return _mm_cmpgt_epi8(*this, other); }
    really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return _mm_cmpgt_epi8(other, *this); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    really_inline simd8() : base8_numeric<uint8_t>() {}
    really_inline simd8(const __m128i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const uint8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(_mm_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Saturated math
    really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return _mm_adds_epu8(*this, other); }
    really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return _mm_subs_epu8(*this, other); }

    // Order-specific operations
    really_inline simd8<uint8_t> max(const simd8<uint8_t> other) const { return _mm_max_epu8(*this, other); }
    really_inline simd8<uint8_t> min(const simd8<uint8_t> other) const { return _mm_min_epu8(*this, other); }
    // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
    really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return this->saturating_sub(other); }
    // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
    really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return other.saturating_sub(*this); }
    really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return other.max(*this) == other; }
    really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return other.min(*this) == other; }
    really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }
    really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }

    // Bit-specific operations
    really_inline simd8<bool> bits_not_set() const { return *this == uint8_t(0); }
    really_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const { return (*this & bits).bits_not_set(); }
    really_inline simd8<bool> any_bits_set() const { return ~this->bits_not_set(); }
    really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return ~this->bits_not_set(bits); }
    really_inline bool bits_not_set_anywhere() const { return _mm_testz_si128(*this, *this); }
    really_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const { return _mm_testz_si128(*this, bits); }
    really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    really_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(_mm_srli_epi16(*this, N)) & uint8_t(0xFFu >> N); }
    template<int N>
    really_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(_mm_slli_epi16(*this, N)) & uint8_t(0xFFu << N); }
    // Get one of the bits and make a bitmask out of it.
    // e.g. value.get_bit<7>() gets the high bit
    template<int N>
    really_inline int get_bit() const { return _mm_movemask_epi8(_mm_slli_epi16(*this, 7-N)); }
  };

  template<typename T>
  struct simd8x64 {
    static const int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    const simd8<T> chunks[NUM_CHUNKS];

    really_inline simd8x64() : chunks{simd8<T>(), simd8<T>(), simd8<T>(), simd8<T>()} {}
    really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2, const simd8<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+16), simd8<T>::load(ptr+32), simd8<T>::load(ptr+48)} {}

    really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store(ptr+sizeof(simd8<T>)*3);
    }

    template <typename F>
    static really_inline void each_index(F const& each) {
      each(0);
      each(1);
      each(2);
      each(3);
    }

    template <typename F>
    really_inline void each(F const& each_chunk) const
    {
      each_chunk(this->chunks[0]);
      each_chunk(this->chunks[1]);
      each_chunk(this->chunks[2]);
      each_chunk(this->chunks[3]);
    }

    template <typename F, typename R=bool>
    really_inline simd8x64<R> map(F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0]),
        map_chunk(this->chunks[1]),
        map_chunk(this->chunks[2]),
        map_chunk(this->chunks[3])
      );
    }

    template <typename F, typename R=bool>
    really_inline simd8x64<R> map(const simd8x64<uint8_t> b, F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0], b.chunks[0]),
        map_chunk(this->chunks[1], b.chunks[1]),
        map_chunk(this->chunks[2], b.chunks[2]),
        map_chunk(this->chunks[3], b.chunks[3])
      );
    }

    template <typename F>
    really_inline simd8<T> reduce(F const& reduce_pair) const {
      return reduce_pair(
        reduce_pair(this->chunks[0], this->chunks[1]),
        reduce_pair(this->chunks[2], this->chunks[3])
      );
    }

    really_inline uint64_t to_bitmask() const {
      uint64_t r0 = static_cast<uint32_t>(this->chunks[0].to_bitmask());
      uint64_t r1 =                       this->chunks[1].to_bitmask();
      uint64_t r2 =                       this->chunks[2].to_bitmask();
      uint64_t r3 =                       this->chunks[3].to_bitmask();
      return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
    }

    really_inline simd8x64<T> bit_or(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a | mask; } );
    }

    really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a == mask; } ).to_bitmask();
    }

    really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a <= mask; } ).to_bitmask();
    }

  }; // struct simd8x64<T>

} // namespace simdjson::westmere::simd
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_WESTMERE_SIMD_INPUT_H
/* end file src/westmere/simd.h */
/* begin file src/westmere/bitmanipulation.h */
#ifndef SIMDJSON_WESTMERE_BITMANIPULATION_H
#define SIMDJSON_WESTMERE_BITMANIPULATION_H

#ifdef IS_X86_64
/* westmere/intrinsics.h already included: #include "westmere/intrinsics.h" */

TARGET_WESTMERE
namespace simdjson::westmere {

#ifndef _MSC_VER
// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
__attribute__((no_sanitize("undefined")))  // this is deliberate
#endif
/* result might be undefined when input_num is zero */
really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  unsigned long ret;
  // Search the mask data from least significant bit (LSB) 
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else
  return __builtin_ctzll(input_num);
#endif// _MSC_VER
}

/* result might be undefined when input_num is zero */
really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
}

/* result might be undefined when input_num is zero */
really_inline int leading_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB) 
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif// _MSC_VER
}

really_inline int hamming(uint64_t input_num) {
#ifdef _MSC_VER
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num);// Visual Studio wants two underscores
#else
  return _popcnt64(input_num);
#endif
}

really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef _MSC_VER
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

#ifdef _MSC_VER
#pragma intrinsic(_umul128)
#endif
really_inline bool mul_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef _MSC_VER
  uint64_t high;
  *result = _umul128(value1, value2, &high);
  return high;
#else
  return __builtin_umulll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

}// namespace simdjson::westmere
UNTARGET_REGION

#endif
#endif //  SIMDJSON_WESTMERE_BITMANIPULATION_H
/* end file src/westmere/bitmanipulation.h */

TARGET_WESTMERE
namespace simdjson::westmere {

using namespace simd;

really_inline void find_whitespace_and_operators(
  const simd8x64<uint8_t> in,
  uint64_t &whitespace, uint64_t &op) {

  // These lookups rely on the fact that anything < 127 will match the lower 4 bits, which is why
  // we can't use the generic lookup_16.
  auto whitespace_table = simd8<uint8_t>::repeat_16(' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100);
  auto op_table = simd8<uint8_t>::repeat_16(',', '}', 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{');

  whitespace = in.map([&](simd8<uint8_t> _in) {
    return _in == simd8<uint8_t>(_mm_shuffle_epi8(whitespace_table, _in));
  }).to_bitmask();

  op = in.map([&](simd8<uint8_t> _in) {
    // | 32 handles the fact that { } and [ ] are exactly 32 bytes apart
    return (_in | 32) == simd8<uint8_t>(_mm_shuffle_epi8(op_table, _in-','));
  }).to_bitmask();
}

really_inline bool is_ascii(simd8x64<uint8_t> input) {
  simd8<uint8_t> bits = input.reduce([&](auto a,auto b) { return a|b; });
  return !bits.any_bits_set_anywhere(0b10000000u);
}

really_inline simd8<bool> must_be_continuation(simd8<uint8_t> prev1, simd8<uint8_t> prev2, simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0b11000000u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

/* begin file src/generic/utf8_lookup2_algorithm.h */
//
// Detect Unicode errors.
//
// UTF-8 is designed to allow multiple bytes and be compatible with ASCII. It's a fairly basic
// encoding that uses the first few bits on each byte to denote a "byte type", and all other bits
// are straight up concatenated into the final value. The first byte of a multibyte character is a
// "leading byte" and starts with N 1's, where N is the total number of bytes (110_____ = 2 byte
// lead). The remaining bytes of a multibyte character all start with 10. 1-byte characters just
// start with 0, because that's what ASCII looks like. Here's what each size 
//
// - ASCII (7 bits):              0_______
// - 2 byte character (11 bits):  110_____ 10______
// - 3 byte character (17 bits):  1110____ 10______ 10______
// - 4 byte character (23 bits):  11110___ 10______ 10______ 10______
// - 5+ byte character (illegal): 11111___ <illegal>
//
// There are 5 classes of error that can happen in Unicode:
//
// - TOO_SHORT: when you have a multibyte character with too few bytes (i.e. missing continuation).
//   We detect this by looking for new characters (lead bytes) inside the range of a multibyte
//   character.
//
//   e.g. 11000000 01100001 (2-byte character where second byte is ASCII)
//
// - TOO_LONG: when there are more bytes in your character than you need (i.e. extra continuation).
//   We detect this by requiring that the next byte after your multibyte character be a new
//   character--so a continuation after your character is wrong.
//
//   e.g. 11011111 10111111 10111111 (2-byte character followed by *another* continuation byte)
//
// - TOO_LARGE: Unicode only goes up to U+10FFFF. These characters are too large.
//
//   e.g. 11110111 10111111 10111111 10111111 (bigger than 10FFFF).
//
// - OVERLONG: multibyte characters with a bunch of leading zeroes, where you could have
//   used fewer bytes to make the same character. Like encoding an ASCII character in 4 bytes is
//   technically possible, but UTF-8 disallows it so that there is only one way to write an "a".
//
//   e.g. 11000001 10100001 (2-byte encoding of "a", which only requires 1 byte: 01100001)
//
// - SURROGATE: Unicode U+D800-U+DFFF is a *surrogate* character, reserved for use in UCS-2 and
//   WTF-8 encodings for characters with > 2 bytes. These are illegal in pure UTF-8.
//
//   e.g. 11101101 10100000 10000000 (U+D800)
//
// - INVALID_5_BYTE: 5-byte, 6-byte, 7-byte and 8-byte characters are unsupported; Unicode does not
//   support values with more than 23 bits (which a 4-byte character supports).
//
//   e.g. 11111000 10100000 10000000 10000000 10000000 (U+800000)
//   
// Legal utf-8 byte sequences per  http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94:
// 
//   Code Points        1st       2s       3s       4s
//  U+0000..U+007F     00..7F
//  U+0080..U+07FF     C2..DF   80..BF
//  U+0800..U+0FFF     E0       A0..BF   80..BF
//  U+1000..U+CFFF     E1..EC   80..BF   80..BF
//  U+D000..U+D7FF     ED       80..9F   80..BF
//  U+E000..U+FFFF     EE..EF   80..BF   80..BF
//  U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
//  U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
//  U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
//
using namespace simd;

namespace utf8_validation {

  //
  // Find special case UTF-8 errors where the character is technically readable (has the right length)
  // but the *value* is disallowed.
  //
  // This includes overlong encodings, surrogates and values too large for Unicode.
  //
  // It turns out the bad character ranges can all be detected by looking at the first 12 bits of the
  // UTF-8 encoded character (i.e. all of byte 1, and the high 4 bits of byte 2). This algorithm does a
  // 3 4-bit table lookups, identifying which errors that 4 bits could match, and then &'s them together.
  // If all 3 lookups detect the same error, it's an error.
  //
  really_inline simd8<uint8_t> check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1) {
    //
    // These are the errors we're going to match for bytes 1-2, by looking at the first three
    // nibbles of the character: <high bits of byte 1>> & <low bits of byte 1> & <high bits of byte 2>
    //
    static const int OVERLONG_2  = 0x01; // 1100000_ 10______ (technically we match 10______ but we could match ________, they both yield errors either way)
    static const int OVERLONG_3  = 0x02; // 11100000 100_____ ________
    static const int OVERLONG_4  = 0x04; // 11110000 1000____ ________ ________
    static const int SURROGATE   = 0x08; // 11101101 [101_]____
    static const int TOO_LARGE   = 0x10; // 11110100 (1001|101_)____
    static const int TOO_LARGE_2 = 0x20; // 1111(1___|011_|0101) 10______

    // After processing the rest of byte 1 (the low bits), we're still not done--we have to check
    // byte 2 to be sure which things are errors and which aren't.
    // Since high_bits is byte 5, byte 2 is high_bits.prev<3>
    static const int CARRY = OVERLONG_2 | TOO_LARGE_2;
    const simd8<uint8_t> byte_2_high = input.shr<4>().lookup_16<uint8_t>(
        // ASCII: ________ [0___]____
        CARRY, CARRY, CARRY, CARRY,
        // ASCII: ________ [0___]____
        CARRY, CARRY, CARRY, CARRY,
        // Continuations: ________ [10__]____
        CARRY | OVERLONG_3 | OVERLONG_4, // ________ [1000]____
        CARRY | OVERLONG_3 | TOO_LARGE,  // ________ [1001]____
        CARRY | TOO_LARGE  | SURROGATE,  // ________ [1010]____
        CARRY | TOO_LARGE  | SURROGATE,  // ________ [1011]____
        // Multibyte Leads: ________ [11__]____
        CARRY, CARRY, CARRY, CARRY
    );

    const simd8<uint8_t> byte_1_high = prev1.shr<4>().lookup_16<uint8_t>(
      // [0___]____ (ASCII)
      0, 0, 0, 0,                          
      0, 0, 0, 0,
      // [10__]____ (continuation)
      0, 0, 0, 0,
      // [11__]____ (2+-byte leads)
      OVERLONG_2, 0,                       // [110_]____ (2-byte lead)
      OVERLONG_3 | SURROGATE,              // [1110]____ (3-byte lead)
      OVERLONG_4 | TOO_LARGE | TOO_LARGE_2 // [1111]____ (4+-byte lead)
    );

    const simd8<uint8_t> byte_1_low = (prev1 & 0x0F).lookup_16<uint8_t>(
      // ____[00__] ________
      OVERLONG_2 | OVERLONG_3 | OVERLONG_4, // ____[0000] ________
      OVERLONG_2,                           // ____[0001] ________
      0, 0,
      // ____[01__] ________
      TOO_LARGE,                            // ____[0100] ________
      TOO_LARGE_2,
      TOO_LARGE_2,
      TOO_LARGE_2,
      // ____[10__] ________
      TOO_LARGE_2, TOO_LARGE_2, TOO_LARGE_2, TOO_LARGE_2,
      // ____[11__] ________
      TOO_LARGE_2,
      TOO_LARGE_2 | SURROGATE,                            // ____[1101] ________
      TOO_LARGE_2, TOO_LARGE_2
    );

    return byte_1_high & byte_1_low & byte_2_high;
  }

  //
  // Validate the length of multibyte characters (that each multibyte character has the right number
  // of continuation characters, and that all continuation characters are part of a multibyte
  // character).
  //
  // Algorithm
  // =========
  //
  // This algorithm compares *expected* continuation characters with *actual* continuation bytes,
  // and emits an error anytime there is a mismatch.
  //
  // For example, in the string "𝄞₿֏ab", which has a 4-, 3-, 2- and 1-byte
  // characters, the file will look like this:
  //
  // | Character             | 𝄞  |    |    |    | ₿  |    |    | ֏  |    | a  | b  |
  // |-----------------------|----|----|----|----|----|----|----|----|----|----|----|
  // | Character Length      |  4 |    |    |    |  3 |    |    |  2 |    |  1 |  1 |
  // | Byte                  | F0 | 9D | 84 | 9E | E2 | 82 | BF | D6 | 8F | 61 | 62 |
  // | is_second_byte        |    |  X |    |    |    |  X |    |    |  X |    |    |
  // | is_third_byte         |    |    |  X |    |    |    |  X |    |    |    |    |
  // | is_fourth_byte        |    |    |    |  X |    |    |    |    |    |    |    |
  // | expected_continuation |    |  X |  X |  X |    |  X |  X |    |  X |    |    |
  // | is_continuation       |    |  X |  X |  X |    |  X |  X |    |  X |    |    |
  //
  // The errors here are basically (Second Byte OR Third Byte OR Fourth Byte == Continuation):
  //
  // - **Extra Continuations:** Any continuation that is not a second, third or fourth byte is not
  //   part of a valid 2-, 3- or 4-byte character and is thus an error. It could be that it's just
  //   floating around extra outside of any character, or that there is an illegal 5-byte character,
  //   or maybe it's at the beginning of the file before any characters have started; but it's an
  //   error in all these cases.
  // - **Missing Continuations:** Any second, third or fourth byte that *isn't* a continuation is an error, because that means
  //   we started a new character before we were finished with the current one.
  //
  // Getting the Previous Bytes
  // --------------------------
  //
  // Because we want to know if a byte is the *second* (or third, or fourth) byte of a multibyte
  // character, we need to "shift the bytes" to find that out. This is what they mean:
  //
  // - `is_continuation`: if the current byte is a continuation.
  // - `is_second_byte`: if 1 byte back is the start of a 2-, 3- or 4-byte character.
  // - `is_third_byte`: if 2 bytes back is the start of a 3- or 4-byte character.
  // - `is_fourth_byte`: if 3 bytes back is the start of a 4-byte character.
  //
  // We use shuffles to go n bytes back, selecting part of the current `input` and part of the
  // `prev_input` (search for `.prev<1>`, `.prev<2>`, etc.). These are passed in by the caller
  // function, because the 1-byte-back data is used by other checks as well.
  //
  // Getting the Continuation Mask
  // -----------------------------
  //
  // Once we have the right bytes, we have to get the masks. To do this, we treat UTF-8 bytes as
  // numbers, using signed `<` and `>` operations to check if they are continuations or leads.
  // In fact, we treat the numbers as *signed*, partly because it helps us, and partly because
  // Intel's SIMD presently only offers signed `<` and `>` operations (not unsigned ones).
  //
  // In UTF-8, bytes that start with the bits 110, 1110 and 11110 are 2-, 3- and 4-byte "leads,"
  // respectively, meaning they expect to have 1, 2 and 3 "continuation bytes" after them.
  // Continuation bytes start with 10, and ASCII (1-byte characters) starts with 0.
  //
  // When treated as signed numbers, they look like this:
  //
  // | Type         | High Bits  | Binary Range | Signed |
  // |--------------|------------|--------------|--------|
  // | ASCII        | `0`        | `01111111`   |   127  |
  // |              |            | `00000000`   |     0  |
  // | 4+-Byte Lead | `1111`     | `11111111`   |    -1  |
  // |              |            | `11110000    |   -16  |
  // | 3-Byte Lead  | `1110`     | `11101111`   |   -17  |
  // |              |            | `11100000    |   -32  |
  // | 2-Byte Lead  | `110`      | `11011111`   |   -33  |
  // |              |            | `11000000    |   -64  |
  // | Continuation | `10`       | `10111111`   |   -65  |
  // |              |            | `10000000    |  -128  |
  //
  // This makes it pretty easy to get the continuation mask! It's just a single comparison:
  //
  // ```
  // is_continuation = input < -64`
  // ```
  //
  // We can do something similar for the others, but it takes two comparisons instead of one: "is
  // the start of a 4-byte character" is `< -32` and `> -65`, for example. And 2+ bytes is `< 0` and
  // `> -64`. Surely we can do better, they're right next to each other!
  //
  // Getting the is_xxx Masks: Shifting the Range
  // --------------------------------------------
  //
  // Notice *why* continuations were a single comparison. The actual *range* would require two
  // comparisons--`< -64` and `> -129`--but all characters are always greater than -128, so we get
  // that for free. In fact, if we had *unsigned* comparisons, 2+, 3+ and 4+ comparisons would be
  // just as easy: 4+ would be `> 239`, 3+ would be `> 223`, and 2+ would be `> 191`.
  //
  // Instead, we add 128 to each byte, shifting the range up to make comparison easy. This wraps
  // ASCII down into the negative, and puts 4+-Byte Lead at the top:
  //
  // | Type                 | High Bits  | Binary Range | Signed |
  // |----------------------|------------|--------------|-------|
  // | 4+-Byte Lead (+ 127) | `0111`     | `01111111`   |   127 |
  // |                      |            | `01110000    |   112 |
  // |----------------------|------------|--------------|-------|
  // | 3-Byte Lead (+ 127)  | `0110`     | `01101111`   |   111 |
  // |                      |            | `01100000    |    96 |
  // |----------------------|------------|--------------|-------|
  // | 2-Byte Lead (+ 127)  | `010`      | `01011111`   |    95 |
  // |                      |            | `01000000    |    64 |
  // |----------------------|------------|--------------|-------|
  // | Continuation (+ 127) | `00`       | `00111111`   |    63 |
  // |                      |            | `00000000    |     0 |
  // |----------------------|------------|--------------|-------|
  // | ASCII (+ 127)        | `1`        | `11111111`   |    -1 |
  // |                      |            | `10000000`   |  -128 |
  // |----------------------|------------|--------------|-------|
  // 
  // *Now* we can use signed `>` on all of them:
  //
  // ```
  // prev1 = input.prev<1>
  // prev2 = input.prev<2>
  // prev3 = input.prev<3>
  // prev1_flipped = input.prev<1>(prev_input) ^ 0x80; // Same as `+ 128`
  // prev2_flipped = input.prev<2>(prev_input) ^ 0x80; // Same as `+ 128`
  // prev3_flipped = input.prev<3>(prev_input) ^ 0x80; // Same as `+ 128`
  // is_second_byte = prev1_flipped > 63;  // 2+-byte lead
  // is_third_byte  = prev2_flipped > 95;  // 3+-byte lead
  // is_fourth_byte = prev3_flipped > 111; // 4+-byte lead
  // ```
  //
  // NOTE: we use `^ 0x80` instead of `+ 128` in the code, which accomplishes the same thing, and even takes the same number
  // of cycles as `+`, but on many Intel architectures can be parallelized better (you can do 3
  // `^`'s at a time on Haswell, but only 2 `+`'s).
  //
  // That doesn't look like it saved us any instructions, did it? Well, because we're adding the
  // same number to all of them, we can save one of those `+ 128` operations by assembling
  // `prev2_flipped` out of prev 1 and prev 3 instead of assembling it from input and adding 128
  // to it. One more instruction saved!
  //
  // ```
  // prev1 = input.prev<1>
  // prev3 = input.prev<3>
  // prev1_flipped = prev1 ^ 0x80; // Same as `+ 128`
  // prev3_flipped = prev3 ^ 0x80; // Same as `+ 128`
  // prev2_flipped = prev1_flipped.concat<2>(prev3_flipped): // <shuffle: take the first 2 bytes from prev1 and the rest from prev3  
  // ```
  //
  // ### Bringing It All Together: Detecting the Errors
  //
  // At this point, we have `is_continuation`, `is_first_byte`, `is_second_byte` and `is_third_byte`.
  // All we have left to do is check if they match!
  //
  // ```
  // return (is_second_byte | is_third_byte | is_fourth_byte) ^ is_continuation;
  // ```
  //
  // But wait--there's more. The above statement is only 3 operations, but they *cannot be done in
  // parallel*. You have to do 2 `|`'s and then 1 `&`. Haswell, at least, has 3 ports that can do
  // bitwise operations, and we're only using 1!
  //
  // Epilogue: Addition For Booleans
  // -------------------------------
  //
  // There is one big case the above code doesn't explicitly talk about--what if is_second_byte
  // and is_third_byte are BOTH true? That means there is a 3-byte and 2-byte character right next
  // to each other (or any combination), and the continuation could be part of either of them!
  // Our algorithm using `&` and `|` won't detect that the continuation byte is problematic.
  //
  // Never fear, though. If that situation occurs, we'll already have detected that the second
  // leading byte was an error, because it was supposed to be a part of the preceding multibyte
  // character, but it *wasn't a continuation*.
  //
  // We could stop here, but it turns out that we can fix it using `+` and `-` instead of `|` and
  // `&`, which is both interesting and possibly useful (even though we're not using it here). It
  // exploits the fact that in SIMD, a *true* value is -1, and a *false* value is 0. So those
  // comparisons were giving us numbers!
  //
  // Given that, if you do `is_second_byte + is_third_byte + is_fourth_byte`, under normal
  // circumstances you will either get 0 (0 + 0 + 0) or -1 (-1 + 0 + 0, etc.). Thus,
  // `(is_second_byte + is_third_byte + is_fourth_byte) - is_continuation` will yield 0 only if
  // *both* or *neither* are 0 (0-0 or -1 - -1). You'll get 1 or -1 if they are different. Because
  // *any* nonzero value is treated as an error (not just -1), we're just fine here :)
  //
  // Further, if *more than one* multibyte character overlaps,
  // `is_second_byte + is_third_byte + is_fourth_byte` will be -2 or -3! Subtracting `is_continuation`
  // from *that* is guaranteed to give you a nonzero value (-1, -2 or -3). So it'll always be
  // considered an error.
  //
  // One reason you might want to do this is parallelism. ^ and | are not associative, so
  // (A | B | C) ^ D will always be three operations in a row: either you do A | B -> | C -> ^ D, or
  // you do B | C -> | A -> ^ D. But addition and subtraction *are* associative: (A + B + C) - D can
  // be written as `(A + B) + (C - D)`. This means you can do A + B and C - D at the same time, and
  // then adds the result together. Same number of operations, but if the processor can run
  // independent things in parallel (which most can), it runs faster.
  //
  // This doesn't help us on Intel, but might help us elsewhere: on Haswell, at least, | and ^ have
  // a super nice advantage in that more of them can be run at the same time (they can run on 3
  // ports, while + and - can run on 2)! This means that we can do A | B while we're still doing C,
  // saving us the cycle we would have earned by using +. Even more, using an instruction with a
  // wider array of ports can help *other* code run ahead, too, since these instructions can "get
  // out of the way," running on a port other instructions can't.
  // 
  // Epilogue II: One More Trick
  // ---------------------------
  //
  // There's one more relevant trick up our sleeve, it turns out: it turns out on Intel we can "pay
  // for" the (prev<1> + 128) instruction, because it can be used to save an instruction in
  // check_special_cases()--but we'll talk about that there :)
  //
  really_inline simd8<uint8_t> check_multibyte_lengths(simd8<uint8_t> input, simd8<uint8_t> prev_input, simd8<uint8_t> prev1) {
    simd8<uint8_t> prev2 = input.prev<2>(prev_input);
    simd8<uint8_t> prev3 = input.prev<3>(prev_input);

    // Cont is 10000000-101111111 (-65...-128)
    simd8<bool> is_continuation = simd8<int8_t>(input) < int8_t(-64);
    // must_be_continuation is architecture-specific because Intel doesn't have unsigned comparisons
    return simd8<uint8_t>(must_be_continuation(prev1, prev2, prev3) ^ is_continuation);
  }

  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  really_inline simd8<uint8_t> is_incomplete(simd8<uint8_t> input) {
    // If the previous input's last 3 bytes match this, they're too short (they ended at EOF):
    // ... 1111____ 111_____ 11______
    static const uint8_t max_array[32] = {
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 0b11110000u-1, 0b11100000u-1, 0b11000000u-1
    };
    const simd8<uint8_t> max_value(&max_array[sizeof(max_array)-sizeof(simd8<uint8_t>)]);
    return input.gt_bits(max_value);
  }

  struct utf8_checker {
    // If this is nonzero, there has been a UTF-8 error.
    simd8<uint8_t> error;
    // The last input we received
    simd8<uint8_t> prev_input_block;
    // Whether the last input we received was incomplete (used for ASCII fast path)
    simd8<uint8_t> prev_incomplete;

    //
    // Check whether the current bytes are valid UTF-8.
    //
    really_inline void check_utf8_bytes(const simd8<uint8_t> input, const simd8<uint8_t> prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      simd8<uint8_t> prev1 = input.prev<1>(prev_input);
      this->error |= check_special_cases(input, prev1);
      this->error |= check_multibyte_lengths(input, prev_input, prev1);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short.
    really_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error |= this->prev_incomplete;
    }

    really_inline void check_next_input(simd8x64<uint8_t> input) {
      if (likely(is_ascii(input))) {
        // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
        // possibly finish them.
        this->error |= this->prev_incomplete;
      } else {
        this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
        for (int i=1; i<simd8x64<uint8_t>::NUM_CHUNKS; i++) {
          this->check_utf8_bytes(input.chunks[i], input.chunks[i-1]);
        }
        this->prev_incomplete = is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1]);
        this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1];
      }
    }

    really_inline ErrorValues errors() {
      return this->error.any_bits_set_anywhere() ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
    }

  }; // struct utf8_checker
}

using utf8_validation::utf8_checker;
/* end file src/generic/utf8_lookup2_algorithm.h */
/* begin file src/generic/stage1_find_marks.h */
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

namespace stage1 {

class bit_indexer {
public:
  uint32_t *tail;

  bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // base_ptr[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so base_ptr
  // needs to be large enough to handle this
  really_inline void write_indexes(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
        return;
    uint32_t cnt = hamming(bits);

    // Do the first 8 all together
    for (int i=0; i<8; i++) {
      this->tail[i] = idx + trailing_zeroes(bits);
      bits = clear_lowest_bit(bits);
    }

    // Do the next 8 all together (we hope in most cases it won't happen at all
    // and the branch is easily predicted).
    if (unlikely(cnt > 8)) {
      for (int i=8; i<16; i++) {
        this->tail[i] = idx + trailing_zeroes(bits);
        bits = clear_lowest_bit(bits);
      }

      // Most files don't have 16+ structurals per block, so we take several basically guaranteed
      // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
      // or the start of a value ("abc" true 123) every four characters.
      if (unlikely(cnt > 16)) {
        uint32_t i = 16;
        do {
          this->tail[i] = idx + trailing_zeroes(bits);
          bits = clear_lowest_bit(bits);
          i++;
        } while (i < cnt);
      }
    }

    this->tail += cnt;
  }
};

class json_structural_scanner {
public:
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
  uint64_t prev_structurals = 0;
  // Errors with unescaped characters in strings (ASCII codepoints < 0x20)
  uint64_t unescaped_chars_error = 0;
  bit_indexer structural_indexes;

  json_structural_scanner(uint32_t *_structural_indexes) : structural_indexes{_structural_indexes} {}

  //
  // Finish the scan and return any errors.
  //
  // This may detect errors as well, such as unclosed string and certain UTF-8 errors.
  // if streaming is set to true, an unclosed string is allowed.
  //
  really_inline ErrorValues detect_errors_on_eof(bool streaming = false);

  //
  // Return a mask of all string characters plus end quotes.
  //
  // prev_escaped is overflow saying whether the next character is escaped.
  // prev_in_string is overflow saying whether we're still in a string.
  //
  // Backslash sequences outside of quotes will be detected in stage 2.
  //
  really_inline uint64_t find_strings(const simd::simd8x64<uint8_t> in);

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
  really_inline uint64_t find_potential_structurals(const simd::simd8x64<uint8_t> in);

  //
  // Find the important bits of JSON in a STEP_SIZE-byte chunk, and add them to structural_indexes.
  //
  template<size_t STEP_SIZE>
  really_inline void scan_step(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker);

  //
  // Parse the entire input in STEP_SIZE-byte chunks.
  //
  template<size_t STEP_SIZE>
  really_inline void scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker);
};

// Routines to print masks and text for debugging bitmask operations
UNUSED static char * format_input_text(const simd8x64<uint8_t> in) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  in.store((uint8_t*)buf);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    if (buf[i] < ' ') { buf[i] = '_'; }
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

UNUSED static char * format_mask(uint64_t mask) {
  static char *buf = (char*)malloc(64 + 1);
  for (size_t i=0; i<64; i++) {
    buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
  }
  buf[64] = '\0';
  return buf;
}

//
// Finds escaped characters (characters following \).
//
// Handles runs of backslashes like \\\" and \\\\" correctly (yielding 0101 and 01010, respectively).
//
// Does this by:
// - Shift the escape mask to get potentially escaped characters (characters after backslashes).
// - Mask escaped sequences that start on *even* bits with 1010101010 (odd bits are escaped, even bits are not)
// - Mask escaped sequences that start on *odd* bits with 0101010101 (even bits are escaped, odd bits are not)
//
// To distinguish between escaped sequences starting on even/odd bits, it finds the start of all
// escape sequences, filters out the ones that start on even bits, and adds that to the mask of
// escape sequences. This causes the addition to clear out the sequences starting on odd bits (since
// the start bit causes a carry), and leaves even-bit sequences alone.
//
// Example:
//
// text           |  \\\ | \\\"\\\" \\\" \\"\\" |
// escape         |  xxx |  xx xxx  xxx  xx xx  | Removed overflow backslash; will | it into follows_escape
// odd_starts     |  x   |  x       x       x   | escape & ~even_bits & ~follows_escape
// even_seq       |     c|    cxxx     c xx   c | c = carry bit -- will be masked out later
// invert_mask    |      |     cxxx     c xx   c| even_seq << 1
// follows_escape |   xx | x xx xxx  xxx  xx xx | Includes overflow bit
// escaped        |   x  | x x  x x  x x  x  x  |
// desired        |   x  | x x  x x  x x  x  x  |
// text           |  \\\ | \\\"\\\" \\\" \\"\\" |
//
really_inline uint64_t find_escaped(uint64_t escape, uint64_t &escaped_overflow) {
  // If there was overflow, pretend the first character isn't a backslash
  escape &= ~escaped_overflow;
  uint64_t follows_escape = escape << 1 | escaped_overflow;

  // Get sequences starting on even bits by clearing out the odd series using +
  const uint64_t even_bits = 0x5555555555555555ULL;
  uint64_t odd_sequence_starts = escape & ~even_bits & ~follows_escape;
  uint64_t sequences_starting_on_even_bits;
  escaped_overflow = add_overflow(odd_sequence_starts, escape, &sequences_starting_on_even_bits);
  uint64_t invert_mask = sequences_starting_on_even_bits << 1; // The mask we want to return is the *escaped* bits, not escapes.

  // Mask every other backslashed character as an escaped character
  // Flip the mask for sequences that start on even bits, to correct them
  return (even_bits ^ invert_mask) & follows_escape;
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
really_inline uint64_t follows(const uint64_t match, const uint64_t filler, uint64_t &overflow) {
  uint64_t follows_match = follows(match, overflow);
  uint64_t result;
  overflow |= add_overflow(follows_match, filler, &result);
  return result;
}

really_inline ErrorValues json_structural_scanner::detect_errors_on_eof(bool streaming) {
  if ((prev_in_string) and (not streaming)) {
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
really_inline uint64_t json_structural_scanner::find_strings(const simd::simd8x64<uint8_t> in) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = find_escaped(backslash, prev_escaped);
  const uint64_t quote = in.eq('"') & ~escaped;
  // prefix_xor flips on bits inside the string (and flips off the end quote).
  const uint64_t in_string = prefix_xor(quote) ^ prev_in_string;
  /* right shift of a signed value expected to be well-defined and standard
  * compliant as of C++20,
  * John Regher from Utah U. says this is fine code */
  prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
  // Use ^ to turn the beginning quote off, and the end quote on.
  return in_string ^ quote;
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
really_inline uint64_t json_structural_scanner::find_potential_structurals(const simd::simd8x64<uint8_t> in) {
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

//
// Find the important bits of JSON in a 128-byte chunk, and add them to structural_indexes.
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
template<>
really_inline void json_structural_scanner::scan_step<128>(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker) {
  //
  // Load up all 128 bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);
  simd::simd8x64<uint8_t> in_2(buf+64);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = this->find_strings(in_1);
  uint64_t structurals_1 = this->find_potential_structurals(in_1);
  uint64_t string_2 = this->find_strings(in_2);
  uint64_t structurals_2 = this->find_potential_structurals(in_2);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_1 & ~string_1;
  this->unescaped_chars_error |= unescaped_1 & string_1;

  uint64_t unescaped_2 = in_2.lteq(0x1F);
  utf8_checker.check_next_input(in_2);
  this->structural_indexes.write_indexes(idx, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_2 & ~string_2;
  this->unescaped_chars_error |= unescaped_2 & string_2;
}

//
// Find the important bits of JSON in a 64-byte chunk, and add them to structural_indexes.
//
template<>
really_inline void json_structural_scanner::scan_step<64>(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker) {
  //
  // Load up bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = this->find_strings(in_1);
  uint64_t structurals_1 = this->find_potential_structurals(in_1);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_1 & ~string_1;
  this->unescaped_chars_error |= unescaped_1 & string_1;
}

template<size_t STEP_SIZE>
really_inline void json_structural_scanner::scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker) {
  size_t lenminusstep = len < STEP_SIZE ? 0 : len - STEP_SIZE;
  size_t idx = 0;

  for (; idx < lenminusstep; idx += STEP_SIZE) {
    this->scan_step<STEP_SIZE>(&buf[idx], idx, utf8_checker);
  }

  /* If we have a final chunk of less than STEP_SIZE bytes, pad it to STEP_SIZE with
  * spaces  before processing it (otherwise, we risk invalidating the UTF-8
  * checks). */
  if (likely(idx < len)) {
    uint8_t tmp_buf[STEP_SIZE];
    memset(tmp_buf, 0x20, STEP_SIZE);
    memcpy(tmp_buf, buf + idx, len - idx);
    this->scan_step<STEP_SIZE>(&tmp_buf[0], idx, utf8_checker);
    idx += STEP_SIZE;
  }

  /* finally, flatten out the remaining structurals from the last iteration */
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals);
}

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
template<size_t STEP_SIZE>
int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  if (unlikely(len > pj.byte_capacity)) {
    return simdjson::CAPACITY;
  }
  utf8_checker utf8_checker{};
  json_structural_scanner scanner{pj.structural_indexes.get()};
  scanner.scan<STEP_SIZE>(buf, len, utf8_checker);
  // we might tolerate an unclosed string if streaming is true
  simdjson::ErrorValues error = scanner.detect_errors_on_eof(streaming);
  if (unlikely(error != simdjson::SUCCESS)) {
    return error;
  }
  pj.n_structural_indexes = scanner.structural_indexes.tail - pj.structural_indexes.get();
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
  return utf8_checker.errors();
}

} // namespace stage1
/* end file src/generic/stage1_find_marks.h */

} // namespace westmere
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {

template <>
int find_structural_bits<Architecture::WESTMERE>(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  return westmere::stage1::find_structural_bits<64>(buf, len, pj, streaming);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_WESTMERE_STAGE1_FIND_MARKS_H
/* end file src/generic/stage1_find_marks.h */
/* end file src/generic/stage1_find_marks.h */
/* begin file src/stage2_build_tape.cpp */
#include <cassert>
#include <cstring>

/* jsoncharutils.h already included: #include "jsoncharutils.h" */

using namespace simdjson;

WARN_UNUSED
really_inline bool is_valid_true_atom(const uint8_t *loc) {
  uint32_t tv = *reinterpret_cast<const uint32_t *>("true");
  uint32_t error = 0;
  uint32_t
      locval; // we want to avoid unaligned 64-bit loads (undefined in C/C++)
  // this can read up to 3 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint32_t));
  error = locval ^ tv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_false_atom(const uint8_t *loc) {
  // assume that loc starts with "f"
  uint32_t fv = *reinterpret_cast<const uint32_t *>("alse");
  uint32_t error = 0;
  uint32_t
      locval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  // this can read up to 4 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc + 1, sizeof(uint32_t));
  error = locval ^ fv;
  error |= is_not_structural_or_whitespace(loc[5]);
  return error == 0;
}

WARN_UNUSED
really_inline bool is_valid_null_atom(const uint8_t *loc) {
  uint32_t nv = *reinterpret_cast<const uint32_t *>("null");
  uint32_t error = 0;
  uint32_t
      locval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  // this can read up to 2 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(sizeof(uint32_t) - 1 <= SIMDJSON_PADDING);
  std::memcpy(&locval, loc, sizeof(uint32_t));
  error = locval ^ nv;
  error |= is_not_structural_or_whitespace(loc[4]);
  return error == 0;
}

#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

/* begin file src/arm64/stage2_build_tape.h */
#ifndef SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
#define SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H


#ifdef IS_ARM64

/* begin file src/arm64/stringparsing.h */
#ifndef SIMDJSON_ARM64_STRINGPARSING_H
#define SIMDJSON_ARM64_STRINGPARSING_H


#ifdef IS_ARM64

/* arm64/simd.h already included: #include "arm64/simd.h" */
/* jsoncharutils.h already included: #include "jsoncharutils.h" */
/* arm64/intrinsics.h already included: #include "arm64/intrinsics.h" */
/* arm64/bitmanipulation.h already included: #include "arm64/bitmanipulation.h" */

namespace simdjson::arm64 {

using namespace simd;

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
  static const uint32_t BYTES_PROCESSED = 32;
};

really_inline parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (parse_string_helper::BYTES_PROCESSED - 1));
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + sizeof(v0));
  v0.store(dst);
  v1.store(dst + sizeof(v0));

  // Getting a 64-bit bitmask is much cheaper than multiple 16-bit bitmasks on ARM; therefore, we
  // smash them together into a 64-byte mask and get the bitmask from there.
  uint64_t bs_and_quote = simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
    static_cast<uint32_t>(bs_and_quote),      // bs_bits
    static_cast<uint32_t>(bs_and_quote >> 32) // quote_bits
  };
}

/* begin file src/generic/stringparsing.h */
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
  pj.write_tape(pj.current_string_buf_loc - pj.string_buf.get(), '"');
  const uint8_t *src = &buf[offset + 1]; /* we know that buf at offset is a " */
  uint8_t *dst = pj.current_string_buf_loc + sizeof(uint32_t);
  const uint8_t *const start_of_string = dst;
  while (1) {
    parse_string_helper helper = find_bs_bits_and_quote_bits(src, dst);
    if (((helper.bs_bits - 1) & helper.quote_bits) != 0) {
      /* we encountered quotes first. Move dst to point to quotes and exit
       */

      /* find out where the quote is... */
      auto quote_dist = trailing_zeroes(helper.quote_bits);

      /* NULL termination is still handy if you expect all your strings to
       * be NULL terminated? */
      /* It comes at a small cost */
      dst[quote_dist] = 0;

      uint32_t str_length = (dst - start_of_string) + quote_dist;
      memcpy(pj.current_string_buf_loc, &str_length, sizeof(str_length));
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
      auto bs_dist = trailing_zeroes(helper.bs_bits);
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
      src += parse_string_helper::BYTES_PROCESSED;
      dst += parse_string_helper::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return true;
}
/* end file src/generic/stringparsing.h */

}
// namespace simdjson::amd64

#endif // IS_ARM64
#endif
/* end file src/generic/stringparsing.h */
/* begin file src/arm64/numberparsing.h */
#ifndef SIMDJSON_ARM64_NUMBERPARSING_H
#define SIMDJSON_ARM64_NUMBERPARSING_H

#ifdef IS_ARM64

/* arm64/intrinsics.h already included: #include "arm64/intrinsics.h" */
/* arm64/bitmanipulation.h already included: #include "arm64/bitmanipulation.h" */
/* jsoncharutils.h already included: #include "jsoncharutils.h" */
#include <cmath>
#include <limits>


#ifdef JSON_TEST_NUMBERS // for unit testing
void found_invalid_number(const uint8_t *buf);
void found_integer(int64_t result, const uint8_t *buf);
void found_unsigned_integer(uint64_t result, const uint8_t *buf);
void found_float(double result, const uint8_t *buf);
#endif

namespace simdjson::arm64 {

// we don't have SSE, so let us use a scalar function
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
static inline uint32_t parse_eight_digits_unrolled(const char *chars) {
  uint64_t val;
  memcpy(&val, chars, sizeof(uint64_t));
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return (val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32;
}

#define SWAR_NUMBER_PARSING

/* begin file src/generic/numberparsing.h */

// Allowable floating-point values range
// std::numeric_limits<double>::lowest() to std::numeric_limits<double>::max(),
// so from -1.7976e308 all the way to 1.7975e308 in binary64. The lowest
// non-zero normal values is std::numeric_limits<double>::min() or
// about 2.225074e-308.
static const double power_of_ten[] = {
    1e-308, 1e-307, 1e-306, 1e-305, 1e-304, 1e-303, 1e-302, 1e-301, 1e-300,
    1e-299, 1e-298, 1e-297, 1e-296, 1e-295, 1e-294, 1e-293, 1e-292, 1e-291,
    1e-290, 1e-289, 1e-288, 1e-287, 1e-286, 1e-285, 1e-284, 1e-283, 1e-282,
    1e-281, 1e-280, 1e-279, 1e-278, 1e-277, 1e-276, 1e-275, 1e-274, 1e-273,
    1e-272, 1e-271, 1e-270, 1e-269, 1e-268, 1e-267, 1e-266, 1e-265, 1e-264,
    1e-263, 1e-262, 1e-261, 1e-260, 1e-259, 1e-258, 1e-257, 1e-256, 1e-255,
    1e-254, 1e-253, 1e-252, 1e-251, 1e-250, 1e-249, 1e-248, 1e-247, 1e-246,
    1e-245, 1e-244, 1e-243, 1e-242, 1e-241, 1e-240, 1e-239, 1e-238, 1e-237,
    1e-236, 1e-235, 1e-234, 1e-233, 1e-232, 1e-231, 1e-230, 1e-229, 1e-228,
    1e-227, 1e-226, 1e-225, 1e-224, 1e-223, 1e-222, 1e-221, 1e-220, 1e-219,
    1e-218, 1e-217, 1e-216, 1e-215, 1e-214, 1e-213, 1e-212, 1e-211, 1e-210,
    1e-209, 1e-208, 1e-207, 1e-206, 1e-205, 1e-204, 1e-203, 1e-202, 1e-201,
    1e-200, 1e-199, 1e-198, 1e-197, 1e-196, 1e-195, 1e-194, 1e-193, 1e-192,
    1e-191, 1e-190, 1e-189, 1e-188, 1e-187, 1e-186, 1e-185, 1e-184, 1e-183,
    1e-182, 1e-181, 1e-180, 1e-179, 1e-178, 1e-177, 1e-176, 1e-175, 1e-174,
    1e-173, 1e-172, 1e-171, 1e-170, 1e-169, 1e-168, 1e-167, 1e-166, 1e-165,
    1e-164, 1e-163, 1e-162, 1e-161, 1e-160, 1e-159, 1e-158, 1e-157, 1e-156,
    1e-155, 1e-154, 1e-153, 1e-152, 1e-151, 1e-150, 1e-149, 1e-148, 1e-147,
    1e-146, 1e-145, 1e-144, 1e-143, 1e-142, 1e-141, 1e-140, 1e-139, 1e-138,
    1e-137, 1e-136, 1e-135, 1e-134, 1e-133, 1e-132, 1e-131, 1e-130, 1e-129,
    1e-128, 1e-127, 1e-126, 1e-125, 1e-124, 1e-123, 1e-122, 1e-121, 1e-120,
    1e-119, 1e-118, 1e-117, 1e-116, 1e-115, 1e-114, 1e-113, 1e-112, 1e-111,
    1e-110, 1e-109, 1e-108, 1e-107, 1e-106, 1e-105, 1e-104, 1e-103, 1e-102,
    1e-101, 1e-100, 1e-99,  1e-98,  1e-97,  1e-96,  1e-95,  1e-94,  1e-93,
    1e-92,  1e-91,  1e-90,  1e-89,  1e-88,  1e-87,  1e-86,  1e-85,  1e-84,
    1e-83,  1e-82,  1e-81,  1e-80,  1e-79,  1e-78,  1e-77,  1e-76,  1e-75,
    1e-74,  1e-73,  1e-72,  1e-71,  1e-70,  1e-69,  1e-68,  1e-67,  1e-66,
    1e-65,  1e-64,  1e-63,  1e-62,  1e-61,  1e-60,  1e-59,  1e-58,  1e-57,
    1e-56,  1e-55,  1e-54,  1e-53,  1e-52,  1e-51,  1e-50,  1e-49,  1e-48,
    1e-47,  1e-46,  1e-45,  1e-44,  1e-43,  1e-42,  1e-41,  1e-40,  1e-39,
    1e-38,  1e-37,  1e-36,  1e-35,  1e-34,  1e-33,  1e-32,  1e-31,  1e-30,
    1e-29,  1e-28,  1e-27,  1e-26,  1e-25,  1e-24,  1e-23,  1e-22,  1e-21,
    1e-20,  1e-19,  1e-18,  1e-17,  1e-16,  1e-15,  1e-14,  1e-13,  1e-12,
    1e-11,  1e-10,  1e-9,   1e-8,   1e-7,   1e-6,   1e-5,   1e-4,   1e-3,
    1e-2,   1e-1,   1e0,    1e1,    1e2,    1e3,    1e4,    1e5,    1e6,
    1e7,    1e8,    1e9,    1e10,   1e11,   1e12,   1e13,   1e14,   1e15,
    1e16,   1e17,   1e18,   1e19,   1e20,   1e21,   1e22,   1e23,   1e24,
    1e25,   1e26,   1e27,   1e28,   1e29,   1e30,   1e31,   1e32,   1e33,
    1e34,   1e35,   1e36,   1e37,   1e38,   1e39,   1e40,   1e41,   1e42,
    1e43,   1e44,   1e45,   1e46,   1e47,   1e48,   1e49,   1e50,   1e51,
    1e52,   1e53,   1e54,   1e55,   1e56,   1e57,   1e58,   1e59,   1e60,
    1e61,   1e62,   1e63,   1e64,   1e65,   1e66,   1e67,   1e68,   1e69,
    1e70,   1e71,   1e72,   1e73,   1e74,   1e75,   1e76,   1e77,   1e78,
    1e79,   1e80,   1e81,   1e82,   1e83,   1e84,   1e85,   1e86,   1e87,
    1e88,   1e89,   1e90,   1e91,   1e92,   1e93,   1e94,   1e95,   1e96,
    1e97,   1e98,   1e99,   1e100,  1e101,  1e102,  1e103,  1e104,  1e105,
    1e106,  1e107,  1e108,  1e109,  1e110,  1e111,  1e112,  1e113,  1e114,
    1e115,  1e116,  1e117,  1e118,  1e119,  1e120,  1e121,  1e122,  1e123,
    1e124,  1e125,  1e126,  1e127,  1e128,  1e129,  1e130,  1e131,  1e132,
    1e133,  1e134,  1e135,  1e136,  1e137,  1e138,  1e139,  1e140,  1e141,
    1e142,  1e143,  1e144,  1e145,  1e146,  1e147,  1e148,  1e149,  1e150,
    1e151,  1e152,  1e153,  1e154,  1e155,  1e156,  1e157,  1e158,  1e159,
    1e160,  1e161,  1e162,  1e163,  1e164,  1e165,  1e166,  1e167,  1e168,
    1e169,  1e170,  1e171,  1e172,  1e173,  1e174,  1e175,  1e176,  1e177,
    1e178,  1e179,  1e180,  1e181,  1e182,  1e183,  1e184,  1e185,  1e186,
    1e187,  1e188,  1e189,  1e190,  1e191,  1e192,  1e193,  1e194,  1e195,
    1e196,  1e197,  1e198,  1e199,  1e200,  1e201,  1e202,  1e203,  1e204,
    1e205,  1e206,  1e207,  1e208,  1e209,  1e210,  1e211,  1e212,  1e213,
    1e214,  1e215,  1e216,  1e217,  1e218,  1e219,  1e220,  1e221,  1e222,
    1e223,  1e224,  1e225,  1e226,  1e227,  1e228,  1e229,  1e230,  1e231,
    1e232,  1e233,  1e234,  1e235,  1e236,  1e237,  1e238,  1e239,  1e240,
    1e241,  1e242,  1e243,  1e244,  1e245,  1e246,  1e247,  1e248,  1e249,
    1e250,  1e251,  1e252,  1e253,  1e254,  1e255,  1e256,  1e257,  1e258,
    1e259,  1e260,  1e261,  1e262,  1e263,  1e264,  1e265,  1e266,  1e267,
    1e268,  1e269,  1e270,  1e271,  1e272,  1e273,  1e274,  1e275,  1e276,
    1e277,  1e278,  1e279,  1e280,  1e281,  1e282,  1e283,  1e284,  1e285,
    1e286,  1e287,  1e288,  1e289,  1e290,  1e291,  1e292,  1e293,  1e294,
    1e295,  1e296,  1e297,  1e298,  1e299,  1e300,  1e301,  1e302,  1e303,
    1e304,  1e305,  1e306,  1e307,  1e308};

static inline bool is_integer(char c) {
  return (c >= '0' && c <= '9');
  // this gets compiled to (uint8_t)(c - '0') <= 9 on all decent compilers
}

// We need to check that the character following a zero is valid. This is
// probably frequent and it is hard than it looks. We are building all of this
// just to differentiate between 0x1 (invalid), 0,1 (valid) 0e1 (valid)...
const bool structural_or_whitespace_or_exponent_or_decimal_negated[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

really_inline bool
is_not_structural_or_whitespace_or_exponent_or_decimal(unsigned char c) {
  return structural_or_whitespace_or_exponent_or_decimal_negated[c];
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
static inline bool is_made_of_eight_digits_fast(const char *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING);
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}


//
// This function computes base * 10 ^ (- negative_exponent ).
// It is only even going to be used when negative_exponent is tiny.
static double subnormal_power10(double base, int64_t negative_exponent) {
    // avoid integer overflows in the pow expression, those values would
    // become zero anyway.
    if(negative_exponent < -1000) {
        return 0;
    }

  // this is probably not going to be fast
  return base * 1e-308 * pow(10, negative_exponent + 308);
}

// called by parse_number when we know that the output is a float,
// but where there might be some integer overflow. The trick here is to
// parse using floats from the start.
// Do not call this function directly as it skips some of the checks from
// parse_number
//
// This function will almost never be called!!!
//
// Note: a redesign could avoid this function entirely.
//
static never_inline bool parse_float(const uint8_t *const buf, ParsedJson &pj,
                                     const uint32_t offset, bool found_minus) {
  const char *p = reinterpret_cast<const char *>(buf + offset);
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
  }
  long double i;
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    i = 0;
  } else {
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    while (is_integer(*p)) {
      digit = *p - '0';
      i = 10 * i + digit;
      ++p;
    }
  }
  if ('.' == *p) {
    ++p;
    int fractional_weight = 308;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;

      fractional_weight--;
      i = i + digit * (fractional_weight >= 0 ? power_of_ten[fractional_weight]
                                              : 0);
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      fractional_weight--;
      i = i + digit * (fractional_weight >= 0 ? power_of_ten[fractional_weight]
                                              : 0);
    }
  }
  if (('e' == *p) || ('E' == *p)) {
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    int64_t exp_number = digit; // exponential part
    p++;
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (is_integer(*p)) {
      if (exp_number > 0x100000000) { // we need to check for overflows
// we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (unlikely(exp_number > 308)) {
      // this path is unlikely
      if (neg_exp) {
        // We either have zero or a subnormal.
        // We expect this to be uncommon so we go through a slow path.
        i = subnormal_power10(i, -exp_number);
      } else {
// We know for sure that we have a number that is too large,
// we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
    } else {
      int exponent = (neg_exp ? -exp_number : exp_number);
      // we have that exp_number is [0,308] so that
      // exponent is [-308,308] so that
      // 308 + exponent is in [0, 2 * 308]
      i *= power_of_ten[308 + exponent];
    }
  }
  if (is_not_structural_or_whitespace(*p)) {
    return false;
  }
  // check that we can go from long double to double safely.
  if(i > std::numeric_limits<double>::max()) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
  }
  double d = negative ? -i : i;
  pj.write_tape_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
  found_float(d, buf + offset);
#endif
  return is_structural_or_whitespace(*p);
}

// called by parse_number when we know that the output is an integer,
// but where there might be some integer overflow.
// we want to catch overflows!
// Do not call this function directly as it skips some of the checks from
// parse_number
//
// This function will almost never be called!!!
//
static never_inline bool parse_large_integer(const uint8_t *const buf,
                                             ParsedJson &pj,
                                             const uint32_t offset,
                                             bool found_minus) {
  const char *p = reinterpret_cast<const char *>(buf + offset);

  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
  }
  uint64_t i;
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    i = 0;
  } else {
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      if (mul_overflow(i, 10, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false; // overflow
      }
      if (add_overflow(i, digit, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false; // overflow
      }
      ++p;
    }
  }
  if (negative) {
    if (i > 0x8000000000000000) {
       // overflows!
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false; // overflow
    } else if (i == 0x8000000000000000) {
      // In two's complement, we cannot represent 0x8000000000000000
      // as a positive signed integer, but the negative version is 
      // possible.
      constexpr int64_t signed_answer = INT64_MIN;
      pj.write_tape_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, buf + offset);
#endif
    } else {
      // we can negate safely
      int64_t signed_answer = -static_cast<int64_t>(i);
      pj.write_tape_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, buf + offset);
#endif
    }
  } else {
    // we have a positive integer, the contract is that
    // we try to represent it as a signed integer and only 
    // fallback on unsigned integers if absolutely necessary.
    if(i < 0x8000000000000000) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(i, buf + offset);
#endif
      pj.write_tape_s64(i);
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_unsigned_integer(i, buf + offset);
#endif
      pj.write_tape_u64(i);
    }
  }
  return is_structural_or_whitespace(*p);
}

// parse the number at buf + offset
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0 or 1) at high speed.
static really_inline bool parse_number(const uint8_t *const buf, ParsedJson &pj,
                                       const uint32_t offset,
                                       bool found_minus) {
#ifdef SIMDJSON_SKIPNUMBERPARSING // for performance analysis, it is sometimes
                                  // useful to skip parsing
  pj.write_tape_s64(0);           // always write zero
  return true;                    // always succeeds
#else
  const char *p = reinterpret_cast<const char *>(buf + offset);
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
    if (!is_integer(*p)) { // a negative sign must be followed by an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
  }
  const char *const start_digits = p;

  uint64_t i;      // an unsigned int avoids signed overflows (which are bad)
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    if (is_not_structural_or_whitespace_or_exponent_or_decimal(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    i = 0;
  } else {
    if (!(is_integer(*p))) { // must start with an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      // a multiplication by 10 is cheaper than an arbitrary integer
      // multiplication
      i = 10 * i + digit; // might overflow, we will handle the overflow later
      ++p;
    }
  }
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true; // At this point we know that we have a float
    // we continue with the fiction that we have an integer. If the
    // floating point number is representable as x * 10^z for some integer
    // z that fits in 53 bits, then we will be able to convert back the
    // the integer into a float in a lossless manner.
    ++p;
    const char *const first_after_period = p;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // might overflow + multiplication by 10 is likely
                          // cheaper than arbitrary mult.
      // we will handle the overflow later
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
#ifdef SWAR_NUMBER_PARSING
    // this helps if we have lots of decimals!
    // this turns out to be frequent enough.
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
    }
#endif
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
                          // because we have parse_highprecision_float later.
    }
    exponent = first_after_period - p;
  }
  int digit_count =
      p - start_digits - 1; // used later to guard against overflows
  int64_t exp_number = 0;   // exponential part
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    exp_number = digit;
    p++;
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (is_integer(*p)) {
      if (exp_number > 0x100000000) { // we need to check for overflows
                                      // we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    exponent += (neg_exp ? -exp_number : exp_number);
  }
  if (is_float) {
    uint64_t power_index = 308 + exponent;
    if (unlikely((digit_count >= 19))) { // this is uncommon
      // It is possible that the integer had an overflow.
      // We have to handle the case where we have 0.0000somenumber.
      const char *start = start_digits;
      while ((*start == '0') || (*start == '.')) {
        start++;
      }
      // we over-decrement by one when there is a '.'
      digit_count -= (start - start_digits);
      if (digit_count >= 19) {
        // Ok, chances are good that we had an overflow!
        // this is almost never going to get called!!!
        // we start anew, going slowly!!!
        return parse_float(buf, pj, offset, found_minus);
      }
    }
    if (unlikely((power_index > 2 * 308))) { // this is uncommon!!!
      // this is almost never going to get called!!!
      // we start anew, going slowly!!!
      return parse_float(buf, pj, offset, found_minus);
    }
    double factor = power_of_ten[power_index];
    factor = negative ? -factor : factor;
    double d = i * factor;
    pj.write_tape_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_float(d, buf + offset);
#endif
  } else {
    if (unlikely(digit_count >= 18)) { // this is uncommon!!!
      // there is a good chance that we had an overflow, so we need
      // need to recover: we parse the whole thing again.
      return parse_large_integer(buf, pj, offset, found_minus);
    }
    i = negative ? 0 - i : i;
    pj.write_tape_s64(i);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_integer(i, buf + offset);
#endif
  }
  return is_structural_or_whitespace(*p);
#endif // SIMDJSON_SKIPNUMBERPARSING
}

/* end file src/generic/numberparsing.h */


}// namespace simdjson::arm64


#endif // IS_ARM64
#endif //  SIMDJSON_ARM64_NUMBERPARSING_H
/* end file src/generic/numberparsing.h */

namespace simdjson::arm64 {

/* begin file src/generic/stage2_build_tape.h */
// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2_build_tape.h" (this simplifies amalgation)

namespace stage2 {

#ifdef SIMDJSON_USE_COMPUTED_GOTO
typedef void* ret_address;
#define INIT_ADDRESSES() { &&array_begin, &&array_continue, &&error, &&finish, &&object_begin, &&object_continue }
#define GOTO(address) { goto *(address); }
#define CONTINUE(address) { goto *(address); }
#else
typedef char ret_address;
#define INIT_ADDRESSES() { '[', 'a', 'e', 'f', '{', 'o' };
#define GOTO(address)                 \
  {                                   \
    switch(address) {                 \
      case '[': goto array_begin;     \
      case 'a': goto array_continue;  \
      case 'e': goto error;           \
      case 'f': goto finish;          \
      case '{': goto object_begin;    \
      case 'o': goto object_continue; \
    }                                 \
  }
// For the more constrained pop_scope() situation
#define CONTINUE(address)             \
  {                                   \
    switch(address) {                 \
      case 'a': goto array_continue;  \
      case 'o': goto object_continue; \
      case 'f': goto finish;          \
    }                                 \
  }
#endif

struct unified_machine_addresses {
  ret_address array_begin;
  ret_address array_continue;
  ret_address error;
  ret_address finish;
  ret_address object_begin;
  ret_address object_continue;
};

#undef FAIL_IF
#define FAIL_IF(EXPR) { if (EXPR) { return addresses.error; } }

// This is just so we can call parse_string() from parser.parse_string() without conflict.
WARN_UNUSED really_inline bool
really_parse_string(const uint8_t *buf, size_t len, ParsedJson &pj, uint32_t depth, uint32_t idx) {
  return parse_string(buf, len, pj, depth, idx);
}
WARN_UNUSED really_inline bool
really_parse_number(const uint8_t *const buf, ParsedJson &pj, const uint32_t offset, bool found_minus) {
  return parse_number(buf, pj, offset, found_minus);
}

struct structural_parser {
  const uint8_t* const buf;
  const size_t len;
  ParsedJson &pj;
  size_t i; // next structural index
  size_t idx; // location of the structural character in the input (buf)
  uint8_t c;    // used to track the (structural) character we are looking at
  uint32_t depth = 0; // could have an arbitrary starting depth

  really_inline structural_parser(const uint8_t *_buf, size_t _len, ParsedJson &_pj, uint32_t _i = 0) : buf{_buf}, len{_len}, pj{_pj}, i{_i} {}

  WARN_UNUSED really_inline int set_error_code(ErrorValues error_code) {
    pj.error_code = error_code;
    return error_code;
  }

  really_inline char advance_char() {
    idx = pj.structural_indexes[i++];
    c = buf[idx];
    return c;
  }

  template<typename F>
  really_inline bool with_space_terminated_copy(const F& f) {
    /**
    * We need to make a copy to make sure that the string is space terminated.
    * This is not about padding the input, which should already padded up
    * to len + SIMDJSON_PADDING. However, we have no control at this stage
    * on how the padding was done. What if the input string was padded with nulls?
    * It is quite common for an input string to have an extra null character (C string).
    * We do not want to allow 9\0 (where \0 is the null character) inside a JSON
    * document, but the string "9\0" by itself is fine. So we make a copy and
    * pad the input with spaces when we know that there is just one input element.
    * This copy is relatively expensive, but it will almost never be called in
    * practice unless you are in the strange scenario where you have many JSON
    * documents made of single atoms.
    */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      return true;
    }
    memcpy(copy, buf, len);
    memset(copy + len, ' ', SIMDJSON_PADDING);
    bool result = f(reinterpret_cast<const uint8_t*>(copy), idx);
    free(copy);
    return result;
  }

  WARN_UNUSED really_inline bool push_start_scope(ret_address continue_state, char type) {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.ret_address[depth] = continue_state;
    depth++;
    pj.write_tape(0, type);
    return depth >= pj.depth_capacity;
  }

  WARN_UNUSED really_inline bool push_start_scope(ret_address continue_state) {
    return push_start_scope(continue_state, c);
  }

  WARN_UNUSED really_inline bool push_scope(ret_address continue_state) {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); // Do this as early as possible
    pj.ret_address[depth] = continue_state;
    depth++;
    return depth >= pj.depth_capacity;
  }

  WARN_UNUSED really_inline ret_address pop_scope() {
    // write our tape location to the header scope
    depth--;
    pj.write_tape(pj.containing_scope_offset[depth], c);
    pj.annotate_previous_loc(pj.containing_scope_offset[depth], pj.get_current_loc());
    return pj.ret_address[depth];
  }
  really_inline void pop_root_scope() {
    // write our tape location to the header scope
    // The root scope gets written *at* the previous location.
    depth--;
    pj.annotate_previous_loc(pj.containing_scope_offset[depth], pj.get_current_loc());
    pj.write_tape(pj.containing_scope_offset[depth], 'r');
  }

  WARN_UNUSED really_inline bool parse_string() {
    return !really_parse_string(buf, len, pj, depth, idx);
  }

  WARN_UNUSED really_inline bool parse_number(const uint8_t *copy, uint32_t offset, bool found_minus) {
    return !really_parse_number(copy, pj, offset, found_minus);
  }
  WARN_UNUSED really_inline bool parse_number(bool found_minus) {
    return parse_number(buf, idx, found_minus);
  }

  WARN_UNUSED really_inline bool parse_atom(const uint8_t *copy, uint32_t offset) {
    switch (c) {
      case 't':
        if (!is_valid_true_atom(copy + offset)) { return true; };
        break;
      case 'f':
        if (!is_valid_false_atom(copy + offset)) { return true; }
        break;
      case 'n':
        if (!is_valid_null_atom(copy + offset)) { return true; }
        break;
      default:
        return false;
    }
    pj.write_tape(0, c);
    return false;
  }

  WARN_UNUSED really_inline bool parse_atom() {
    return parse_atom(buf, idx);
  }

  WARN_UNUSED really_inline ret_address parse_value(const unified_machine_addresses &addresses, ret_address continue_state) {
    switch (c) {
    case '"':
      FAIL_IF( parse_string() );
      return continue_state;
    case 't': case 'f': case 'n':
      FAIL_IF( parse_atom() );
      return continue_state;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      FAIL_IF( parse_number(false) );
      return continue_state;
    case '-':
      FAIL_IF( parse_number(true) );
      return continue_state;
    case '{':
      FAIL_IF( push_scope(continue_state) );
      return addresses.object_begin;
    case '[':
      FAIL_IF( push_scope(continue_state) );
      return addresses.array_begin;
    default:
      return addresses.error;
    }
  }

  WARN_UNUSED really_inline int finish() {
    // the string might not be NULL terminated.
    if ( i + 1 != pj.n_structural_indexes ) {
      return set_error_code(TAPE_ERROR);
    }
    pop_root_scope();
    if (depth != 0) {
      return set_error_code(TAPE_ERROR);
    }
    if (pj.containing_scope_offset[depth] != 0) {
      return set_error_code(TAPE_ERROR);
    }

    pj.valid = true;
    return set_error_code(SUCCESS);
  }

  WARN_UNUSED really_inline int error() {
    /* We do not need the next line because this is done by pj.init(),
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
      return set_error_code(DEPTH_ERROR);
    }
    switch (c) {
    case '"':
      return set_error_code(STRING_ERROR);
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
      return set_error_code(NUMBER_ERROR);
    case 't':
      return set_error_code(T_ATOM_ERROR);
    case 'n':
      return set_error_code(N_ATOM_ERROR);
    case 'f':
      return set_error_code(F_ATOM_ERROR);
    default:
      return set_error_code(TAPE_ERROR);
    }
  }

  WARN_UNUSED really_inline int start(ret_address finish_state) {
    pj.init(); // sets is_valid to false
    if (len > pj.byte_capacity) {
      return CAPACITY;
    }
    // Advance to the first character as soon as possible
    advance_char();
    // Push the root scope (there is always at least one scope)
    if (push_start_scope(finish_state, 'r')) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }
};

// Redefine FAIL_IF to use goto since it'll be used inside the function now
#undef FAIL_IF
#define FAIL_IF(EXPR) { if (EXPR) { goto error; } }

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
  static constexpr unified_machine_addresses addresses = INIT_ADDRESSES();
  structural_parser parser(buf, len, pj);
  int result = parser.start(addresses.finish);
  if (result) { return result; }

  //
  // Read first value
  //
  switch (parser.c) {
  case '{':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto object_begin;
  case '[':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto array_begin;
  case '"':
    FAIL_IF( parser.parse_string() );
    goto finish;
  case 't': case 'f': case 'n':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_atom(copy, idx);
      })
    );
    goto finish;
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, false);
      })
    );
    goto finish;
  case '-':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, true);
      })
    );
    goto finish;
  default:
    goto error;
  }

//
// Object parser states
//
object_begin:
  parser.advance_char();
  switch (parser.c) {
  case '"': {
    FAIL_IF( parser.parse_string() );
    goto object_key_state;
  }
  case '}':
    goto scope_end; // could also go to object_continue
  default:
    goto error;
  }

object_key_state:
  FAIL_IF( parser.advance_char() != ':' );
  parser.advance_char();
  GOTO( parser.parse_value(addresses, addresses.object_continue) );

object_continue:
  switch (parser.advance_char()) {
  case ',':
    FAIL_IF( parser.advance_char() != '"' );
    FAIL_IF( parser.parse_string() );
    goto object_key_state;
  case '}':
    goto scope_end;
  default:
    goto error;
  }

scope_end:
  CONTINUE( parser.pop_scope() );

//
// Array parser states
//
array_begin:
  if (parser.advance_char() == ']') {
    goto scope_end; // could also go to array_continue
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at parser.c on the
   * on paths that can accept a close square brace (post-, and at start) */
  GOTO( parser.parse_value(addresses, addresses.array_continue) );

array_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.advance_char();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto error;
  }

finish:
  return parser.finish();

error:
  return parser.error();
}

} // namespace stage2
/* end file src/generic/stage2_build_tape.h */
/* begin file src/generic/stage2_streaming_build_tape.h */
namespace stage2 {

struct streaming_structural_parser: structural_parser {
  really_inline streaming_structural_parser(const uint8_t *_buf, size_t _len, ParsedJson &_pj, size_t _i) : structural_parser(_buf, _len, _pj, _i) {}

  // override to add streaming
  WARN_UNUSED really_inline int start(ret_address finish_parser) {
    pj.init(); // sets is_valid to false
    // Capacity ain't no thang for streaming, so we don't check it.
    // Advance to the first character as soon as possible
    advance_char();
    // Push the root scope (there is always at least one scope)
    if (push_start_scope(finish_parser, 'r')) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }

  // override to add streaming
  WARN_UNUSED really_inline int finish() {
    if ( i + 1 > pj.n_structural_indexes ) {
      return set_error_code(TAPE_ERROR);
    }
    pop_root_scope();
    if (depth != 0) {
      return set_error_code(TAPE_ERROR);
    }
    if (pj.containing_scope_offset[depth] != 0) {
      return set_error_code(TAPE_ERROR);
    }
    bool finished = i + 1 == pj.n_structural_indexes;
    pj.valid = true;
    return set_error_code(finished ? SUCCESS : SUCCESS_AND_HAS_MORE);
  }
};

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
  static constexpr unified_machine_addresses addresses = INIT_ADDRESSES();
  streaming_structural_parser parser(buf, len, pj, next_json);
  int result = parser.start(addresses.finish);
  if (result) { return result; }
  //
  // Read first value
  //
  switch (parser.c) {
  case '{':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto object_begin;
  case '[':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto array_begin;
  case '"':
    FAIL_IF( parser.parse_string() );
    goto finish;
  case 't': case 'f': case 'n':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_atom(copy, idx);
      })
    );
    goto finish;
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, false);
      })
    );
    goto finish;
  case '-':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, true);
      })
    );
    goto finish;
  default:
    goto error;
  }

//
// Object parser parsers
//
object_begin:
  parser.advance_char();
  switch (parser.c) {
  case '"': {
    FAIL_IF( parser.parse_string() );
    goto object_key_parser;
  }
  case '}':
    goto scope_end; // could also go to object_continue
  default:
    goto error;
  }

object_key_parser:
  FAIL_IF( parser.advance_char() != ':' );
  parser.advance_char();
  GOTO( parser.parse_value(addresses, addresses.object_continue) );

object_continue:
  switch (parser.advance_char()) {
  case ',':
    FAIL_IF( parser.advance_char() != '"' );
    FAIL_IF( parser.parse_string() );
    goto object_key_parser;
  case '}':
    goto scope_end;
  default:
    goto error;
  }

scope_end:
  CONTINUE( parser.pop_scope() );

//
// Array parser parsers
//
array_begin:
  if (parser.advance_char() == ']') {
    goto scope_end; // could also go to array_continue
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at parser.c on the
   * on paths that can accept a close square brace (post-, and at start) */
  GOTO( parser.parse_value(addresses, addresses.array_continue) );

array_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.advance_char();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto error;
  }

finish:
  next_json = parser.i;
  return parser.finish();

error:
  return parser.error();
}

} // namespace stage2
/* end file src/generic/stage2_streaming_build_tape.h */

} // namespace simdjson::arm64

namespace simdjson {

template <>
WARN_UNUSED int
unified_machine<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  return arm64::stage2::unified_machine(buf, len, pj);
}

template <>
WARN_UNUSED int
unified_machine<Architecture::ARM64>(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    return arm64::stage2::unified_machine(buf, len, pj, next_json);
}

} // namespace simdjson

#endif // IS_ARM64

#endif // SIMDJSON_ARM64_STAGE2_BUILD_TAPE_H
/* end file src/generic/stage2_streaming_build_tape.h */
/* begin file src/haswell/stage2_build_tape.h */
#ifndef SIMDJSON_HASWELL_STAGE2_BUILD_TAPE_H
#define SIMDJSON_HASWELL_STAGE2_BUILD_TAPE_H


#ifdef IS_X86_64

/* begin file src/haswell/stringparsing.h */
#ifndef SIMDJSON_HASWELL_STRINGPARSING_H
#define SIMDJSON_HASWELL_STRINGPARSING_H


#ifdef IS_X86_64

/* haswell/simd.h already included: #include "haswell/simd.h" */
/* jsoncharutils.h already included: #include "jsoncharutils.h" */
/* haswell/intrinsics.h already included: #include "haswell/intrinsics.h" */
/* haswell/bitmanipulation.h already included: #include "haswell/bitmanipulation.h" */

TARGET_HASWELL
namespace simdjson::haswell {

using namespace simd;

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
  static const uint32_t BYTES_PROCESSED = 32;
};

really_inline parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src, uint8_t *dst) {
  // this can read up to 15 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (parse_string_helper::BYTES_PROCESSED - 1));
  simd8<uint8_t> v(src);
  // store to dest unconditionally - we can overwrite the bits we don't like later
  v.store(dst);
  return {
      (uint32_t)(v == '\\').to_bitmask(),     // bs_bits
      (uint32_t)(v == '"').to_bitmask(), // quote_bits
  };
}

/* begin file src/generic/stringparsing.h */
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
  pj.write_tape(pj.current_string_buf_loc - pj.string_buf.get(), '"');
  const uint8_t *src = &buf[offset + 1]; /* we know that buf at offset is a " */
  uint8_t *dst = pj.current_string_buf_loc + sizeof(uint32_t);
  const uint8_t *const start_of_string = dst;
  while (1) {
    parse_string_helper helper = find_bs_bits_and_quote_bits(src, dst);
    if (((helper.bs_bits - 1) & helper.quote_bits) != 0) {
      /* we encountered quotes first. Move dst to point to quotes and exit
       */

      /* find out where the quote is... */
      auto quote_dist = trailing_zeroes(helper.quote_bits);

      /* NULL termination is still handy if you expect all your strings to
       * be NULL terminated? */
      /* It comes at a small cost */
      dst[quote_dist] = 0;

      uint32_t str_length = (dst - start_of_string) + quote_dist;
      memcpy(pj.current_string_buf_loc, &str_length, sizeof(str_length));
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
      auto bs_dist = trailing_zeroes(helper.bs_bits);
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
      src += parse_string_helper::BYTES_PROCESSED;
      dst += parse_string_helper::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return true;
}
/* end file src/generic/stringparsing.h */

} // namespace simdjson::haswell
UNTARGET_REGION

#endif // IS_X86_64

#endif
/* end file src/generic/stringparsing.h */
/* begin file src/haswell/numberparsing.h */
#ifndef SIMDJSON_HASWELL_NUMBERPARSING_H
#define SIMDJSON_HASWELL_NUMBERPARSING_H

#ifdef IS_X86_64

/* haswell/intrinsics.h already included: #include "haswell/intrinsics.h" */
/* haswell/bitmanipulation.h already included: #include "haswell/bitmanipulation.h" */
/* jsoncharutils.h already included: #include "jsoncharutils.h" */
#include <cmath>
#include <limits>


#ifdef JSON_TEST_NUMBERS // for unit testing
void found_invalid_number(const uint8_t *buf);
void found_integer(int64_t result, const uint8_t *buf);
void found_unsigned_integer(uint64_t result, const uint8_t *buf);
void found_float(double result, const uint8_t *buf);
#endif

TARGET_HASWELL
namespace simdjson::haswell {
static inline uint32_t parse_eight_digits_unrolled(const char *chars) {
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

#define SWAR_NUMBER_PARSING

/* begin file src/generic/numberparsing.h */

// Allowable floating-point values range
// std::numeric_limits<double>::lowest() to std::numeric_limits<double>::max(),
// so from -1.7976e308 all the way to 1.7975e308 in binary64. The lowest
// non-zero normal values is std::numeric_limits<double>::min() or
// about 2.225074e-308.
static const double power_of_ten[] = {
    1e-308, 1e-307, 1e-306, 1e-305, 1e-304, 1e-303, 1e-302, 1e-301, 1e-300,
    1e-299, 1e-298, 1e-297, 1e-296, 1e-295, 1e-294, 1e-293, 1e-292, 1e-291,
    1e-290, 1e-289, 1e-288, 1e-287, 1e-286, 1e-285, 1e-284, 1e-283, 1e-282,
    1e-281, 1e-280, 1e-279, 1e-278, 1e-277, 1e-276, 1e-275, 1e-274, 1e-273,
    1e-272, 1e-271, 1e-270, 1e-269, 1e-268, 1e-267, 1e-266, 1e-265, 1e-264,
    1e-263, 1e-262, 1e-261, 1e-260, 1e-259, 1e-258, 1e-257, 1e-256, 1e-255,
    1e-254, 1e-253, 1e-252, 1e-251, 1e-250, 1e-249, 1e-248, 1e-247, 1e-246,
    1e-245, 1e-244, 1e-243, 1e-242, 1e-241, 1e-240, 1e-239, 1e-238, 1e-237,
    1e-236, 1e-235, 1e-234, 1e-233, 1e-232, 1e-231, 1e-230, 1e-229, 1e-228,
    1e-227, 1e-226, 1e-225, 1e-224, 1e-223, 1e-222, 1e-221, 1e-220, 1e-219,
    1e-218, 1e-217, 1e-216, 1e-215, 1e-214, 1e-213, 1e-212, 1e-211, 1e-210,
    1e-209, 1e-208, 1e-207, 1e-206, 1e-205, 1e-204, 1e-203, 1e-202, 1e-201,
    1e-200, 1e-199, 1e-198, 1e-197, 1e-196, 1e-195, 1e-194, 1e-193, 1e-192,
    1e-191, 1e-190, 1e-189, 1e-188, 1e-187, 1e-186, 1e-185, 1e-184, 1e-183,
    1e-182, 1e-181, 1e-180, 1e-179, 1e-178, 1e-177, 1e-176, 1e-175, 1e-174,
    1e-173, 1e-172, 1e-171, 1e-170, 1e-169, 1e-168, 1e-167, 1e-166, 1e-165,
    1e-164, 1e-163, 1e-162, 1e-161, 1e-160, 1e-159, 1e-158, 1e-157, 1e-156,
    1e-155, 1e-154, 1e-153, 1e-152, 1e-151, 1e-150, 1e-149, 1e-148, 1e-147,
    1e-146, 1e-145, 1e-144, 1e-143, 1e-142, 1e-141, 1e-140, 1e-139, 1e-138,
    1e-137, 1e-136, 1e-135, 1e-134, 1e-133, 1e-132, 1e-131, 1e-130, 1e-129,
    1e-128, 1e-127, 1e-126, 1e-125, 1e-124, 1e-123, 1e-122, 1e-121, 1e-120,
    1e-119, 1e-118, 1e-117, 1e-116, 1e-115, 1e-114, 1e-113, 1e-112, 1e-111,
    1e-110, 1e-109, 1e-108, 1e-107, 1e-106, 1e-105, 1e-104, 1e-103, 1e-102,
    1e-101, 1e-100, 1e-99,  1e-98,  1e-97,  1e-96,  1e-95,  1e-94,  1e-93,
    1e-92,  1e-91,  1e-90,  1e-89,  1e-88,  1e-87,  1e-86,  1e-85,  1e-84,
    1e-83,  1e-82,  1e-81,  1e-80,  1e-79,  1e-78,  1e-77,  1e-76,  1e-75,
    1e-74,  1e-73,  1e-72,  1e-71,  1e-70,  1e-69,  1e-68,  1e-67,  1e-66,
    1e-65,  1e-64,  1e-63,  1e-62,  1e-61,  1e-60,  1e-59,  1e-58,  1e-57,
    1e-56,  1e-55,  1e-54,  1e-53,  1e-52,  1e-51,  1e-50,  1e-49,  1e-48,
    1e-47,  1e-46,  1e-45,  1e-44,  1e-43,  1e-42,  1e-41,  1e-40,  1e-39,
    1e-38,  1e-37,  1e-36,  1e-35,  1e-34,  1e-33,  1e-32,  1e-31,  1e-30,
    1e-29,  1e-28,  1e-27,  1e-26,  1e-25,  1e-24,  1e-23,  1e-22,  1e-21,
    1e-20,  1e-19,  1e-18,  1e-17,  1e-16,  1e-15,  1e-14,  1e-13,  1e-12,
    1e-11,  1e-10,  1e-9,   1e-8,   1e-7,   1e-6,   1e-5,   1e-4,   1e-3,
    1e-2,   1e-1,   1e0,    1e1,    1e2,    1e3,    1e4,    1e5,    1e6,
    1e7,    1e8,    1e9,    1e10,   1e11,   1e12,   1e13,   1e14,   1e15,
    1e16,   1e17,   1e18,   1e19,   1e20,   1e21,   1e22,   1e23,   1e24,
    1e25,   1e26,   1e27,   1e28,   1e29,   1e30,   1e31,   1e32,   1e33,
    1e34,   1e35,   1e36,   1e37,   1e38,   1e39,   1e40,   1e41,   1e42,
    1e43,   1e44,   1e45,   1e46,   1e47,   1e48,   1e49,   1e50,   1e51,
    1e52,   1e53,   1e54,   1e55,   1e56,   1e57,   1e58,   1e59,   1e60,
    1e61,   1e62,   1e63,   1e64,   1e65,   1e66,   1e67,   1e68,   1e69,
    1e70,   1e71,   1e72,   1e73,   1e74,   1e75,   1e76,   1e77,   1e78,
    1e79,   1e80,   1e81,   1e82,   1e83,   1e84,   1e85,   1e86,   1e87,
    1e88,   1e89,   1e90,   1e91,   1e92,   1e93,   1e94,   1e95,   1e96,
    1e97,   1e98,   1e99,   1e100,  1e101,  1e102,  1e103,  1e104,  1e105,
    1e106,  1e107,  1e108,  1e109,  1e110,  1e111,  1e112,  1e113,  1e114,
    1e115,  1e116,  1e117,  1e118,  1e119,  1e120,  1e121,  1e122,  1e123,
    1e124,  1e125,  1e126,  1e127,  1e128,  1e129,  1e130,  1e131,  1e132,
    1e133,  1e134,  1e135,  1e136,  1e137,  1e138,  1e139,  1e140,  1e141,
    1e142,  1e143,  1e144,  1e145,  1e146,  1e147,  1e148,  1e149,  1e150,
    1e151,  1e152,  1e153,  1e154,  1e155,  1e156,  1e157,  1e158,  1e159,
    1e160,  1e161,  1e162,  1e163,  1e164,  1e165,  1e166,  1e167,  1e168,
    1e169,  1e170,  1e171,  1e172,  1e173,  1e174,  1e175,  1e176,  1e177,
    1e178,  1e179,  1e180,  1e181,  1e182,  1e183,  1e184,  1e185,  1e186,
    1e187,  1e188,  1e189,  1e190,  1e191,  1e192,  1e193,  1e194,  1e195,
    1e196,  1e197,  1e198,  1e199,  1e200,  1e201,  1e202,  1e203,  1e204,
    1e205,  1e206,  1e207,  1e208,  1e209,  1e210,  1e211,  1e212,  1e213,
    1e214,  1e215,  1e216,  1e217,  1e218,  1e219,  1e220,  1e221,  1e222,
    1e223,  1e224,  1e225,  1e226,  1e227,  1e228,  1e229,  1e230,  1e231,
    1e232,  1e233,  1e234,  1e235,  1e236,  1e237,  1e238,  1e239,  1e240,
    1e241,  1e242,  1e243,  1e244,  1e245,  1e246,  1e247,  1e248,  1e249,
    1e250,  1e251,  1e252,  1e253,  1e254,  1e255,  1e256,  1e257,  1e258,
    1e259,  1e260,  1e261,  1e262,  1e263,  1e264,  1e265,  1e266,  1e267,
    1e268,  1e269,  1e270,  1e271,  1e272,  1e273,  1e274,  1e275,  1e276,
    1e277,  1e278,  1e279,  1e280,  1e281,  1e282,  1e283,  1e284,  1e285,
    1e286,  1e287,  1e288,  1e289,  1e290,  1e291,  1e292,  1e293,  1e294,
    1e295,  1e296,  1e297,  1e298,  1e299,  1e300,  1e301,  1e302,  1e303,
    1e304,  1e305,  1e306,  1e307,  1e308};

static inline bool is_integer(char c) {
  return (c >= '0' && c <= '9');
  // this gets compiled to (uint8_t)(c - '0') <= 9 on all decent compilers
}

// We need to check that the character following a zero is valid. This is
// probably frequent and it is hard than it looks. We are building all of this
// just to differentiate between 0x1 (invalid), 0,1 (valid) 0e1 (valid)...
const bool structural_or_whitespace_or_exponent_or_decimal_negated[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

really_inline bool
is_not_structural_or_whitespace_or_exponent_or_decimal(unsigned char c) {
  return structural_or_whitespace_or_exponent_or_decimal_negated[c];
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
static inline bool is_made_of_eight_digits_fast(const char *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING);
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}


//
// This function computes base * 10 ^ (- negative_exponent ).
// It is only even going to be used when negative_exponent is tiny.
static double subnormal_power10(double base, int64_t negative_exponent) {
    // avoid integer overflows in the pow expression, those values would
    // become zero anyway.
    if(negative_exponent < -1000) {
        return 0;
    }

  // this is probably not going to be fast
  return base * 1e-308 * pow(10, negative_exponent + 308);
}

// called by parse_number when we know that the output is a float,
// but where there might be some integer overflow. The trick here is to
// parse using floats from the start.
// Do not call this function directly as it skips some of the checks from
// parse_number
//
// This function will almost never be called!!!
//
// Note: a redesign could avoid this function entirely.
//
static never_inline bool parse_float(const uint8_t *const buf, ParsedJson &pj,
                                     const uint32_t offset, bool found_minus) {
  const char *p = reinterpret_cast<const char *>(buf + offset);
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
  }
  long double i;
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    i = 0;
  } else {
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    while (is_integer(*p)) {
      digit = *p - '0';
      i = 10 * i + digit;
      ++p;
    }
  }
  if ('.' == *p) {
    ++p;
    int fractional_weight = 308;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;

      fractional_weight--;
      i = i + digit * (fractional_weight >= 0 ? power_of_ten[fractional_weight]
                                              : 0);
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      fractional_weight--;
      i = i + digit * (fractional_weight >= 0 ? power_of_ten[fractional_weight]
                                              : 0);
    }
  }
  if (('e' == *p) || ('E' == *p)) {
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    int64_t exp_number = digit; // exponential part
    p++;
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (is_integer(*p)) {
      if (exp_number > 0x100000000) { // we need to check for overflows
// we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (unlikely(exp_number > 308)) {
      // this path is unlikely
      if (neg_exp) {
        // We either have zero or a subnormal.
        // We expect this to be uncommon so we go through a slow path.
        i = subnormal_power10(i, -exp_number);
      } else {
// We know for sure that we have a number that is too large,
// we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
    } else {
      int exponent = (neg_exp ? -exp_number : exp_number);
      // we have that exp_number is [0,308] so that
      // exponent is [-308,308] so that
      // 308 + exponent is in [0, 2 * 308]
      i *= power_of_ten[308 + exponent];
    }
  }
  if (is_not_structural_or_whitespace(*p)) {
    return false;
  }
  // check that we can go from long double to double safely.
  if(i > std::numeric_limits<double>::max()) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
  }
  double d = negative ? -i : i;
  pj.write_tape_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
  found_float(d, buf + offset);
#endif
  return is_structural_or_whitespace(*p);
}

// called by parse_number when we know that the output is an integer,
// but where there might be some integer overflow.
// we want to catch overflows!
// Do not call this function directly as it skips some of the checks from
// parse_number
//
// This function will almost never be called!!!
//
static never_inline bool parse_large_integer(const uint8_t *const buf,
                                             ParsedJson &pj,
                                             const uint32_t offset,
                                             bool found_minus) {
  const char *p = reinterpret_cast<const char *>(buf + offset);

  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
  }
  uint64_t i;
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    i = 0;
  } else {
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      if (mul_overflow(i, 10, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false; // overflow
      }
      if (add_overflow(i, digit, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false; // overflow
      }
      ++p;
    }
  }
  if (negative) {
    if (i > 0x8000000000000000) {
       // overflows!
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false; // overflow
    } else if (i == 0x8000000000000000) {
      // In two's complement, we cannot represent 0x8000000000000000
      // as a positive signed integer, but the negative version is 
      // possible.
      constexpr int64_t signed_answer = INT64_MIN;
      pj.write_tape_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, buf + offset);
#endif
    } else {
      // we can negate safely
      int64_t signed_answer = -static_cast<int64_t>(i);
      pj.write_tape_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, buf + offset);
#endif
    }
  } else {
    // we have a positive integer, the contract is that
    // we try to represent it as a signed integer and only 
    // fallback on unsigned integers if absolutely necessary.
    if(i < 0x8000000000000000) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(i, buf + offset);
#endif
      pj.write_tape_s64(i);
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_unsigned_integer(i, buf + offset);
#endif
      pj.write_tape_u64(i);
    }
  }
  return is_structural_or_whitespace(*p);
}

// parse the number at buf + offset
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0 or 1) at high speed.
static really_inline bool parse_number(const uint8_t *const buf, ParsedJson &pj,
                                       const uint32_t offset,
                                       bool found_minus) {
#ifdef SIMDJSON_SKIPNUMBERPARSING // for performance analysis, it is sometimes
                                  // useful to skip parsing
  pj.write_tape_s64(0);           // always write zero
  return true;                    // always succeeds
#else
  const char *p = reinterpret_cast<const char *>(buf + offset);
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
    if (!is_integer(*p)) { // a negative sign must be followed by an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
  }
  const char *const start_digits = p;

  uint64_t i;      // an unsigned int avoids signed overflows (which are bad)
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    if (is_not_structural_or_whitespace_or_exponent_or_decimal(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    i = 0;
  } else {
    if (!(is_integer(*p))) { // must start with an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      // a multiplication by 10 is cheaper than an arbitrary integer
      // multiplication
      i = 10 * i + digit; // might overflow, we will handle the overflow later
      ++p;
    }
  }
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true; // At this point we know that we have a float
    // we continue with the fiction that we have an integer. If the
    // floating point number is representable as x * 10^z for some integer
    // z that fits in 53 bits, then we will be able to convert back the
    // the integer into a float in a lossless manner.
    ++p;
    const char *const first_after_period = p;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // might overflow + multiplication by 10 is likely
                          // cheaper than arbitrary mult.
      // we will handle the overflow later
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
#ifdef SWAR_NUMBER_PARSING
    // this helps if we have lots of decimals!
    // this turns out to be frequent enough.
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
    }
#endif
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
                          // because we have parse_highprecision_float later.
    }
    exponent = first_after_period - p;
  }
  int digit_count =
      p - start_digits - 1; // used later to guard against overflows
  int64_t exp_number = 0;   // exponential part
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    exp_number = digit;
    p++;
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (is_integer(*p)) {
      if (exp_number > 0x100000000) { // we need to check for overflows
                                      // we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    exponent += (neg_exp ? -exp_number : exp_number);
  }
  if (is_float) {
    uint64_t power_index = 308 + exponent;
    if (unlikely((digit_count >= 19))) { // this is uncommon
      // It is possible that the integer had an overflow.
      // We have to handle the case where we have 0.0000somenumber.
      const char *start = start_digits;
      while ((*start == '0') || (*start == '.')) {
        start++;
      }
      // we over-decrement by one when there is a '.'
      digit_count -= (start - start_digits);
      if (digit_count >= 19) {
        // Ok, chances are good that we had an overflow!
        // this is almost never going to get called!!!
        // we start anew, going slowly!!!
        return parse_float(buf, pj, offset, found_minus);
      }
    }
    if (unlikely((power_index > 2 * 308))) { // this is uncommon!!!
      // this is almost never going to get called!!!
      // we start anew, going slowly!!!
      return parse_float(buf, pj, offset, found_minus);
    }
    double factor = power_of_ten[power_index];
    factor = negative ? -factor : factor;
    double d = i * factor;
    pj.write_tape_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_float(d, buf + offset);
#endif
  } else {
    if (unlikely(digit_count >= 18)) { // this is uncommon!!!
      // there is a good chance that we had an overflow, so we need
      // need to recover: we parse the whole thing again.
      return parse_large_integer(buf, pj, offset, found_minus);
    }
    i = negative ? 0 - i : i;
    pj.write_tape_s64(i);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_integer(i, buf + offset);
#endif
  }
  return is_structural_or_whitespace(*p);
#endif // SIMDJSON_SKIPNUMBERPARSING
}

/* end file src/generic/numberparsing.h */

} // namespace simdjson::haswell
UNTARGET_REGION




#endif // IS_X86_64


#endif //  SIMDJSON_HASWELL_NUMBERPARSING_H
/* end file src/generic/numberparsing.h */

TARGET_HASWELL
namespace simdjson::haswell {

/* begin file src/generic/stage2_build_tape.h */
// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2_build_tape.h" (this simplifies amalgation)

namespace stage2 {

#ifdef SIMDJSON_USE_COMPUTED_GOTO
typedef void* ret_address;
#define INIT_ADDRESSES() { &&array_begin, &&array_continue, &&error, &&finish, &&object_begin, &&object_continue }
#define GOTO(address) { goto *(address); }
#define CONTINUE(address) { goto *(address); }
#else
typedef char ret_address;
#define INIT_ADDRESSES() { '[', 'a', 'e', 'f', '{', 'o' };
#define GOTO(address)                 \
  {                                   \
    switch(address) {                 \
      case '[': goto array_begin;     \
      case 'a': goto array_continue;  \
      case 'e': goto error;           \
      case 'f': goto finish;          \
      case '{': goto object_begin;    \
      case 'o': goto object_continue; \
    }                                 \
  }
// For the more constrained pop_scope() situation
#define CONTINUE(address)             \
  {                                   \
    switch(address) {                 \
      case 'a': goto array_continue;  \
      case 'o': goto object_continue; \
      case 'f': goto finish;          \
    }                                 \
  }
#endif

struct unified_machine_addresses {
  ret_address array_begin;
  ret_address array_continue;
  ret_address error;
  ret_address finish;
  ret_address object_begin;
  ret_address object_continue;
};

#undef FAIL_IF
#define FAIL_IF(EXPR) { if (EXPR) { return addresses.error; } }

// This is just so we can call parse_string() from parser.parse_string() without conflict.
WARN_UNUSED really_inline bool
really_parse_string(const uint8_t *buf, size_t len, ParsedJson &pj, uint32_t depth, uint32_t idx) {
  return parse_string(buf, len, pj, depth, idx);
}
WARN_UNUSED really_inline bool
really_parse_number(const uint8_t *const buf, ParsedJson &pj, const uint32_t offset, bool found_minus) {
  return parse_number(buf, pj, offset, found_minus);
}

struct structural_parser {
  const uint8_t* const buf;
  const size_t len;
  ParsedJson &pj;
  size_t i; // next structural index
  size_t idx; // location of the structural character in the input (buf)
  uint8_t c;    // used to track the (structural) character we are looking at
  uint32_t depth = 0; // could have an arbitrary starting depth

  really_inline structural_parser(const uint8_t *_buf, size_t _len, ParsedJson &_pj, uint32_t _i = 0) : buf{_buf}, len{_len}, pj{_pj}, i{_i} {}

  WARN_UNUSED really_inline int set_error_code(ErrorValues error_code) {
    pj.error_code = error_code;
    return error_code;
  }

  really_inline char advance_char() {
    idx = pj.structural_indexes[i++];
    c = buf[idx];
    return c;
  }

  template<typename F>
  really_inline bool with_space_terminated_copy(const F& f) {
    /**
    * We need to make a copy to make sure that the string is space terminated.
    * This is not about padding the input, which should already padded up
    * to len + SIMDJSON_PADDING. However, we have no control at this stage
    * on how the padding was done. What if the input string was padded with nulls?
    * It is quite common for an input string to have an extra null character (C string).
    * We do not want to allow 9\0 (where \0 is the null character) inside a JSON
    * document, but the string "9\0" by itself is fine. So we make a copy and
    * pad the input with spaces when we know that there is just one input element.
    * This copy is relatively expensive, but it will almost never be called in
    * practice unless you are in the strange scenario where you have many JSON
    * documents made of single atoms.
    */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      return true;
    }
    memcpy(copy, buf, len);
    memset(copy + len, ' ', SIMDJSON_PADDING);
    bool result = f(reinterpret_cast<const uint8_t*>(copy), idx);
    free(copy);
    return result;
  }

  WARN_UNUSED really_inline bool push_start_scope(ret_address continue_state, char type) {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.ret_address[depth] = continue_state;
    depth++;
    pj.write_tape(0, type);
    return depth >= pj.depth_capacity;
  }

  WARN_UNUSED really_inline bool push_start_scope(ret_address continue_state) {
    return push_start_scope(continue_state, c);
  }

  WARN_UNUSED really_inline bool push_scope(ret_address continue_state) {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); // Do this as early as possible
    pj.ret_address[depth] = continue_state;
    depth++;
    return depth >= pj.depth_capacity;
  }

  WARN_UNUSED really_inline ret_address pop_scope() {
    // write our tape location to the header scope
    depth--;
    pj.write_tape(pj.containing_scope_offset[depth], c);
    pj.annotate_previous_loc(pj.containing_scope_offset[depth], pj.get_current_loc());
    return pj.ret_address[depth];
  }
  really_inline void pop_root_scope() {
    // write our tape location to the header scope
    // The root scope gets written *at* the previous location.
    depth--;
    pj.annotate_previous_loc(pj.containing_scope_offset[depth], pj.get_current_loc());
    pj.write_tape(pj.containing_scope_offset[depth], 'r');
  }

  WARN_UNUSED really_inline bool parse_string() {
    return !really_parse_string(buf, len, pj, depth, idx);
  }

  WARN_UNUSED really_inline bool parse_number(const uint8_t *copy, uint32_t offset, bool found_minus) {
    return !really_parse_number(copy, pj, offset, found_minus);
  }
  WARN_UNUSED really_inline bool parse_number(bool found_minus) {
    return parse_number(buf, idx, found_minus);
  }

  WARN_UNUSED really_inline bool parse_atom(const uint8_t *copy, uint32_t offset) {
    switch (c) {
      case 't':
        if (!is_valid_true_atom(copy + offset)) { return true; };
        break;
      case 'f':
        if (!is_valid_false_atom(copy + offset)) { return true; }
        break;
      case 'n':
        if (!is_valid_null_atom(copy + offset)) { return true; }
        break;
      default:
        return false;
    }
    pj.write_tape(0, c);
    return false;
  }

  WARN_UNUSED really_inline bool parse_atom() {
    return parse_atom(buf, idx);
  }

  WARN_UNUSED really_inline ret_address parse_value(const unified_machine_addresses &addresses, ret_address continue_state) {
    switch (c) {
    case '"':
      FAIL_IF( parse_string() );
      return continue_state;
    case 't': case 'f': case 'n':
      FAIL_IF( parse_atom() );
      return continue_state;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      FAIL_IF( parse_number(false) );
      return continue_state;
    case '-':
      FAIL_IF( parse_number(true) );
      return continue_state;
    case '{':
      FAIL_IF( push_scope(continue_state) );
      return addresses.object_begin;
    case '[':
      FAIL_IF( push_scope(continue_state) );
      return addresses.array_begin;
    default:
      return addresses.error;
    }
  }

  WARN_UNUSED really_inline int finish() {
    // the string might not be NULL terminated.
    if ( i + 1 != pj.n_structural_indexes ) {
      return set_error_code(TAPE_ERROR);
    }
    pop_root_scope();
    if (depth != 0) {
      return set_error_code(TAPE_ERROR);
    }
    if (pj.containing_scope_offset[depth] != 0) {
      return set_error_code(TAPE_ERROR);
    }

    pj.valid = true;
    return set_error_code(SUCCESS);
  }

  WARN_UNUSED really_inline int error() {
    /* We do not need the next line because this is done by pj.init(),
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
      return set_error_code(DEPTH_ERROR);
    }
    switch (c) {
    case '"':
      return set_error_code(STRING_ERROR);
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
      return set_error_code(NUMBER_ERROR);
    case 't':
      return set_error_code(T_ATOM_ERROR);
    case 'n':
      return set_error_code(N_ATOM_ERROR);
    case 'f':
      return set_error_code(F_ATOM_ERROR);
    default:
      return set_error_code(TAPE_ERROR);
    }
  }

  WARN_UNUSED really_inline int start(ret_address finish_state) {
    pj.init(); // sets is_valid to false
    if (len > pj.byte_capacity) {
      return CAPACITY;
    }
    // Advance to the first character as soon as possible
    advance_char();
    // Push the root scope (there is always at least one scope)
    if (push_start_scope(finish_state, 'r')) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }
};

// Redefine FAIL_IF to use goto since it'll be used inside the function now
#undef FAIL_IF
#define FAIL_IF(EXPR) { if (EXPR) { goto error; } }

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
  static constexpr unified_machine_addresses addresses = INIT_ADDRESSES();
  structural_parser parser(buf, len, pj);
  int result = parser.start(addresses.finish);
  if (result) { return result; }

  //
  // Read first value
  //
  switch (parser.c) {
  case '{':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto object_begin;
  case '[':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto array_begin;
  case '"':
    FAIL_IF( parser.parse_string() );
    goto finish;
  case 't': case 'f': case 'n':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_atom(copy, idx);
      })
    );
    goto finish;
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, false);
      })
    );
    goto finish;
  case '-':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, true);
      })
    );
    goto finish;
  default:
    goto error;
  }

//
// Object parser states
//
object_begin:
  parser.advance_char();
  switch (parser.c) {
  case '"': {
    FAIL_IF( parser.parse_string() );
    goto object_key_state;
  }
  case '}':
    goto scope_end; // could also go to object_continue
  default:
    goto error;
  }

object_key_state:
  FAIL_IF( parser.advance_char() != ':' );
  parser.advance_char();
  GOTO( parser.parse_value(addresses, addresses.object_continue) );

object_continue:
  switch (parser.advance_char()) {
  case ',':
    FAIL_IF( parser.advance_char() != '"' );
    FAIL_IF( parser.parse_string() );
    goto object_key_state;
  case '}':
    goto scope_end;
  default:
    goto error;
  }

scope_end:
  CONTINUE( parser.pop_scope() );

//
// Array parser states
//
array_begin:
  if (parser.advance_char() == ']') {
    goto scope_end; // could also go to array_continue
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at parser.c on the
   * on paths that can accept a close square brace (post-, and at start) */
  GOTO( parser.parse_value(addresses, addresses.array_continue) );

array_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.advance_char();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto error;
  }

finish:
  return parser.finish();

error:
  return parser.error();
}

} // namespace stage2
/* end file src/generic/stage2_build_tape.h */
/* begin file src/generic/stage2_streaming_build_tape.h */
namespace stage2 {

struct streaming_structural_parser: structural_parser {
  really_inline streaming_structural_parser(const uint8_t *_buf, size_t _len, ParsedJson &_pj, size_t _i) : structural_parser(_buf, _len, _pj, _i) {}

  // override to add streaming
  WARN_UNUSED really_inline int start(ret_address finish_parser) {
    pj.init(); // sets is_valid to false
    // Capacity ain't no thang for streaming, so we don't check it.
    // Advance to the first character as soon as possible
    advance_char();
    // Push the root scope (there is always at least one scope)
    if (push_start_scope(finish_parser, 'r')) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }

  // override to add streaming
  WARN_UNUSED really_inline int finish() {
    if ( i + 1 > pj.n_structural_indexes ) {
      return set_error_code(TAPE_ERROR);
    }
    pop_root_scope();
    if (depth != 0) {
      return set_error_code(TAPE_ERROR);
    }
    if (pj.containing_scope_offset[depth] != 0) {
      return set_error_code(TAPE_ERROR);
    }
    bool finished = i + 1 == pj.n_structural_indexes;
    pj.valid = true;
    return set_error_code(finished ? SUCCESS : SUCCESS_AND_HAS_MORE);
  }
};

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
  static constexpr unified_machine_addresses addresses = INIT_ADDRESSES();
  streaming_structural_parser parser(buf, len, pj, next_json);
  int result = parser.start(addresses.finish);
  if (result) { return result; }
  //
  // Read first value
  //
  switch (parser.c) {
  case '{':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto object_begin;
  case '[':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto array_begin;
  case '"':
    FAIL_IF( parser.parse_string() );
    goto finish;
  case 't': case 'f': case 'n':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_atom(copy, idx);
      })
    );
    goto finish;
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, false);
      })
    );
    goto finish;
  case '-':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, true);
      })
    );
    goto finish;
  default:
    goto error;
  }

//
// Object parser parsers
//
object_begin:
  parser.advance_char();
  switch (parser.c) {
  case '"': {
    FAIL_IF( parser.parse_string() );
    goto object_key_parser;
  }
  case '}':
    goto scope_end; // could also go to object_continue
  default:
    goto error;
  }

object_key_parser:
  FAIL_IF( parser.advance_char() != ':' );
  parser.advance_char();
  GOTO( parser.parse_value(addresses, addresses.object_continue) );

object_continue:
  switch (parser.advance_char()) {
  case ',':
    FAIL_IF( parser.advance_char() != '"' );
    FAIL_IF( parser.parse_string() );
    goto object_key_parser;
  case '}':
    goto scope_end;
  default:
    goto error;
  }

scope_end:
  CONTINUE( parser.pop_scope() );

//
// Array parser parsers
//
array_begin:
  if (parser.advance_char() == ']') {
    goto scope_end; // could also go to array_continue
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at parser.c on the
   * on paths that can accept a close square brace (post-, and at start) */
  GOTO( parser.parse_value(addresses, addresses.array_continue) );

array_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.advance_char();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto error;
  }

finish:
  next_json = parser.i;
  return parser.finish();

error:
  return parser.error();
}

} // namespace stage2
/* end file src/generic/stage2_streaming_build_tape.h */

} // namespace simdjson::haswell
UNTARGET_REGION

TARGET_HASWELL
namespace simdjson {

template <>
WARN_UNUSED int
unified_machine<Architecture::HASWELL>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  return haswell::stage2::unified_machine(buf, len, pj);
}

template <>
WARN_UNUSED int
unified_machine<Architecture::HASWELL>(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
  return haswell::stage2::unified_machine(buf, len, pj, next_json);
}

} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64

#endif // SIMDJSON_HASWELL_STAGE2_BUILD_TAPE_H
/* end file src/generic/stage2_streaming_build_tape.h */
/* begin file src/westmere/stage2_build_tape.h */
#ifndef SIMDJSON_WESTMERE_STAGE2_BUILD_TAPE_H
#define SIMDJSON_WESTMERE_STAGE2_BUILD_TAPE_H


#ifdef IS_X86_64

/* begin file src/westmere/stringparsing.h */
#ifndef SIMDJSON_WESTMERE_STRINGPARSING_H
#define SIMDJSON_WESTMERE_STRINGPARSING_H


#ifdef IS_X86_64

/* westmere/simd.h already included: #include "westmere/simd.h" */
/* jsoncharutils.h already included: #include "jsoncharutils.h" */
/* westmere/intrinsics.h already included: #include "westmere/intrinsics.h" */
/* westmere/bitmanipulation.h already included: #include "westmere/bitmanipulation.h" */

TARGET_WESTMERE
namespace simdjson::westmere {

using namespace simd;

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
  static const uint32_t BYTES_PROCESSED = 32;
};

really_inline parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (parse_string_helper::BYTES_PROCESSED - 1));
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + 16);
  v0.store(dst);
  v1.store(dst + 16);
  uint64_t bs_and_quote = simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
    static_cast<uint32_t>(bs_and_quote),      // bs_bits
    static_cast<uint32_t>(bs_and_quote >> 32) // quote_bits
  };
}

/* begin file src/generic/stringparsing.h */
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
  pj.write_tape(pj.current_string_buf_loc - pj.string_buf.get(), '"');
  const uint8_t *src = &buf[offset + 1]; /* we know that buf at offset is a " */
  uint8_t *dst = pj.current_string_buf_loc + sizeof(uint32_t);
  const uint8_t *const start_of_string = dst;
  while (1) {
    parse_string_helper helper = find_bs_bits_and_quote_bits(src, dst);
    if (((helper.bs_bits - 1) & helper.quote_bits) != 0) {
      /* we encountered quotes first. Move dst to point to quotes and exit
       */

      /* find out where the quote is... */
      auto quote_dist = trailing_zeroes(helper.quote_bits);

      /* NULL termination is still handy if you expect all your strings to
       * be NULL terminated? */
      /* It comes at a small cost */
      dst[quote_dist] = 0;

      uint32_t str_length = (dst - start_of_string) + quote_dist;
      memcpy(pj.current_string_buf_loc, &str_length, sizeof(str_length));
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
      auto bs_dist = trailing_zeroes(helper.bs_bits);
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
      src += parse_string_helper::BYTES_PROCESSED;
      dst += parse_string_helper::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return true;
}
/* end file src/generic/stringparsing.h */

} // namespace simdjson::westmere
UNTARGET_REGION

#endif // IS_X86_64

#endif
/* end file src/generic/stringparsing.h */
/* begin file src/westmere/numberparsing.h */
#ifndef SIMDJSON_WESTMERE_NUMBERPARSING_H
#define SIMDJSON_WESTMERE_NUMBERPARSING_H

#ifdef IS_X86_64

/* westmere/intrinsics.h already included: #include "westmere/intrinsics.h" */
/* westmere/intrinsics.h already included: #include "westmere/intrinsics.h" */
/* westmere/bitmanipulation.h already included: #include "westmere/bitmanipulation.h" */
/* jsoncharutils.h already included: #include "jsoncharutils.h" */
#include <cmath>
#include <limits>


#ifdef JSON_TEST_NUMBERS // for unit testing
void found_invalid_number(const uint8_t *buf);
void found_integer(int64_t result, const uint8_t *buf);
void found_unsigned_integer(uint64_t result, const uint8_t *buf);
void found_float(double result, const uint8_t *buf);
#endif


TARGET_WESTMERE
namespace simdjson::westmere {
static inline uint32_t parse_eight_digits_unrolled(const char *chars) {
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

#define SWAR_NUMBER_PARSING

/* begin file src/generic/numberparsing.h */

// Allowable floating-point values range
// std::numeric_limits<double>::lowest() to std::numeric_limits<double>::max(),
// so from -1.7976e308 all the way to 1.7975e308 in binary64. The lowest
// non-zero normal values is std::numeric_limits<double>::min() or
// about 2.225074e-308.
static const double power_of_ten[] = {
    1e-308, 1e-307, 1e-306, 1e-305, 1e-304, 1e-303, 1e-302, 1e-301, 1e-300,
    1e-299, 1e-298, 1e-297, 1e-296, 1e-295, 1e-294, 1e-293, 1e-292, 1e-291,
    1e-290, 1e-289, 1e-288, 1e-287, 1e-286, 1e-285, 1e-284, 1e-283, 1e-282,
    1e-281, 1e-280, 1e-279, 1e-278, 1e-277, 1e-276, 1e-275, 1e-274, 1e-273,
    1e-272, 1e-271, 1e-270, 1e-269, 1e-268, 1e-267, 1e-266, 1e-265, 1e-264,
    1e-263, 1e-262, 1e-261, 1e-260, 1e-259, 1e-258, 1e-257, 1e-256, 1e-255,
    1e-254, 1e-253, 1e-252, 1e-251, 1e-250, 1e-249, 1e-248, 1e-247, 1e-246,
    1e-245, 1e-244, 1e-243, 1e-242, 1e-241, 1e-240, 1e-239, 1e-238, 1e-237,
    1e-236, 1e-235, 1e-234, 1e-233, 1e-232, 1e-231, 1e-230, 1e-229, 1e-228,
    1e-227, 1e-226, 1e-225, 1e-224, 1e-223, 1e-222, 1e-221, 1e-220, 1e-219,
    1e-218, 1e-217, 1e-216, 1e-215, 1e-214, 1e-213, 1e-212, 1e-211, 1e-210,
    1e-209, 1e-208, 1e-207, 1e-206, 1e-205, 1e-204, 1e-203, 1e-202, 1e-201,
    1e-200, 1e-199, 1e-198, 1e-197, 1e-196, 1e-195, 1e-194, 1e-193, 1e-192,
    1e-191, 1e-190, 1e-189, 1e-188, 1e-187, 1e-186, 1e-185, 1e-184, 1e-183,
    1e-182, 1e-181, 1e-180, 1e-179, 1e-178, 1e-177, 1e-176, 1e-175, 1e-174,
    1e-173, 1e-172, 1e-171, 1e-170, 1e-169, 1e-168, 1e-167, 1e-166, 1e-165,
    1e-164, 1e-163, 1e-162, 1e-161, 1e-160, 1e-159, 1e-158, 1e-157, 1e-156,
    1e-155, 1e-154, 1e-153, 1e-152, 1e-151, 1e-150, 1e-149, 1e-148, 1e-147,
    1e-146, 1e-145, 1e-144, 1e-143, 1e-142, 1e-141, 1e-140, 1e-139, 1e-138,
    1e-137, 1e-136, 1e-135, 1e-134, 1e-133, 1e-132, 1e-131, 1e-130, 1e-129,
    1e-128, 1e-127, 1e-126, 1e-125, 1e-124, 1e-123, 1e-122, 1e-121, 1e-120,
    1e-119, 1e-118, 1e-117, 1e-116, 1e-115, 1e-114, 1e-113, 1e-112, 1e-111,
    1e-110, 1e-109, 1e-108, 1e-107, 1e-106, 1e-105, 1e-104, 1e-103, 1e-102,
    1e-101, 1e-100, 1e-99,  1e-98,  1e-97,  1e-96,  1e-95,  1e-94,  1e-93,
    1e-92,  1e-91,  1e-90,  1e-89,  1e-88,  1e-87,  1e-86,  1e-85,  1e-84,
    1e-83,  1e-82,  1e-81,  1e-80,  1e-79,  1e-78,  1e-77,  1e-76,  1e-75,
    1e-74,  1e-73,  1e-72,  1e-71,  1e-70,  1e-69,  1e-68,  1e-67,  1e-66,
    1e-65,  1e-64,  1e-63,  1e-62,  1e-61,  1e-60,  1e-59,  1e-58,  1e-57,
    1e-56,  1e-55,  1e-54,  1e-53,  1e-52,  1e-51,  1e-50,  1e-49,  1e-48,
    1e-47,  1e-46,  1e-45,  1e-44,  1e-43,  1e-42,  1e-41,  1e-40,  1e-39,
    1e-38,  1e-37,  1e-36,  1e-35,  1e-34,  1e-33,  1e-32,  1e-31,  1e-30,
    1e-29,  1e-28,  1e-27,  1e-26,  1e-25,  1e-24,  1e-23,  1e-22,  1e-21,
    1e-20,  1e-19,  1e-18,  1e-17,  1e-16,  1e-15,  1e-14,  1e-13,  1e-12,
    1e-11,  1e-10,  1e-9,   1e-8,   1e-7,   1e-6,   1e-5,   1e-4,   1e-3,
    1e-2,   1e-1,   1e0,    1e1,    1e2,    1e3,    1e4,    1e5,    1e6,
    1e7,    1e8,    1e9,    1e10,   1e11,   1e12,   1e13,   1e14,   1e15,
    1e16,   1e17,   1e18,   1e19,   1e20,   1e21,   1e22,   1e23,   1e24,
    1e25,   1e26,   1e27,   1e28,   1e29,   1e30,   1e31,   1e32,   1e33,
    1e34,   1e35,   1e36,   1e37,   1e38,   1e39,   1e40,   1e41,   1e42,
    1e43,   1e44,   1e45,   1e46,   1e47,   1e48,   1e49,   1e50,   1e51,
    1e52,   1e53,   1e54,   1e55,   1e56,   1e57,   1e58,   1e59,   1e60,
    1e61,   1e62,   1e63,   1e64,   1e65,   1e66,   1e67,   1e68,   1e69,
    1e70,   1e71,   1e72,   1e73,   1e74,   1e75,   1e76,   1e77,   1e78,
    1e79,   1e80,   1e81,   1e82,   1e83,   1e84,   1e85,   1e86,   1e87,
    1e88,   1e89,   1e90,   1e91,   1e92,   1e93,   1e94,   1e95,   1e96,
    1e97,   1e98,   1e99,   1e100,  1e101,  1e102,  1e103,  1e104,  1e105,
    1e106,  1e107,  1e108,  1e109,  1e110,  1e111,  1e112,  1e113,  1e114,
    1e115,  1e116,  1e117,  1e118,  1e119,  1e120,  1e121,  1e122,  1e123,
    1e124,  1e125,  1e126,  1e127,  1e128,  1e129,  1e130,  1e131,  1e132,
    1e133,  1e134,  1e135,  1e136,  1e137,  1e138,  1e139,  1e140,  1e141,
    1e142,  1e143,  1e144,  1e145,  1e146,  1e147,  1e148,  1e149,  1e150,
    1e151,  1e152,  1e153,  1e154,  1e155,  1e156,  1e157,  1e158,  1e159,
    1e160,  1e161,  1e162,  1e163,  1e164,  1e165,  1e166,  1e167,  1e168,
    1e169,  1e170,  1e171,  1e172,  1e173,  1e174,  1e175,  1e176,  1e177,
    1e178,  1e179,  1e180,  1e181,  1e182,  1e183,  1e184,  1e185,  1e186,
    1e187,  1e188,  1e189,  1e190,  1e191,  1e192,  1e193,  1e194,  1e195,
    1e196,  1e197,  1e198,  1e199,  1e200,  1e201,  1e202,  1e203,  1e204,
    1e205,  1e206,  1e207,  1e208,  1e209,  1e210,  1e211,  1e212,  1e213,
    1e214,  1e215,  1e216,  1e217,  1e218,  1e219,  1e220,  1e221,  1e222,
    1e223,  1e224,  1e225,  1e226,  1e227,  1e228,  1e229,  1e230,  1e231,
    1e232,  1e233,  1e234,  1e235,  1e236,  1e237,  1e238,  1e239,  1e240,
    1e241,  1e242,  1e243,  1e244,  1e245,  1e246,  1e247,  1e248,  1e249,
    1e250,  1e251,  1e252,  1e253,  1e254,  1e255,  1e256,  1e257,  1e258,
    1e259,  1e260,  1e261,  1e262,  1e263,  1e264,  1e265,  1e266,  1e267,
    1e268,  1e269,  1e270,  1e271,  1e272,  1e273,  1e274,  1e275,  1e276,
    1e277,  1e278,  1e279,  1e280,  1e281,  1e282,  1e283,  1e284,  1e285,
    1e286,  1e287,  1e288,  1e289,  1e290,  1e291,  1e292,  1e293,  1e294,
    1e295,  1e296,  1e297,  1e298,  1e299,  1e300,  1e301,  1e302,  1e303,
    1e304,  1e305,  1e306,  1e307,  1e308};

static inline bool is_integer(char c) {
  return (c >= '0' && c <= '9');
  // this gets compiled to (uint8_t)(c - '0') <= 9 on all decent compilers
}

// We need to check that the character following a zero is valid. This is
// probably frequent and it is hard than it looks. We are building all of this
// just to differentiate between 0x1 (invalid), 0,1 (valid) 0e1 (valid)...
const bool structural_or_whitespace_or_exponent_or_decimal_negated[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

really_inline bool
is_not_structural_or_whitespace_or_exponent_or_decimal(unsigned char c) {
  return structural_or_whitespace_or_exponent_or_decimal_negated[c];
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
static inline bool is_made_of_eight_digits_fast(const char *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING);
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}


//
// This function computes base * 10 ^ (- negative_exponent ).
// It is only even going to be used when negative_exponent is tiny.
static double subnormal_power10(double base, int64_t negative_exponent) {
    // avoid integer overflows in the pow expression, those values would
    // become zero anyway.
    if(negative_exponent < -1000) {
        return 0;
    }

  // this is probably not going to be fast
  return base * 1e-308 * pow(10, negative_exponent + 308);
}

// called by parse_number when we know that the output is a float,
// but where there might be some integer overflow. The trick here is to
// parse using floats from the start.
// Do not call this function directly as it skips some of the checks from
// parse_number
//
// This function will almost never be called!!!
//
// Note: a redesign could avoid this function entirely.
//
static never_inline bool parse_float(const uint8_t *const buf, ParsedJson &pj,
                                     const uint32_t offset, bool found_minus) {
  const char *p = reinterpret_cast<const char *>(buf + offset);
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
  }
  long double i;
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    i = 0;
  } else {
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    while (is_integer(*p)) {
      digit = *p - '0';
      i = 10 * i + digit;
      ++p;
    }
  }
  if ('.' == *p) {
    ++p;
    int fractional_weight = 308;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;

      fractional_weight--;
      i = i + digit * (fractional_weight >= 0 ? power_of_ten[fractional_weight]
                                              : 0);
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      fractional_weight--;
      i = i + digit * (fractional_weight >= 0 ? power_of_ten[fractional_weight]
                                              : 0);
    }
  }
  if (('e' == *p) || ('E' == *p)) {
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    int64_t exp_number = digit; // exponential part
    p++;
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (is_integer(*p)) {
      if (exp_number > 0x100000000) { // we need to check for overflows
// we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (unlikely(exp_number > 308)) {
      // this path is unlikely
      if (neg_exp) {
        // We either have zero or a subnormal.
        // We expect this to be uncommon so we go through a slow path.
        i = subnormal_power10(i, -exp_number);
      } else {
// We know for sure that we have a number that is too large,
// we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
    } else {
      int exponent = (neg_exp ? -exp_number : exp_number);
      // we have that exp_number is [0,308] so that
      // exponent is [-308,308] so that
      // 308 + exponent is in [0, 2 * 308]
      i *= power_of_ten[308 + exponent];
    }
  }
  if (is_not_structural_or_whitespace(*p)) {
    return false;
  }
  // check that we can go from long double to double safely.
  if(i > std::numeric_limits<double>::max()) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
  }
  double d = negative ? -i : i;
  pj.write_tape_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
  found_float(d, buf + offset);
#endif
  return is_structural_or_whitespace(*p);
}

// called by parse_number when we know that the output is an integer,
// but where there might be some integer overflow.
// we want to catch overflows!
// Do not call this function directly as it skips some of the checks from
// parse_number
//
// This function will almost never be called!!!
//
static never_inline bool parse_large_integer(const uint8_t *const buf,
                                             ParsedJson &pj,
                                             const uint32_t offset,
                                             bool found_minus) {
  const char *p = reinterpret_cast<const char *>(buf + offset);

  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
  }
  uint64_t i;
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    i = 0;
  } else {
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      if (mul_overflow(i, 10, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false; // overflow
      }
      if (add_overflow(i, digit, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false; // overflow
      }
      ++p;
    }
  }
  if (negative) {
    if (i > 0x8000000000000000) {
       // overflows!
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false; // overflow
    } else if (i == 0x8000000000000000) {
      // In two's complement, we cannot represent 0x8000000000000000
      // as a positive signed integer, but the negative version is 
      // possible.
      constexpr int64_t signed_answer = INT64_MIN;
      pj.write_tape_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, buf + offset);
#endif
    } else {
      // we can negate safely
      int64_t signed_answer = -static_cast<int64_t>(i);
      pj.write_tape_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, buf + offset);
#endif
    }
  } else {
    // we have a positive integer, the contract is that
    // we try to represent it as a signed integer and only 
    // fallback on unsigned integers if absolutely necessary.
    if(i < 0x8000000000000000) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(i, buf + offset);
#endif
      pj.write_tape_s64(i);
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_unsigned_integer(i, buf + offset);
#endif
      pj.write_tape_u64(i);
    }
  }
  return is_structural_or_whitespace(*p);
}

// parse the number at buf + offset
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0 or 1) at high speed.
static really_inline bool parse_number(const uint8_t *const buf, ParsedJson &pj,
                                       const uint32_t offset,
                                       bool found_minus) {
#ifdef SIMDJSON_SKIPNUMBERPARSING // for performance analysis, it is sometimes
                                  // useful to skip parsing
  pj.write_tape_s64(0);           // always write zero
  return true;                    // always succeeds
#else
  const char *p = reinterpret_cast<const char *>(buf + offset);
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
    if (!is_integer(*p)) { // a negative sign must be followed by an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
  }
  const char *const start_digits = p;

  uint64_t i;      // an unsigned int avoids signed overflows (which are bad)
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    if (is_not_structural_or_whitespace_or_exponent_or_decimal(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    i = 0;
  } else {
    if (!(is_integer(*p))) { // must start with an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      // a multiplication by 10 is cheaper than an arbitrary integer
      // multiplication
      i = 10 * i + digit; // might overflow, we will handle the overflow later
      ++p;
    }
  }
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true; // At this point we know that we have a float
    // we continue with the fiction that we have an integer. If the
    // floating point number is representable as x * 10^z for some integer
    // z that fits in 53 bits, then we will be able to convert back the
    // the integer into a float in a lossless manner.
    ++p;
    const char *const first_after_period = p;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // might overflow + multiplication by 10 is likely
                          // cheaper than arbitrary mult.
      // we will handle the overflow later
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
#ifdef SWAR_NUMBER_PARSING
    // this helps if we have lots of decimals!
    // this turns out to be frequent enough.
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
    }
#endif
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
                          // because we have parse_highprecision_float later.
    }
    exponent = first_after_period - p;
  }
  int digit_count =
      p - start_digits - 1; // used later to guard against overflows
  int64_t exp_number = 0;   // exponential part
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    exp_number = digit;
    p++;
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (is_integer(*p)) {
      if (exp_number > 0x100000000) { // we need to check for overflows
                                      // we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    exponent += (neg_exp ? -exp_number : exp_number);
  }
  if (is_float) {
    uint64_t power_index = 308 + exponent;
    if (unlikely((digit_count >= 19))) { // this is uncommon
      // It is possible that the integer had an overflow.
      // We have to handle the case where we have 0.0000somenumber.
      const char *start = start_digits;
      while ((*start == '0') || (*start == '.')) {
        start++;
      }
      // we over-decrement by one when there is a '.'
      digit_count -= (start - start_digits);
      if (digit_count >= 19) {
        // Ok, chances are good that we had an overflow!
        // this is almost never going to get called!!!
        // we start anew, going slowly!!!
        return parse_float(buf, pj, offset, found_minus);
      }
    }
    if (unlikely((power_index > 2 * 308))) { // this is uncommon!!!
      // this is almost never going to get called!!!
      // we start anew, going slowly!!!
      return parse_float(buf, pj, offset, found_minus);
    }
    double factor = power_of_ten[power_index];
    factor = negative ? -factor : factor;
    double d = i * factor;
    pj.write_tape_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_float(d, buf + offset);
#endif
  } else {
    if (unlikely(digit_count >= 18)) { // this is uncommon!!!
      // there is a good chance that we had an overflow, so we need
      // need to recover: we parse the whole thing again.
      return parse_large_integer(buf, pj, offset, found_minus);
    }
    i = negative ? 0 - i : i;
    pj.write_tape_s64(i);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_integer(i, buf + offset);
#endif
  }
  return is_structural_or_whitespace(*p);
#endif // SIMDJSON_SKIPNUMBERPARSING
}

/* end file src/generic/numberparsing.h */

} // namespace simdjson::westmere
UNTARGET_REGION



#endif // IS_X86_64
#endif //  SIMDJSON_WESTMERE_NUMBERPARSING_H
/* end file src/generic/numberparsing.h */

TARGET_WESTMERE
namespace simdjson::westmere {

/* begin file src/generic/stage2_build_tape.h */
// This file contains the common code every implementation uses for stage2
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is include already includes
// "simdjson/stage2_build_tape.h" (this simplifies amalgation)

namespace stage2 {

#ifdef SIMDJSON_USE_COMPUTED_GOTO
typedef void* ret_address;
#define INIT_ADDRESSES() { &&array_begin, &&array_continue, &&error, &&finish, &&object_begin, &&object_continue }
#define GOTO(address) { goto *(address); }
#define CONTINUE(address) { goto *(address); }
#else
typedef char ret_address;
#define INIT_ADDRESSES() { '[', 'a', 'e', 'f', '{', 'o' };
#define GOTO(address)                 \
  {                                   \
    switch(address) {                 \
      case '[': goto array_begin;     \
      case 'a': goto array_continue;  \
      case 'e': goto error;           \
      case 'f': goto finish;          \
      case '{': goto object_begin;    \
      case 'o': goto object_continue; \
    }                                 \
  }
// For the more constrained pop_scope() situation
#define CONTINUE(address)             \
  {                                   \
    switch(address) {                 \
      case 'a': goto array_continue;  \
      case 'o': goto object_continue; \
      case 'f': goto finish;          \
    }                                 \
  }
#endif

struct unified_machine_addresses {
  ret_address array_begin;
  ret_address array_continue;
  ret_address error;
  ret_address finish;
  ret_address object_begin;
  ret_address object_continue;
};

#undef FAIL_IF
#define FAIL_IF(EXPR) { if (EXPR) { return addresses.error; } }

// This is just so we can call parse_string() from parser.parse_string() without conflict.
WARN_UNUSED really_inline bool
really_parse_string(const uint8_t *buf, size_t len, ParsedJson &pj, uint32_t depth, uint32_t idx) {
  return parse_string(buf, len, pj, depth, idx);
}
WARN_UNUSED really_inline bool
really_parse_number(const uint8_t *const buf, ParsedJson &pj, const uint32_t offset, bool found_minus) {
  return parse_number(buf, pj, offset, found_minus);
}

struct structural_parser {
  const uint8_t* const buf;
  const size_t len;
  ParsedJson &pj;
  size_t i; // next structural index
  size_t idx; // location of the structural character in the input (buf)
  uint8_t c;    // used to track the (structural) character we are looking at
  uint32_t depth = 0; // could have an arbitrary starting depth

  really_inline structural_parser(const uint8_t *_buf, size_t _len, ParsedJson &_pj, uint32_t _i = 0) : buf{_buf}, len{_len}, pj{_pj}, i{_i} {}

  WARN_UNUSED really_inline int set_error_code(ErrorValues error_code) {
    pj.error_code = error_code;
    return error_code;
  }

  really_inline char advance_char() {
    idx = pj.structural_indexes[i++];
    c = buf[idx];
    return c;
  }

  template<typename F>
  really_inline bool with_space_terminated_copy(const F& f) {
    /**
    * We need to make a copy to make sure that the string is space terminated.
    * This is not about padding the input, which should already padded up
    * to len + SIMDJSON_PADDING. However, we have no control at this stage
    * on how the padding was done. What if the input string was padded with nulls?
    * It is quite common for an input string to have an extra null character (C string).
    * We do not want to allow 9\0 (where \0 is the null character) inside a JSON
    * document, but the string "9\0" by itself is fine. So we make a copy and
    * pad the input with spaces when we know that there is just one input element.
    * This copy is relatively expensive, but it will almost never be called in
    * practice unless you are in the strange scenario where you have many JSON
    * documents made of single atoms.
    */
    char *copy = static_cast<char *>(malloc(len + SIMDJSON_PADDING));
    if (copy == nullptr) {
      return true;
    }
    memcpy(copy, buf, len);
    memset(copy + len, ' ', SIMDJSON_PADDING);
    bool result = f(reinterpret_cast<const uint8_t*>(copy), idx);
    free(copy);
    return result;
  }

  WARN_UNUSED really_inline bool push_start_scope(ret_address continue_state, char type) {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.ret_address[depth] = continue_state;
    depth++;
    pj.write_tape(0, type);
    return depth >= pj.depth_capacity;
  }

  WARN_UNUSED really_inline bool push_start_scope(ret_address continue_state) {
    return push_start_scope(continue_state, c);
  }

  WARN_UNUSED really_inline bool push_scope(ret_address continue_state) {
    pj.containing_scope_offset[depth] = pj.get_current_loc();
    pj.write_tape(0, c); // Do this as early as possible
    pj.ret_address[depth] = continue_state;
    depth++;
    return depth >= pj.depth_capacity;
  }

  WARN_UNUSED really_inline ret_address pop_scope() {
    // write our tape location to the header scope
    depth--;
    pj.write_tape(pj.containing_scope_offset[depth], c);
    pj.annotate_previous_loc(pj.containing_scope_offset[depth], pj.get_current_loc());
    return pj.ret_address[depth];
  }
  really_inline void pop_root_scope() {
    // write our tape location to the header scope
    // The root scope gets written *at* the previous location.
    depth--;
    pj.annotate_previous_loc(pj.containing_scope_offset[depth], pj.get_current_loc());
    pj.write_tape(pj.containing_scope_offset[depth], 'r');
  }

  WARN_UNUSED really_inline bool parse_string() {
    return !really_parse_string(buf, len, pj, depth, idx);
  }

  WARN_UNUSED really_inline bool parse_number(const uint8_t *copy, uint32_t offset, bool found_minus) {
    return !really_parse_number(copy, pj, offset, found_minus);
  }
  WARN_UNUSED really_inline bool parse_number(bool found_minus) {
    return parse_number(buf, idx, found_minus);
  }

  WARN_UNUSED really_inline bool parse_atom(const uint8_t *copy, uint32_t offset) {
    switch (c) {
      case 't':
        if (!is_valid_true_atom(copy + offset)) { return true; };
        break;
      case 'f':
        if (!is_valid_false_atom(copy + offset)) { return true; }
        break;
      case 'n':
        if (!is_valid_null_atom(copy + offset)) { return true; }
        break;
      default:
        return false;
    }
    pj.write_tape(0, c);
    return false;
  }

  WARN_UNUSED really_inline bool parse_atom() {
    return parse_atom(buf, idx);
  }

  WARN_UNUSED really_inline ret_address parse_value(const unified_machine_addresses &addresses, ret_address continue_state) {
    switch (c) {
    case '"':
      FAIL_IF( parse_string() );
      return continue_state;
    case 't': case 'f': case 'n':
      FAIL_IF( parse_atom() );
      return continue_state;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      FAIL_IF( parse_number(false) );
      return continue_state;
    case '-':
      FAIL_IF( parse_number(true) );
      return continue_state;
    case '{':
      FAIL_IF( push_scope(continue_state) );
      return addresses.object_begin;
    case '[':
      FAIL_IF( push_scope(continue_state) );
      return addresses.array_begin;
    default:
      return addresses.error;
    }
  }

  WARN_UNUSED really_inline int finish() {
    // the string might not be NULL terminated.
    if ( i + 1 != pj.n_structural_indexes ) {
      return set_error_code(TAPE_ERROR);
    }
    pop_root_scope();
    if (depth != 0) {
      return set_error_code(TAPE_ERROR);
    }
    if (pj.containing_scope_offset[depth] != 0) {
      return set_error_code(TAPE_ERROR);
    }

    pj.valid = true;
    return set_error_code(SUCCESS);
  }

  WARN_UNUSED really_inline int error() {
    /* We do not need the next line because this is done by pj.init(),
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
      return set_error_code(DEPTH_ERROR);
    }
    switch (c) {
    case '"':
      return set_error_code(STRING_ERROR);
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
      return set_error_code(NUMBER_ERROR);
    case 't':
      return set_error_code(T_ATOM_ERROR);
    case 'n':
      return set_error_code(N_ATOM_ERROR);
    case 'f':
      return set_error_code(F_ATOM_ERROR);
    default:
      return set_error_code(TAPE_ERROR);
    }
  }

  WARN_UNUSED really_inline int start(ret_address finish_state) {
    pj.init(); // sets is_valid to false
    if (len > pj.byte_capacity) {
      return CAPACITY;
    }
    // Advance to the first character as soon as possible
    advance_char();
    // Push the root scope (there is always at least one scope)
    if (push_start_scope(finish_state, 'r')) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }
};

// Redefine FAIL_IF to use goto since it'll be used inside the function now
#undef FAIL_IF
#define FAIL_IF(EXPR) { if (EXPR) { goto error; } }

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj) {
  static constexpr unified_machine_addresses addresses = INIT_ADDRESSES();
  structural_parser parser(buf, len, pj);
  int result = parser.start(addresses.finish);
  if (result) { return result; }

  //
  // Read first value
  //
  switch (parser.c) {
  case '{':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto object_begin;
  case '[':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto array_begin;
  case '"':
    FAIL_IF( parser.parse_string() );
    goto finish;
  case 't': case 'f': case 'n':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_atom(copy, idx);
      })
    );
    goto finish;
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, false);
      })
    );
    goto finish;
  case '-':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, true);
      })
    );
    goto finish;
  default:
    goto error;
  }

//
// Object parser states
//
object_begin:
  parser.advance_char();
  switch (parser.c) {
  case '"': {
    FAIL_IF( parser.parse_string() );
    goto object_key_state;
  }
  case '}':
    goto scope_end; // could also go to object_continue
  default:
    goto error;
  }

object_key_state:
  FAIL_IF( parser.advance_char() != ':' );
  parser.advance_char();
  GOTO( parser.parse_value(addresses, addresses.object_continue) );

object_continue:
  switch (parser.advance_char()) {
  case ',':
    FAIL_IF( parser.advance_char() != '"' );
    FAIL_IF( parser.parse_string() );
    goto object_key_state;
  case '}':
    goto scope_end;
  default:
    goto error;
  }

scope_end:
  CONTINUE( parser.pop_scope() );

//
// Array parser states
//
array_begin:
  if (parser.advance_char() == ']') {
    goto scope_end; // could also go to array_continue
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at parser.c on the
   * on paths that can accept a close square brace (post-, and at start) */
  GOTO( parser.parse_value(addresses, addresses.array_continue) );

array_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.advance_char();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto error;
  }

finish:
  return parser.finish();

error:
  return parser.error();
}

} // namespace stage2
/* end file src/generic/stage2_build_tape.h */
/* begin file src/generic/stage2_streaming_build_tape.h */
namespace stage2 {

struct streaming_structural_parser: structural_parser {
  really_inline streaming_structural_parser(const uint8_t *_buf, size_t _len, ParsedJson &_pj, size_t _i) : structural_parser(_buf, _len, _pj, _i) {}

  // override to add streaming
  WARN_UNUSED really_inline int start(ret_address finish_parser) {
    pj.init(); // sets is_valid to false
    // Capacity ain't no thang for streaming, so we don't check it.
    // Advance to the first character as soon as possible
    advance_char();
    // Push the root scope (there is always at least one scope)
    if (push_start_scope(finish_parser, 'r')) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }

  // override to add streaming
  WARN_UNUSED really_inline int finish() {
    if ( i + 1 > pj.n_structural_indexes ) {
      return set_error_code(TAPE_ERROR);
    }
    pop_root_scope();
    if (depth != 0) {
      return set_error_code(TAPE_ERROR);
    }
    if (pj.containing_scope_offset[depth] != 0) {
      return set_error_code(TAPE_ERROR);
    }
    bool finished = i + 1 == pj.n_structural_indexes;
    pj.valid = true;
    return set_error_code(finished ? SUCCESS : SUCCESS_AND_HAS_MORE);
  }
};

/************
 * The JSON is parsed to a tape, see the accompanying tape.md file
 * for documentation.
 ***********/
WARN_UNUSED  int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
  static constexpr unified_machine_addresses addresses = INIT_ADDRESSES();
  streaming_structural_parser parser(buf, len, pj, next_json);
  int result = parser.start(addresses.finish);
  if (result) { return result; }
  //
  // Read first value
  //
  switch (parser.c) {
  case '{':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto object_begin;
  case '[':
    FAIL_IF( parser.push_start_scope(addresses.finish) );
    goto array_begin;
  case '"':
    FAIL_IF( parser.parse_string() );
    goto finish;
  case 't': case 'f': case 'n':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_atom(copy, idx);
      })
    );
    goto finish;
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, false);
      })
    );
    goto finish;
  case '-':
    FAIL_IF(
      parser.with_space_terminated_copy([&](auto copy, auto idx) {
        return parser.parse_number(copy, idx, true);
      })
    );
    goto finish;
  default:
    goto error;
  }

//
// Object parser parsers
//
object_begin:
  parser.advance_char();
  switch (parser.c) {
  case '"': {
    FAIL_IF( parser.parse_string() );
    goto object_key_parser;
  }
  case '}':
    goto scope_end; // could also go to object_continue
  default:
    goto error;
  }

object_key_parser:
  FAIL_IF( parser.advance_char() != ':' );
  parser.advance_char();
  GOTO( parser.parse_value(addresses, addresses.object_continue) );

object_continue:
  switch (parser.advance_char()) {
  case ',':
    FAIL_IF( parser.advance_char() != '"' );
    FAIL_IF( parser.parse_string() );
    goto object_key_parser;
  case '}':
    goto scope_end;
  default:
    goto error;
  }

scope_end:
  CONTINUE( parser.pop_scope() );

//
// Array parser parsers
//
array_begin:
  if (parser.advance_char() == ']') {
    goto scope_end; // could also go to array_continue
  }

main_array_switch:
  /* we call update char on all paths in, so we can peek at parser.c on the
   * on paths that can accept a close square brace (post-, and at start) */
  GOTO( parser.parse_value(addresses, addresses.array_continue) );

array_continue:
  switch (parser.advance_char()) {
  case ',':
    parser.advance_char();
    goto main_array_switch;
  case ']':
    goto scope_end;
  default:
    goto error;
  }

finish:
  next_json = parser.i;
  return parser.finish();

error:
  return parser.error();
}

} // namespace stage2
/* end file src/generic/stage2_streaming_build_tape.h */

} // namespace simdjson::westmere
UNTARGET_REGION

TARGET_WESTMERE
namespace simdjson {

template <>
WARN_UNUSED int
unified_machine<Architecture::WESTMERE>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  return westmere::stage2::unified_machine(buf, len, pj);
}

template <>
WARN_UNUSED int
unified_machine<Architecture::WESTMERE>(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    return westmere::stage2::unified_machine(buf, len, pj, next_json);
}


} // namespace simdjson
UNTARGET_REGION

#endif // IS_X86_64

#endif // SIMDJSON_WESTMERE_STAGE2_BUILD_TAPE_H
/* end file src/generic/stage2_streaming_build_tape.h */
/* end file src/generic/stage2_streaming_build_tape.h */
/* begin file src/parsedjson.cpp */

namespace simdjson {

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
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures]);

  // a pathological input like "[[[[..." would generate len tape elements, so
  // need a capacity of at least len + 1, but it is also possible to do
  // worse with "[7,7,7,7,6,7,7,7,6,7,7,6,[7,7,7,7,6,7,7,7,6,7,7,6,7,7,7,7,7,7,6" 
  //where len + 1 tape elements are
  // generated, see issue https://github.com/lemire/simdjson/issues/345
  size_t local_tape_capacity = ROUNDUP_N(len + 2, 64);
  // a document with only zero-length strings... could have len/3 string
  // and we would need len/3 * 5 bytes on the string buffer
  size_t local_string_capacity = ROUNDUP_N(5 * len / 3 + 32, 64);
  string_buf.reset( new (std::nothrow) uint8_t[local_string_capacity]);
  tape.reset(new (std::nothrow) uint64_t[local_tape_capacity]);
  containing_scope_offset.reset(new (std::nothrow) uint32_t[max_depth]);
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  //ret_address = new (std::nothrow) void *[max_depth];
  ret_address.reset(new (std::nothrow) void *[max_depth]);
#else
  ret_address.reset(new (std::nothrow) char[max_depth]);
#endif
  if (!string_buf || !tape ||
      !containing_scope_offset || !ret_address ||
      !structural_indexes) {
    // Could not allocate memory
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
  ret_address.reset();
  containing_scope_offset.reset();
  tape.reset();
  string_buf.reset();
  structural_indexes.reset();
  valid = false;
}

void ParsedJson::init() {
  current_string_buf_loc = string_buf.get();
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
    // Error: no starting root node?
    return false;
  }
  if (how_many > tape_capacity) {
    // We may be exceeding the tape capacity. Is this a valid document?
    return false;
  }
  tape_idx++;
  std::unique_ptr<bool[]> in_object(new bool[depth_capacity]);
  std::unique_ptr<size_t[]> in_object_idx(new size_t[depth_capacity]);
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
      memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
      print_with_escapes(
          (const unsigned char *)(string_buf.get() + payload + sizeof(uint32_t)),
          os, string_length);
      os << '"';
      break;
    case 'l': // we have a long int
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << static_cast<int64_t>(tape[++tape_idx]);
      break;
    case 'u':
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << tape[++tape_idx];
      break;
    case 'd': // we have a double
      if (tape_idx + 1 >= how_many) {
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
      // should we be hitting the root node?
      return false;
    default:
      // bug?
      return false;
    }
  }
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
    // Error: no starting root node?
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
      memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
      print_with_escapes(
          (const unsigned char *)(string_buf.get() + payload + sizeof(uint32_t)),
                  os,
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
      // should we be hitting the root node?
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
