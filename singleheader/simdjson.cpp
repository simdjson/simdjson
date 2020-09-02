/* auto-generated on Wed  2 Sep 2020 18:06:06 EDT. Do not edit! */
/* begin file src/simdjson.cpp */
#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

/* begin file src/error.cpp */

namespace simdjson {
namespace internal {

  SIMDJSON_DLLIMPORTEXPORT const error_code_info error_codes[] {
    { SUCCESS, "No error" },
    { CAPACITY, "This parser can't support a document that big" },
    { MEMALLOC, "Error allocating memory, we're most likely out of memory" },
    { TAPE_ERROR, "The JSON document has an improper structure: missing or superfluous commas, braces, missing keys, etc." },
    { DEPTH_ERROR, "The JSON document was too deep (too many nested objects and arrays)" },
    { STRING_ERROR, "Problem while parsing a string" },
    { T_ATOM_ERROR, "Problem while parsing an atom starting with the letter 't'" },
    { F_ATOM_ERROR, "Problem while parsing an atom starting with the letter 'f'" },
    { N_ATOM_ERROR, "Problem while parsing an atom starting with the letter 'n'" },
    { NUMBER_ERROR, "Problem while parsing a number" },
    { UTF8_ERROR, "The input is not valid UTF-8" },
    { UNINITIALIZED, "Uninitialized" },
    { EMPTY, "Empty: no JSON found" },
    { UNESCAPED_CHARS, "Within strings, some characters must be escaped, we found unescaped characters" },
    { UNCLOSED_STRING, "A string is opened, but never closed." },
    { UNSUPPORTED_ARCHITECTURE, "simdjson does not have an implementation supported by this CPU architecture (perhaps it's a non-SIMD CPU?)." },
    { INCORRECT_TYPE, "The JSON element does not have the requested type." },
    { NUMBER_OUT_OF_RANGE, "The JSON number is too large or too small to fit within the requested type." },
    { INDEX_OUT_OF_BOUNDS, "Attempted to access an element of a JSON array that is beyond its length." },
    { NO_SUCH_FIELD, "The JSON field referenced does not exist in this object." },
    { IO_ERROR, "Error reading the file." },
    { INVALID_JSON_POINTER, "Invalid JSON pointer syntax." },
    { INVALID_URI_FRAGMENT, "Invalid URI fragment syntax." },
    { UNEXPECTED_ERROR, "Unexpected error, consider reporting this problem as you may have found a bug in simdjson" }
  }; // error_messages[]

} // namespace internal
} // namespace simdjson
/* end file src/error.cpp */
/* begin file src/implementation.cpp */
/* begin file src/isadetection.h */
/* From
https://github.com/endorno/pytorch/blob/master/torch/lib/TH/generic/simd/simd.h
Highly modified.

Copyright (c) 2016-     Facebook, Inc            (Adam Paszke)
Copyright (c) 2014-     Facebook, Inc            (Soumith Chintala)
Copyright (c) 2011-2014 Idiap Research Institute (Ronan Collobert)
Copyright (c) 2012-2014 Deepmind Technologies    (Koray Kavukcuoglu)
Copyright (c) 2011-2012 NEC Laboratories America (Koray Kavukcuoglu)
Copyright (c) 2011-2013 NYU                      (Clement Farabet)
Copyright (c) 2006-2010 NEC Laboratories America (Ronan Collobert, Leon Bottou,
Iain Melvin, Jason Weston) Copyright (c) 2006      Idiap Research Institute
(Samy Bengio) Copyright (c) 2001-2004 Idiap Research Institute (Ronan Collobert,
Samy Bengio, Johnny Mariethoz)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the names of Facebook, Deepmind Technologies, NYU, NEC Laboratories
America and IDIAP Research Institute nor the names of its contributors may be
   used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SIMDJSON_ISADETECTION_H
#define SIMDJSON_ISADETECTION_H

#include <stdint.h>
#include <stdlib.h>
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)
#include <cpuid.h>
#endif

namespace simdjson {


enum instruction_set {
  DEFAULT = 0x0,
  NEON = 0x1,
  AVX2 = 0x4,
  SSE42 = 0x8,
  PCLMULQDQ = 0x10,
  BMI1 = 0x20,
  BMI2 = 0x40
};

#if defined(__arm__) || defined(__aarch64__) // incl. armel, armhf, arm64

#if defined(__ARM_NEON)

static inline uint32_t detect_supported_architectures() {
  return instruction_set::NEON;
}

#else // ARM without NEON

static inline uint32_t detect_supported_architectures() {
  return instruction_set::DEFAULT;
}

#endif

#elif defined(__x86_64__) || defined(_M_AMD64) // x64


namespace {
// Can be found on Intel ISA Reference for CPUID
constexpr uint32_t cpuid_avx2_bit = 1 << 5;      ///< @private Bit 5 of EBX for EAX=0x7
constexpr uint32_t cpuid_bmi1_bit = 1 << 3;      ///< @private bit 3 of EBX for EAX=0x7
constexpr uint32_t cpuid_bmi2_bit = 1 << 8;      ///< @private bit 8 of EBX for EAX=0x7
constexpr uint32_t cpuid_sse42_bit = 1 << 20;    ///< @private bit 20 of ECX for EAX=0x1
constexpr uint32_t cpuid_pclmulqdq_bit = 1 << 1; ///< @private bit  1 of ECX for EAX=0x1
}



static inline void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
                         uint32_t *edx) {
#if defined(_MSC_VER)
  int cpu_info[4];
  __cpuid(cpu_info, *eax);
  *eax = cpu_info[0];
  *ebx = cpu_info[1];
  *ecx = cpu_info[2];
  *edx = cpu_info[3];
#elif defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)
  uint32_t level = *eax;
  __get_cpuid(level, eax, ebx, ecx, edx);
#else
  uint32_t a = *eax, b, c = *ecx, d;
  asm volatile("cpuid\n\t" : "+a"(a), "=b"(b), "+c"(c), "=d"(d));
  *eax = a;
  *ebx = b;
  *ecx = c;
  *edx = d;
#endif
}

static inline uint32_t detect_supported_architectures() {
  uint32_t eax, ebx, ecx, edx;
  uint32_t host_isa = 0x0;

  // ECX for EAX=0x7
  eax = 0x7;
  ecx = 0x0;
  cpuid(&eax, &ebx, &ecx, &edx);
  if (ebx & cpuid_avx2_bit) {
    host_isa |= instruction_set::AVX2;
  }
  if (ebx & cpuid_bmi1_bit) {
    host_isa |= instruction_set::BMI1;
  }

  if (ebx & cpuid_bmi2_bit) {
    host_isa |= instruction_set::BMI2;
  }

  // EBX for EAX=0x1
  eax = 0x1;
  cpuid(&eax, &ebx, &ecx, &edx);

  if (ecx & cpuid_sse42_bit) {
    host_isa |= instruction_set::SSE42;
  }

  if (ecx & cpuid_pclmulqdq_bit) {
    host_isa |= instruction_set::PCLMULQDQ;
  }

  return host_isa;
}
#else // fallback


static inline uint32_t detect_supported_architectures() {
  return instruction_set::DEFAULT;
}


#endif // end SIMD extension detection code

} // namespace simdjson

#endif // SIMDJSON_ISADETECTION_H
/* end file src/isadetection.h */
/* begin file src/simdprune_tables.h */
#ifndef SIMDJSON_SIMDPRUNE_TABLES_H
#define SIMDJSON_SIMDPRUNE_TABLES_H


#if SIMDJSON_IMPLEMENTATION_ARM64 || SIMDJSON_IMPLEMENTATION_HASWELL || SIMDJSON_IMPLEMENTATION_WESTMERE

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


#endif //  SIMDJSON_IMPLEMENTATION_ARM64 || SIMDJSON_IMPLEMENTATION_HASWELL || SIMDJSON_IMPLEMENTATION_WESTMERE
#endif // SIMDJSON_SIMDPRUNE_TABLES_H
/* end file src/simdprune_tables.h */

#include <initializer_list>

#define SIMDJSON_TRY(EXPR) { auto _err = (EXPR); if (_err) { return _err; } }

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.

#if SIMDJSON_IMPLEMENTATION_HASWELL
/* begin file src/haswell/implementation.h */
#ifndef SIMDJSON_HASWELL_IMPLEMENTATION_H
#define SIMDJSON_HASWELL_IMPLEMENTATION_H

/* isadetection.h already included: #include "isadetection.h" */

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_REGION
namespace {
namespace haswell {

using namespace simdjson;

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation(
      "haswell",
      "Intel/AMD AVX2",
      instruction_set::AVX2 | instruction_set::PCLMULQDQ | instruction_set::BMI1 | instruction_set::BMI2
  ) {}
  SIMDJSON_WARN_UNUSED error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  SIMDJSON_WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  SIMDJSON_WARN_UNUSED bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace haswell
} // unnamed namespace

#endif // SIMDJSON_HASWELL_IMPLEMENTATION_H
/* end file src/haswell/implementation.h */
namespace simdjson { namespace internal { const haswell::implementation haswell_singleton{}; } }
#endif // SIMDJSON_IMPLEMENTATION_HASWELL

#if SIMDJSON_IMPLEMENTATION_WESTMERE
/* begin file src/westmere/implementation.h */
#ifndef SIMDJSON_WESTMERE_IMPLEMENTATION_H
#define SIMDJSON_WESTMERE_IMPLEMENTATION_H

/* isadetection.h already included: #include "isadetection.h" */

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_REGION
namespace {
namespace westmere {

using namespace simdjson;
using namespace simdjson::dom;

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation("westmere", "Intel/AMD SSE4.2", instruction_set::SSE42 | instruction_set::PCLMULQDQ) {}
  SIMDJSON_WARN_UNUSED error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  SIMDJSON_WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  SIMDJSON_WARN_UNUSED bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace westmere
} // unnamed namespace

#endif // SIMDJSON_WESTMERE_IMPLEMENTATION_H
/* end file src/westmere/implementation.h */
namespace simdjson { namespace internal { const westmere::implementation westmere_singleton{}; } }
#endif // SIMDJSON_IMPLEMENTATION_WESTMERE

#if SIMDJSON_IMPLEMENTATION_ARM64
/* begin file src/arm64/implementation.h */
#ifndef SIMDJSON_ARM64_IMPLEMENTATION_H
#define SIMDJSON_ARM64_IMPLEMENTATION_H

/* isadetection.h already included: #include "isadetection.h" */

namespace {
namespace arm64 {

using namespace simdjson;
using namespace simdjson::dom;

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation("arm64", "ARM NEON", instruction_set::NEON) {}
  SIMDJSON_WARN_UNUSED error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  SIMDJSON_WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  SIMDJSON_WARN_UNUSED bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace arm64
} // unnamed namespace

#endif // SIMDJSON_ARM64_IMPLEMENTATION_H
/* end file src/arm64/implementation.h */
namespace simdjson { namespace internal { const arm64::implementation arm64_singleton{}; } }
#endif // SIMDJSON_IMPLEMENTATION_ARM64

#if SIMDJSON_IMPLEMENTATION_FALLBACK
/* begin file src/fallback/implementation.h */
#ifndef SIMDJSON_FALLBACK_IMPLEMENTATION_H
#define SIMDJSON_FALLBACK_IMPLEMENTATION_H

/* isadetection.h already included: #include "isadetection.h" */

namespace {
namespace fallback {

using namespace simdjson;
using namespace simdjson::dom;

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation(
      "fallback",
      "Generic fallback implementation",
      0
  ) {}
  SIMDJSON_WARN_UNUSED error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  SIMDJSON_WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  SIMDJSON_WARN_UNUSED bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace fallback
} // unnamed namespace

#endif // SIMDJSON_FALLBACK_IMPLEMENTATION_H
/* end file src/fallback/implementation.h */
namespace simdjson { namespace internal { const fallback::implementation fallback_singleton{}; } }
#endif // SIMDJSON_IMPLEMENTATION_FALLBACK

namespace simdjson {
namespace internal {

/**
 * @private Detects best supported implementation on first use, and sets it
 */
class detect_best_supported_implementation_on_first_use final : public implementation {
public:
  const std::string &name() const noexcept final { return set_best()->name(); }
  const std::string &description() const noexcept final { return set_best()->description(); }
  uint32_t required_instruction_sets() const noexcept final { return set_best()->required_instruction_sets(); }
  SIMDJSON_WARN_UNUSED error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final {
    return set_best()->create_dom_parser_implementation(capacity, max_length, dst);
  }
  SIMDJSON_WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final {
    return set_best()->minify(buf, len, dst, dst_len);
  }
  SIMDJSON_WARN_UNUSED bool validate_utf8(const char * buf, size_t len) const noexcept final override {
    return set_best()->validate_utf8(buf, len);
  }
  simdjson_really_inline detect_best_supported_implementation_on_first_use() noexcept : implementation("best_supported_detector", "Detects the best supported implementation and sets it", 0) {}
private:
  const implementation *set_best() const noexcept;
};

const detect_best_supported_implementation_on_first_use detect_best_supported_implementation_on_first_use_singleton;

const std::initializer_list<const implementation *> available_implementation_pointers {
#if SIMDJSON_IMPLEMENTATION_HASWELL
  &haswell_singleton,
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
  &westmere_singleton,
#endif
#if SIMDJSON_IMPLEMENTATION_ARM64
  &arm64_singleton,
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
  &fallback_singleton,
#endif
}; // available_implementation_pointers

// So we can return UNSUPPORTED_ARCHITECTURE from the parser when there is no support
class unsupported_implementation final : public implementation {
public:
  SIMDJSON_WARN_UNUSED error_code create_dom_parser_implementation(
    size_t,
    size_t,
    std::unique_ptr<internal::dom_parser_implementation>&
  ) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  SIMDJSON_WARN_UNUSED error_code minify(const uint8_t *, size_t, uint8_t *, size_t &) const noexcept final override {
    return UNSUPPORTED_ARCHITECTURE;
  }
  SIMDJSON_WARN_UNUSED bool validate_utf8(const char *, size_t) const noexcept final override {
    return false; // Just refuse to validate. Given that we have a fallback implementation
    // it seems unlikely that unsupported_implementation will ever be used. If it is used,
    // then it will flag all strings as invalid. The alternative is to return an error_code
    // from which the user has to figure out whether the string is valid UTF-8... which seems
    // like a lot of work just to handle the very unlikely case that we have an unsupported
    // implementation. And, when it does happen (that we have an unsupported implementation),
    // what are the chances that the programmer has a fallback? Given that *we* provide the
    // fallback, it implies that the programmer would need a fallback for our fallback.
  }
  unsupported_implementation() : implementation("unsupported", "Unsupported CPU (no detected SIMD instructions)", 0) {}
};

const unsupported_implementation unsupported_singleton{};

size_t available_implementation_list::size() const noexcept {
  return internal::available_implementation_pointers.size();
}
const implementation * const *available_implementation_list::begin() const noexcept {
  return internal::available_implementation_pointers.begin();
}
const implementation * const *available_implementation_list::end() const noexcept {
  return internal::available_implementation_pointers.end();
}
const implementation *available_implementation_list::detect_best_supported() const noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets = detect_supported_architectures();
  for (const implementation *impl : internal::available_implementation_pointers) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return impl; }
  }
  return &unsupported_singleton; // this should never happen?
}

const implementation *detect_best_supported_implementation_on_first_use::set_best() const noexcept {
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  char *force_implementation_name = getenv("SIMDJSON_FORCE_IMPLEMENTATION");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (force_implementation_name) {
    auto force_implementation = available_implementations[force_implementation_name];
    if (force_implementation) {
      return active_implementation = force_implementation;
    } else {
      // Note: abort() and stderr usage within the library is forbidden.
      return active_implementation = &unsupported_singleton;
    }
  }
  return active_implementation = available_implementations.detect_best_supported();
}

} // namespace internal

SIMDJSON_DLLIMPORTEXPORT const internal::available_implementation_list available_implementations{};
SIMDJSON_DLLIMPORTEXPORT internal::atomic_ptr<const implementation> active_implementation{&internal::detect_best_supported_implementation_on_first_use_singleton};

SIMDJSON_WARN_UNUSED error_code minify(const char *buf, size_t len, char *dst, size_t &dst_len) noexcept {
  return active_implementation->minify((const uint8_t *)buf, len, (uint8_t *)dst, dst_len);
}
SIMDJSON_WARN_UNUSED bool validate_utf8(const char *buf, size_t len) noexcept {
  return active_implementation->validate_utf8(buf, len);
}


} // namespace simdjson
/* end file src/fallback/implementation.h */

// Anything in the top level directory MUST be included outside of the #if statements
// below, or amalgamation will screw them up!
/* isadetection.h already included: #include "isadetection.h" */
/* begin file src/jsoncharutils_tables.h */
#ifndef SIMDJSON_JSONCHARUTILS_TABLES_H
#define SIMDJSON_JSONCHARUTILS_TABLES_H


#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

namespace simdjson {
// structural chars here are
// they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c (and NULL)
// we are also interested in the four whitespace characters
// space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d

const bool structural_or_whitespace_negated[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

const bool structural_or_whitespace[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const uint32_t digit_to_val32[886] = {
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0x0,        0x1,        0x2,        0x3,        0x4,        0x5,
    0x6,        0x7,        0x8,        0x9,        0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xa,
    0xb,        0xc,        0xd,        0xe,        0xf,        0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xa,        0xb,        0xc,        0xd,        0xe,
    0xf,        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0x0,        0x10,       0x20,       0x30,       0x40,       0x50,
    0x60,       0x70,       0x80,       0x90,       0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xa0,
    0xb0,       0xc0,       0xd0,       0xe0,       0xf0,       0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xa0,       0xb0,       0xc0,       0xd0,       0xe0,
    0xf0,       0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0x0,        0x100,      0x200,      0x300,      0x400,      0x500,
    0x600,      0x700,      0x800,      0x900,      0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xa00,
    0xb00,      0xc00,      0xd00,      0xe00,      0xf00,      0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xa00,      0xb00,      0xc00,      0xd00,      0xe00,
    0xf00,      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0x0,        0x1000,     0x2000,     0x3000,     0x4000,     0x5000,
    0x6000,     0x7000,     0x8000,     0x9000,     0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xa000,
    0xb000,     0xc000,     0xd000,     0xe000,     0xf000,     0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xa000,     0xb000,     0xc000,     0xd000,     0xe000,
    0xf000,     0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
////
// The following code is used in number parsing. It is not
// properly "char utils" stuff, but we move it here so that
// it does not get copied multiple times in the binaries (once
// per instruction set).
///

constexpr int FASTFLOAT_SMALLEST_POWER = -325;
constexpr int FASTFLOAT_LARGEST_POWER = 308;

struct value128 {
  uint64_t low;
  uint64_t high;
};

// Precomputed powers of ten from 10^0 to 10^22. These
// can be represented exactly using the double type.
static const double power_of_ten[] = {
    1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
    1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

// The mantissas of powers of ten from -308 to 308, extended out to sixty four
// bits. The array contains the powers of ten approximated
// as a 64-bit mantissa. It goes from 10^FASTFLOAT_SMALLEST_POWER to 
// 10^FASTFLOAT_LARGEST_POWER (inclusively). 
// The mantissa is truncated, and
// never rounded up. Uses about 5KB.
static const uint64_t mantissa_64[] = {
    0xa5ced43b7e3e9188, 0xcf42894a5dce35ea,
    0x818995ce7aa0e1b2, 0xa1ebfb4219491a1f,
    0xca66fa129f9b60a6, 0xfd00b897478238d0,
    0x9e20735e8cb16382, 0xc5a890362fddbc62,
    0xf712b443bbd52b7b, 0x9a6bb0aa55653b2d,
    0xc1069cd4eabe89f8, 0xf148440a256e2c76,
    0x96cd2a865764dbca, 0xbc807527ed3e12bc,
    0xeba09271e88d976b, 0x93445b8731587ea3,
    0xb8157268fdae9e4c, 0xe61acf033d1a45df,
    0x8fd0c16206306bab, 0xb3c4f1ba87bc8696,
    0xe0b62e2929aba83c, 0x8c71dcd9ba0b4925,
    0xaf8e5410288e1b6f, 0xdb71e91432b1a24a,
    0x892731ac9faf056e, 0xab70fe17c79ac6ca,
    0xd64d3d9db981787d, 0x85f0468293f0eb4e,
    0xa76c582338ed2621, 0xd1476e2c07286faa,
    0x82cca4db847945ca, 0xa37fce126597973c,
    0xcc5fc196fefd7d0c, 0xff77b1fcbebcdc4f,
    0x9faacf3df73609b1, 0xc795830d75038c1d,
    0xf97ae3d0d2446f25, 0x9becce62836ac577,
    0xc2e801fb244576d5, 0xf3a20279ed56d48a,
    0x9845418c345644d6, 0xbe5691ef416bd60c,
    0xedec366b11c6cb8f, 0x94b3a202eb1c3f39,
    0xb9e08a83a5e34f07, 0xe858ad248f5c22c9,
    0x91376c36d99995be, 0xb58547448ffffb2d,
    0xe2e69915b3fff9f9, 0x8dd01fad907ffc3b,
    0xb1442798f49ffb4a, 0xdd95317f31c7fa1d,
    0x8a7d3eef7f1cfc52, 0xad1c8eab5ee43b66,
    0xd863b256369d4a40, 0x873e4f75e2224e68,
    0xa90de3535aaae202, 0xd3515c2831559a83,
    0x8412d9991ed58091, 0xa5178fff668ae0b6,
    0xce5d73ff402d98e3, 0x80fa687f881c7f8e,
    0xa139029f6a239f72, 0xc987434744ac874e,
    0xfbe9141915d7a922, 0x9d71ac8fada6c9b5,
    0xc4ce17b399107c22, 0xf6019da07f549b2b,
    0x99c102844f94e0fb, 0xc0314325637a1939,
    0xf03d93eebc589f88, 0x96267c7535b763b5,
    0xbbb01b9283253ca2, 0xea9c227723ee8bcb,
    0x92a1958a7675175f, 0xb749faed14125d36,
    0xe51c79a85916f484, 0x8f31cc0937ae58d2,
    0xb2fe3f0b8599ef07, 0xdfbdcece67006ac9,
    0x8bd6a141006042bd, 0xaecc49914078536d,
    0xda7f5bf590966848, 0x888f99797a5e012d,
    0xaab37fd7d8f58178, 0xd5605fcdcf32e1d6,
    0x855c3be0a17fcd26, 0xa6b34ad8c9dfc06f,
    0xd0601d8efc57b08b, 0x823c12795db6ce57,
    0xa2cb1717b52481ed, 0xcb7ddcdda26da268,
    0xfe5d54150b090b02, 0x9efa548d26e5a6e1,
    0xc6b8e9b0709f109a, 0xf867241c8cc6d4c0,
    0x9b407691d7fc44f8, 0xc21094364dfb5636,
    0xf294b943e17a2bc4, 0x979cf3ca6cec5b5a,
    0xbd8430bd08277231, 0xece53cec4a314ebd,
    0x940f4613ae5ed136, 0xb913179899f68584,
    0xe757dd7ec07426e5, 0x9096ea6f3848984f,
    0xb4bca50b065abe63, 0xe1ebce4dc7f16dfb,
    0x8d3360f09cf6e4bd, 0xb080392cc4349dec,
    0xdca04777f541c567, 0x89e42caaf9491b60,
    0xac5d37d5b79b6239, 0xd77485cb25823ac7,
    0x86a8d39ef77164bc, 0xa8530886b54dbdeb,
    0xd267caa862a12d66, 0x8380dea93da4bc60,
    0xa46116538d0deb78, 0xcd795be870516656,
    0x806bd9714632dff6, 0xa086cfcd97bf97f3,
    0xc8a883c0fdaf7df0, 0xfad2a4b13d1b5d6c,
    0x9cc3a6eec6311a63, 0xc3f490aa77bd60fc,
    0xf4f1b4d515acb93b, 0x991711052d8bf3c5,
    0xbf5cd54678eef0b6, 0xef340a98172aace4,
    0x9580869f0e7aac0e, 0xbae0a846d2195712,
    0xe998d258869facd7, 0x91ff83775423cc06,
    0xb67f6455292cbf08, 0xe41f3d6a7377eeca,
    0x8e938662882af53e, 0xb23867fb2a35b28d,
    0xdec681f9f4c31f31, 0x8b3c113c38f9f37e,
    0xae0b158b4738705e, 0xd98ddaee19068c76,
    0x87f8a8d4cfa417c9, 0xa9f6d30a038d1dbc,
    0xd47487cc8470652b, 0x84c8d4dfd2c63f3b,
    0xa5fb0a17c777cf09, 0xcf79cc9db955c2cc,
    0x81ac1fe293d599bf, 0xa21727db38cb002f,
    0xca9cf1d206fdc03b, 0xfd442e4688bd304a,
    0x9e4a9cec15763e2e, 0xc5dd44271ad3cdba,
    0xf7549530e188c128, 0x9a94dd3e8cf578b9,
    0xc13a148e3032d6e7, 0xf18899b1bc3f8ca1,
    0x96f5600f15a7b7e5, 0xbcb2b812db11a5de,
    0xebdf661791d60f56, 0x936b9fcebb25c995,
    0xb84687c269ef3bfb, 0xe65829b3046b0afa,
    0x8ff71a0fe2c2e6dc, 0xb3f4e093db73a093,
    0xe0f218b8d25088b8, 0x8c974f7383725573,
    0xafbd2350644eeacf, 0xdbac6c247d62a583,
    0x894bc396ce5da772, 0xab9eb47c81f5114f,
    0xd686619ba27255a2, 0x8613fd0145877585,
    0xa798fc4196e952e7, 0xd17f3b51fca3a7a0,
    0x82ef85133de648c4, 0xa3ab66580d5fdaf5,
    0xcc963fee10b7d1b3, 0xffbbcfe994e5c61f,
    0x9fd561f1fd0f9bd3, 0xc7caba6e7c5382c8,
    0xf9bd690a1b68637b, 0x9c1661a651213e2d,
    0xc31bfa0fe5698db8, 0xf3e2f893dec3f126,
    0x986ddb5c6b3a76b7, 0xbe89523386091465,
    0xee2ba6c0678b597f, 0x94db483840b717ef,
    0xba121a4650e4ddeb, 0xe896a0d7e51e1566,
    0x915e2486ef32cd60, 0xb5b5ada8aaff80b8,
    0xe3231912d5bf60e6, 0x8df5efabc5979c8f,
    0xb1736b96b6fd83b3, 0xddd0467c64bce4a0,
    0x8aa22c0dbef60ee4, 0xad4ab7112eb3929d,
    0xd89d64d57a607744, 0x87625f056c7c4a8b,
    0xa93af6c6c79b5d2d, 0xd389b47879823479,
    0x843610cb4bf160cb, 0xa54394fe1eedb8fe,
    0xce947a3da6a9273e, 0x811ccc668829b887,
    0xa163ff802a3426a8, 0xc9bcff6034c13052,
    0xfc2c3f3841f17c67, 0x9d9ba7832936edc0,
    0xc5029163f384a931, 0xf64335bcf065d37d,
    0x99ea0196163fa42e, 0xc06481fb9bcf8d39,
    0xf07da27a82c37088, 0x964e858c91ba2655,
    0xbbe226efb628afea, 0xeadab0aba3b2dbe5,
    0x92c8ae6b464fc96f, 0xb77ada0617e3bbcb,
    0xe55990879ddcaabd, 0x8f57fa54c2a9eab6,
    0xb32df8e9f3546564, 0xdff9772470297ebd,
    0x8bfbea76c619ef36, 0xaefae51477a06b03,
    0xdab99e59958885c4, 0x88b402f7fd75539b,
    0xaae103b5fcd2a881, 0xd59944a37c0752a2,
    0x857fcae62d8493a5, 0xa6dfbd9fb8e5b88e,
    0xd097ad07a71f26b2, 0x825ecc24c873782f,
    0xa2f67f2dfa90563b, 0xcbb41ef979346bca,
    0xfea126b7d78186bc, 0x9f24b832e6b0f436,
    0xc6ede63fa05d3143, 0xf8a95fcf88747d94,
    0x9b69dbe1b548ce7c, 0xc24452da229b021b,
    0xf2d56790ab41c2a2, 0x97c560ba6b0919a5,
    0xbdb6b8e905cb600f, 0xed246723473e3813,
    0x9436c0760c86e30b, 0xb94470938fa89bce,
    0xe7958cb87392c2c2, 0x90bd77f3483bb9b9,
    0xb4ecd5f01a4aa828, 0xe2280b6c20dd5232,
    0x8d590723948a535f, 0xb0af48ec79ace837,
    0xdcdb1b2798182244, 0x8a08f0f8bf0f156b,
    0xac8b2d36eed2dac5, 0xd7adf884aa879177,
    0x86ccbb52ea94baea, 0xa87fea27a539e9a5,
    0xd29fe4b18e88640e, 0x83a3eeeef9153e89,
    0xa48ceaaab75a8e2b, 0xcdb02555653131b6,
    0x808e17555f3ebf11, 0xa0b19d2ab70e6ed6,
    0xc8de047564d20a8b, 0xfb158592be068d2e,
    0x9ced737bb6c4183d, 0xc428d05aa4751e4c,
    0xf53304714d9265df, 0x993fe2c6d07b7fab,
    0xbf8fdb78849a5f96, 0xef73d256a5c0f77c,
    0x95a8637627989aad, 0xbb127c53b17ec159,
    0xe9d71b689dde71af, 0x9226712162ab070d,
    0xb6b00d69bb55c8d1, 0xe45c10c42a2b3b05,
    0x8eb98a7a9a5b04e3, 0xb267ed1940f1c61c,
    0xdf01e85f912e37a3, 0x8b61313bbabce2c6,
    0xae397d8aa96c1b77, 0xd9c7dced53c72255,
    0x881cea14545c7575, 0xaa242499697392d2,
    0xd4ad2dbfc3d07787, 0x84ec3c97da624ab4,
    0xa6274bbdd0fadd61, 0xcfb11ead453994ba,
    0x81ceb32c4b43fcf4, 0xa2425ff75e14fc31,
    0xcad2f7f5359a3b3e, 0xfd87b5f28300ca0d,
    0x9e74d1b791e07e48, 0xc612062576589dda,
    0xf79687aed3eec551, 0x9abe14cd44753b52,
    0xc16d9a0095928a27, 0xf1c90080baf72cb1,
    0x971da05074da7bee, 0xbce5086492111aea,
    0xec1e4a7db69561a5, 0x9392ee8e921d5d07,
    0xb877aa3236a4b449, 0xe69594bec44de15b,
    0x901d7cf73ab0acd9, 0xb424dc35095cd80f,
    0xe12e13424bb40e13, 0x8cbccc096f5088cb,
    0xafebff0bcb24aafe, 0xdbe6fecebdedd5be,
    0x89705f4136b4a597, 0xabcc77118461cefc,
    0xd6bf94d5e57a42bc, 0x8637bd05af6c69b5,
    0xa7c5ac471b478423, 0xd1b71758e219652b,
    0x83126e978d4fdf3b, 0xa3d70a3d70a3d70a,
    0xcccccccccccccccc, 0x8000000000000000,
    0xa000000000000000, 0xc800000000000000,
    0xfa00000000000000, 0x9c40000000000000,
    0xc350000000000000, 0xf424000000000000,
    0x9896800000000000, 0xbebc200000000000,
    0xee6b280000000000, 0x9502f90000000000,
    0xba43b74000000000, 0xe8d4a51000000000,
    0x9184e72a00000000, 0xb5e620f480000000,
    0xe35fa931a0000000, 0x8e1bc9bf04000000,
    0xb1a2bc2ec5000000, 0xde0b6b3a76400000,
    0x8ac7230489e80000, 0xad78ebc5ac620000,
    0xd8d726b7177a8000, 0x878678326eac9000,
    0xa968163f0a57b400, 0xd3c21bcecceda100,
    0x84595161401484a0, 0xa56fa5b99019a5c8,
    0xcecb8f27f4200f3a, 0x813f3978f8940984,
    0xa18f07d736b90be5, 0xc9f2c9cd04674ede,
    0xfc6f7c4045812296, 0x9dc5ada82b70b59d,
    0xc5371912364ce305, 0xf684df56c3e01bc6,
    0x9a130b963a6c115c, 0xc097ce7bc90715b3,
    0xf0bdc21abb48db20, 0x96769950b50d88f4,
    0xbc143fa4e250eb31, 0xeb194f8e1ae525fd,
    0x92efd1b8d0cf37be, 0xb7abc627050305ad,
    0xe596b7b0c643c719, 0x8f7e32ce7bea5c6f,
    0xb35dbf821ae4f38b, 0xe0352f62a19e306e,
    0x8c213d9da502de45, 0xaf298d050e4395d6,
    0xdaf3f04651d47b4c, 0x88d8762bf324cd0f,
    0xab0e93b6efee0053, 0xd5d238a4abe98068,
    0x85a36366eb71f041, 0xa70c3c40a64e6c51,
    0xd0cf4b50cfe20765, 0x82818f1281ed449f,
    0xa321f2d7226895c7, 0xcbea6f8ceb02bb39,
    0xfee50b7025c36a08, 0x9f4f2726179a2245,
    0xc722f0ef9d80aad6, 0xf8ebad2b84e0d58b,
    0x9b934c3b330c8577, 0xc2781f49ffcfa6d5,
    0xf316271c7fc3908a, 0x97edd871cfda3a56,
    0xbde94e8e43d0c8ec, 0xed63a231d4c4fb27,
    0x945e455f24fb1cf8, 0xb975d6b6ee39e436,
    0xe7d34c64a9c85d44, 0x90e40fbeea1d3a4a,
    0xb51d13aea4a488dd, 0xe264589a4dcdab14,
    0x8d7eb76070a08aec, 0xb0de65388cc8ada8,
    0xdd15fe86affad912, 0x8a2dbf142dfcc7ab,
    0xacb92ed9397bf996, 0xd7e77a8f87daf7fb,
    0x86f0ac99b4e8dafd, 0xa8acd7c0222311bc,
    0xd2d80db02aabd62b, 0x83c7088e1aab65db,
    0xa4b8cab1a1563f52, 0xcde6fd5e09abcf26,
    0x80b05e5ac60b6178, 0xa0dc75f1778e39d6,
    0xc913936dd571c84c, 0xfb5878494ace3a5f,
    0x9d174b2dcec0e47b, 0xc45d1df942711d9a,
    0xf5746577930d6500, 0x9968bf6abbe85f20,
    0xbfc2ef456ae276e8, 0xefb3ab16c59b14a2,
    0x95d04aee3b80ece5, 0xbb445da9ca61281f,
    0xea1575143cf97226, 0x924d692ca61be758,
    0xb6e0c377cfa2e12e, 0xe498f455c38b997a,
    0x8edf98b59a373fec, 0xb2977ee300c50fe7,
    0xdf3d5e9bc0f653e1, 0x8b865b215899f46c,
    0xae67f1e9aec07187, 0xda01ee641a708de9,
    0x884134fe908658b2, 0xaa51823e34a7eede,
    0xd4e5e2cdc1d1ea96, 0x850fadc09923329e,
    0xa6539930bf6bff45, 0xcfe87f7cef46ff16,
    0x81f14fae158c5f6e, 0xa26da3999aef7749,
    0xcb090c8001ab551c, 0xfdcb4fa002162a63,
    0x9e9f11c4014dda7e, 0xc646d63501a1511d,
    0xf7d88bc24209a565, 0x9ae757596946075f,
    0xc1a12d2fc3978937, 0xf209787bb47d6b84,
    0x9745eb4d50ce6332, 0xbd176620a501fbff,
    0xec5d3fa8ce427aff, 0x93ba47c980e98cdf,
    0xb8a8d9bbe123f017, 0xe6d3102ad96cec1d,
    0x9043ea1ac7e41392, 0xb454e4a179dd1877,
    0xe16a1dc9d8545e94, 0x8ce2529e2734bb1d,
    0xb01ae745b101e9e4, 0xdc21a1171d42645d,
    0x899504ae72497eba, 0xabfa45da0edbde69,
    0xd6f8d7509292d603, 0x865b86925b9bc5c2,
    0xa7f26836f282b732, 0xd1ef0244af2364ff,
    0x8335616aed761f1f, 0xa402b9c5a8d3a6e7,
    0xcd036837130890a1, 0x802221226be55a64,
    0xa02aa96b06deb0fd, 0xc83553c5c8965d3d,
    0xfa42a8b73abbf48c, 0x9c69a97284b578d7,
    0xc38413cf25e2d70d, 0xf46518c2ef5b8cd1,
    0x98bf2f79d5993802, 0xbeeefb584aff8603,
    0xeeaaba2e5dbf6784, 0x952ab45cfa97a0b2,
    0xba756174393d88df, 0xe912b9d1478ceb17,
    0x91abb422ccb812ee, 0xb616a12b7fe617aa,
    0xe39c49765fdf9d94, 0x8e41ade9fbebc27d,
    0xb1d219647ae6b31c, 0xde469fbd99a05fe3,
    0x8aec23d680043bee, 0xada72ccc20054ae9,
    0xd910f7ff28069da4, 0x87aa9aff79042286,
    0xa99541bf57452b28, 0xd3fa922f2d1675f2,
    0x847c9b5d7c2e09b7, 0xa59bc234db398c25,
    0xcf02b2c21207ef2e, 0x8161afb94b44f57d,
    0xa1ba1ba79e1632dc, 0xca28a291859bbf93,
    0xfcb2cb35e702af78, 0x9defbf01b061adab,
    0xc56baec21c7a1916, 0xf6c69a72a3989f5b,
    0x9a3c2087a63f6399, 0xc0cb28a98fcf3c7f,
    0xf0fdf2d3f3c30b9f, 0x969eb7c47859e743,
    0xbc4665b596706114, 0xeb57ff22fc0c7959,
    0x9316ff75dd87cbd8, 0xb7dcbf5354e9bece,
    0xe5d3ef282a242e81, 0x8fa475791a569d10,
    0xb38d92d760ec4455, 0xe070f78d3927556a,
    0x8c469ab843b89562, 0xaf58416654a6babb,
    0xdb2e51bfe9d0696a, 0x88fcf317f22241e2,
    0xab3c2fddeeaad25a, 0xd60b3bd56a5586f1,
    0x85c7056562757456, 0xa738c6bebb12d16c,
    0xd106f86e69d785c7, 0x82a45b450226b39c,
    0xa34d721642b06084, 0xcc20ce9bd35c78a5,
    0xff290242c83396ce, 0x9f79a169bd203e41,
    0xc75809c42c684dd1, 0xf92e0c3537826145,
    0x9bbcc7a142b17ccb, 0xc2abf989935ddbfe,
    0xf356f7ebf83552fe, 0x98165af37b2153de,
    0xbe1bf1b059e9a8d6, 0xeda2ee1c7064130c,
    0x9485d4d1c63e8be7, 0xb9a74a0637ce2ee1,
    0xe8111c87c5c1ba99, 0x910ab1d4db9914a0,
    0xb54d5e4a127f59c8, 0xe2a0b5dc971f303a,
    0x8da471a9de737e24, 0xb10d8e1456105dad,
    0xdd50f1996b947518, 0x8a5296ffe33cc92f,
    0xace73cbfdc0bfb7b, 0xd8210befd30efa5a,
    0x8714a775e3e95c78, 0xa8d9d1535ce3b396,
    0xd31045a8341ca07c, 0x83ea2b892091e44d,
    0xa4e4b66b68b65d60, 0xce1de40642e3f4b9,
    0x80d2ae83e9ce78f3, 0xa1075a24e4421730,
    0xc94930ae1d529cfc, 0xfb9b7cd9a4a7443c,
    0x9d412e0806e88aa5, 0xc491798a08a2ad4e,
    0xf5b5d7ec8acb58a2, 0x9991a6f3d6bf1765,
    0xbff610b0cc6edd3f, 0xeff394dcff8a948e,
    0x95f83d0a1fb69cd9, 0xbb764c4ca7a4440f,
    0xea53df5fd18d5513, 0x92746b9be2f8552c,
    0xb7118682dbb66a77, 0xe4d5e82392a40515,
    0x8f05b1163ba6832d, 0xb2c71d5bca9023f8,
    0xdf78e4b2bd342cf6, 0x8bab8eefb6409c1a,
    0xae9672aba3d0c320, 0xda3c0f568cc4f3e8,
    0x8865899617fb1871, 0xaa7eebfb9df9de8d,
    0xd51ea6fa85785631, 0x8533285c936b35de,
    0xa67ff273b8460356, 0xd01fef10a657842c,
    0x8213f56a67f6b29b, 0xa298f2c501f45f42,
    0xcb3f2f7642717713, 0xfe0efb53d30dd4d7,
    0x9ec95d1463e8a506, 0xc67bb4597ce2ce48,
    0xf81aa16fdc1b81da, 0x9b10a4e5e9913128,
    0xc1d4ce1f63f57d72, 0xf24a01a73cf2dccf,
    0x976e41088617ca01, 0xbd49d14aa79dbc82,
    0xec9c459d51852ba2, 0x93e1ab8252f33b45,
    0xb8da1662e7b00a17, 0xe7109bfba19c0c9d,
    0x906a617d450187e2, 0xb484f9dc9641e9da,
    0xe1a63853bbd26451, 0x8d07e33455637eb2,
    0xb049dc016abc5e5f, 0xdc5c5301c56b75f7,
    0x89b9b3e11b6329ba, 0xac2820d9623bf429,
    0xd732290fbacaf133, 0x867f59a9d4bed6c0,
    0xa81f301449ee8c70, 0xd226fc195c6a2f8c,
    0x83585d8fd9c25db7, 0xa42e74f3d032f525,
    0xcd3a1230c43fb26f, 0x80444b5e7aa7cf85,
    0xa0555e361951c366, 0xc86ab5c39fa63440,
    0xfa856334878fc150, 0x9c935e00d4b9d8d2,
    0xc3b8358109e84f07, 0xf4a642e14c6262c8,
    0x98e7e9cccfbd7dbd, 0xbf21e44003acdd2c,
    0xeeea5d5004981478, 0x95527a5202df0ccb,
    0xbaa718e68396cffd, 0xe950df20247c83fd,
    0x91d28b7416cdd27e, 0xb6472e511c81471d,
    0xe3d8f9e563a198e5, 0x8e679c2f5e44ff8f};
// A complement to mantissa_64
// complete to a 128-bit mantissa.
// Uses about 5KB but is rarely accessed.
const uint64_t mantissa_128[] = {
    0x419ea3bd35385e2d, 0x52064cac828675b9,
    0x7343efebd1940993, 0x1014ebe6c5f90bf8,
    0xd41a26e077774ef6, 0x8920b098955522b4,
    0x55b46e5f5d5535b0, 0xeb2189f734aa831d,
    0xa5e9ec7501d523e4, 0x47b233c92125366e,
    0x999ec0bb696e840a, 0xc00670ea43ca250d,
    0x380406926a5e5728, 0xc605083704f5ecf2,
    0xf7864a44c633682e, 0x7ab3ee6afbe0211d,
    0x5960ea05bad82964, 0x6fb92487298e33bd,
    0xa5d3b6d479f8e056, 0x8f48a4899877186c,
    0x331acdabfe94de87, 0x9ff0c08b7f1d0b14,
    0x7ecf0ae5ee44dd9, 0xc9e82cd9f69d6150,
    0xbe311c083a225cd2, 0x6dbd630a48aaf406,
    0x92cbbccdad5b108, 0x25bbf56008c58ea5,
    0xaf2af2b80af6f24e, 0x1af5af660db4aee1,
    0x50d98d9fc890ed4d, 0xe50ff107bab528a0,
    0x1e53ed49a96272c8, 0x25e8e89c13bb0f7a,
    0x77b191618c54e9ac, 0xd59df5b9ef6a2417,
    0x4b0573286b44ad1d, 0x4ee367f9430aec32,
    0x229c41f793cda73f, 0x6b43527578c1110f,
    0x830a13896b78aaa9, 0x23cc986bc656d553,
    0x2cbfbe86b7ec8aa8, 0x7bf7d71432f3d6a9,
    0xdaf5ccd93fb0cc53, 0xd1b3400f8f9cff68,
    0x23100809b9c21fa1, 0xabd40a0c2832a78a,
    0x16c90c8f323f516c, 0xae3da7d97f6792e3,
    0x99cd11cfdf41779c, 0x40405643d711d583,
    0x482835ea666b2572, 0xda3243650005eecf,
    0x90bed43e40076a82, 0x5a7744a6e804a291,
    0x711515d0a205cb36, 0xd5a5b44ca873e03,
    0xe858790afe9486c2, 0x626e974dbe39a872,
    0xfb0a3d212dc8128f, 0x7ce66634bc9d0b99,
    0x1c1fffc1ebc44e80, 0xa327ffb266b56220,
    0x4bf1ff9f0062baa8, 0x6f773fc3603db4a9,
    0xcb550fb4384d21d3, 0x7e2a53a146606a48,
    0x2eda7444cbfc426d, 0xfa911155fefb5308,
    0x793555ab7eba27ca, 0x4bc1558b2f3458de,
    0x9eb1aaedfb016f16, 0x465e15a979c1cadc,
    0xbfacd89ec191ec9, 0xcef980ec671f667b,
    0x82b7e12780e7401a, 0xd1b2ecb8b0908810,
    0x861fa7e6dcb4aa15, 0x67a791e093e1d49a,
    0xe0c8bb2c5c6d24e0, 0x58fae9f773886e18,
    0xaf39a475506a899e, 0x6d8406c952429603,
    0xc8e5087ba6d33b83, 0xfb1e4a9a90880a64,
    0x5cf2eea09a55067f, 0xf42faa48c0ea481e,
    0xf13b94daf124da26, 0x76c53d08d6b70858,
    0x54768c4b0c64ca6e, 0xa9942f5dcf7dfd09,
    0xd3f93b35435d7c4c, 0xc47bc5014a1a6daf,
    0x359ab6419ca1091b, 0xc30163d203c94b62,
    0x79e0de63425dcf1d, 0x985915fc12f542e4,
    0x3e6f5b7b17b2939d, 0xa705992ceecf9c42,
    0x50c6ff782a838353, 0xa4f8bf5635246428,
    0x871b7795e136be99, 0x28e2557b59846e3f,
    0x331aeada2fe589cf, 0x3ff0d2c85def7621,
    0xfed077a756b53a9, 0xd3e8495912c62894,
    0x64712dd7abbbd95c, 0xbd8d794d96aacfb3,
    0xecf0d7a0fc5583a0, 0xf41686c49db57244,
    0x311c2875c522ced5, 0x7d633293366b828b,
    0xae5dff9c02033197, 0xd9f57f830283fdfc,
    0xd072df63c324fd7b, 0x4247cb9e59f71e6d,
    0x52d9be85f074e608, 0x67902e276c921f8b,
    0xba1cd8a3db53b6, 0x80e8a40eccd228a4,
    0x6122cd128006b2cd, 0x796b805720085f81,
    0xcbe3303674053bb0, 0xbedbfc4411068a9c,
    0xee92fb5515482d44, 0x751bdd152d4d1c4a,
    0xd262d45a78a0635d, 0x86fb897116c87c34,
    0xd45d35e6ae3d4da0, 0x8974836059cca109,
    0x2bd1a438703fc94b, 0x7b6306a34627ddcf,
    0x1a3bc84c17b1d542, 0x20caba5f1d9e4a93,
    0x547eb47b7282ee9c, 0xe99e619a4f23aa43,
    0x6405fa00e2ec94d4, 0xde83bc408dd3dd04,
    0x9624ab50b148d445, 0x3badd624dd9b0957,
    0xe54ca5d70a80e5d6, 0x5e9fcf4ccd211f4c,
    0x7647c3200069671f, 0x29ecd9f40041e073,
    0xf468107100525890, 0x7182148d4066eeb4,
    0xc6f14cd848405530, 0xb8ada00e5a506a7c,
    0xa6d90811f0e4851c, 0x908f4a166d1da663,
    0x9a598e4e043287fe, 0x40eff1e1853f29fd,
    0xd12bee59e68ef47c, 0x82bb74f8301958ce,
    0xe36a52363c1faf01, 0xdc44e6c3cb279ac1,
    0x29ab103a5ef8c0b9, 0x7415d448f6b6f0e7,
    0x111b495b3464ad21, 0xcab10dd900beec34,
    0x3d5d514f40eea742, 0xcb4a5a3112a5112,
    0x47f0e785eaba72ab, 0x59ed216765690f56,
    0x306869c13ec3532c, 0x1e414218c73a13fb,
    0xe5d1929ef90898fa, 0xdf45f746b74abf39,
    0x6b8bba8c328eb783, 0x66ea92f3f326564,
    0xc80a537b0efefebd, 0xbd06742ce95f5f36,
    0x2c48113823b73704, 0xf75a15862ca504c5,
    0x9a984d73dbe722fb, 0xc13e60d0d2e0ebba,
    0x318df905079926a8, 0xfdf17746497f7052,
    0xfeb6ea8bedefa633, 0xfe64a52ee96b8fc0,
    0x3dfdce7aa3c673b0, 0x6bea10ca65c084e,
    0x486e494fcff30a62, 0x5a89dba3c3efccfa,
    0xf89629465a75e01c, 0xf6bbb397f1135823,
    0x746aa07ded582e2c, 0xa8c2a44eb4571cdc,
    0x92f34d62616ce413, 0x77b020baf9c81d17,
    0xace1474dc1d122e, 0xd819992132456ba,
    0x10e1fff697ed6c69, 0xca8d3ffa1ef463c1,
    0xbd308ff8a6b17cb2, 0xac7cb3f6d05ddbde,
    0x6bcdf07a423aa96b, 0x86c16c98d2c953c6,
    0xe871c7bf077ba8b7, 0x11471cd764ad4972,
    0xd598e40d3dd89bcf, 0x4aff1d108d4ec2c3,
    0xcedf722a585139ba, 0xc2974eb4ee658828,
    0x733d226229feea32, 0x806357d5a3f525f,
    0xca07c2dcb0cf26f7, 0xfc89b393dd02f0b5,
    0xbbac2078d443ace2, 0xd54b944b84aa4c0d,
    0xa9e795e65d4df11, 0x4d4617b5ff4a16d5,
    0x504bced1bf8e4e45, 0xe45ec2862f71e1d6,
    0x5d767327bb4e5a4c, 0x3a6a07f8d510f86f,
    0x890489f70a55368b, 0x2b45ac74ccea842e,
    0x3b0b8bc90012929d, 0x9ce6ebb40173744,
    0xcc420a6a101d0515, 0x9fa946824a12232d,
    0x47939822dc96abf9, 0x59787e2b93bc56f7,
    0x57eb4edb3c55b65a, 0xede622920b6b23f1,
    0xe95fab368e45eced, 0x11dbcb0218ebb414,
    0xd652bdc29f26a119, 0x4be76d3346f0495f,
    0x6f70a4400c562ddb, 0xcb4ccd500f6bb952,
    0x7e2000a41346a7a7, 0x8ed400668c0c28c8,
    0x728900802f0f32fa, 0x4f2b40a03ad2ffb9,
    0xe2f610c84987bfa8, 0xdd9ca7d2df4d7c9,
    0x91503d1c79720dbb, 0x75a44c6397ce912a,
    0xc986afbe3ee11aba, 0xfbe85badce996168,
    0xfae27299423fb9c3, 0xdccd879fc967d41a,
    0x5400e987bbc1c920, 0x290123e9aab23b68,
    0xf9a0b6720aaf6521, 0xf808e40e8d5b3e69,
    0xb60b1d1230b20e04, 0xb1c6f22b5e6f48c2,
    0x1e38aeb6360b1af3, 0x25c6da63c38de1b0,
    0x579c487e5a38ad0e, 0x2d835a9df0c6d851,
    0xf8e431456cf88e65, 0x1b8e9ecb641b58ff,
    0xe272467e3d222f3f, 0x5b0ed81dcc6abb0f,
    0x98e947129fc2b4e9, 0x3f2398d747b36224,
    0x8eec7f0d19a03aad, 0x1953cf68300424ac,
    0x5fa8c3423c052dd7, 0x3792f412cb06794d,
    0xe2bbd88bbee40bd0, 0x5b6aceaeae9d0ec4,
    0xf245825a5a445275, 0xeed6e2f0f0d56712,
    0x55464dd69685606b, 0xaa97e14c3c26b886,
    0xd53dd99f4b3066a8, 0xe546a8038efe4029,
    0xde98520472bdd033, 0x963e66858f6d4440,
    0xdde7001379a44aa8, 0x5560c018580d5d52,
    0xaab8f01e6e10b4a6, 0xcab3961304ca70e8,
    0x3d607b97c5fd0d22, 0x8cb89a7db77c506a,
    0x77f3608e92adb242, 0x55f038b237591ed3,
    0x6b6c46dec52f6688, 0x2323ac4b3b3da015,
    0xabec975e0a0d081a, 0x96e7bd358c904a21,
    0x7e50d64177da2e54, 0xdde50bd1d5d0b9e9,
    0x955e4ec64b44e864, 0xbd5af13bef0b113e,
    0xecb1ad8aeacdd58e, 0x67de18eda5814af2,
    0x80eacf948770ced7, 0xa1258379a94d028d,
    0x96ee45813a04330, 0x8bca9d6e188853fc,
    0x775ea264cf55347d, 0x95364afe032a819d,
    0x3a83ddbd83f52204, 0xc4926a9672793542,
    0x75b7053c0f178293, 0x5324c68b12dd6338,
    0xd3f6fc16ebca5e03, 0x88f4bb1ca6bcf584,
    0x2b31e9e3d06c32e5, 0x3aff322e62439fcf,
    0x9befeb9fad487c2, 0x4c2ebe687989a9b3,
    0xf9d37014bf60a10, 0x538484c19ef38c94,
    0x2865a5f206b06fb9, 0xf93f87b7442e45d3,
    0xf78f69a51539d748, 0xb573440e5a884d1b,
    0x31680a88f8953030, 0xfdc20d2b36ba7c3d,
    0x3d32907604691b4c, 0xa63f9a49c2c1b10f,
    0xfcf80dc33721d53, 0xd3c36113404ea4a8,
    0x645a1cac083126e9, 0x3d70a3d70a3d70a3,
    0xcccccccccccccccc, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x0,
    0x0, 0x4000000000000000,
    0x5000000000000000, 0xa400000000000000,
    0x4d00000000000000, 0xf020000000000000,
    0x6c28000000000000, 0xc732000000000000,
    0x3c7f400000000000, 0x4b9f100000000000,
    0x1e86d40000000000, 0x1314448000000000,
    0x17d955a000000000, 0x5dcfab0800000000,
    0x5aa1cae500000000, 0xf14a3d9e40000000,
    0x6d9ccd05d0000000, 0xe4820023a2000000,
    0xdda2802c8a800000, 0xd50b2037ad200000,
    0x4526f422cc340000, 0x9670b12b7f410000,
    0x3c0cdd765f114000, 0xa5880a69fb6ac800,
    0x8eea0d047a457a00, 0x72a4904598d6d880,
    0x47a6da2b7f864750, 0x999090b65f67d924,
    0xfff4b4e3f741cf6d, 0xbff8f10e7a8921a4,
    0xaff72d52192b6a0d, 0x9bf4f8a69f764490,
    0x2f236d04753d5b4, 0x1d762422c946590,
    0x424d3ad2b7b97ef5, 0xd2e0898765a7deb2,
    0x63cc55f49f88eb2f, 0x3cbf6b71c76b25fb,
    0x8bef464e3945ef7a, 0x97758bf0e3cbb5ac,
    0x3d52eeed1cbea317, 0x4ca7aaa863ee4bdd,
    0x8fe8caa93e74ef6a, 0xb3e2fd538e122b44,
    0x60dbbca87196b616, 0xbc8955e946fe31cd,
    0x6babab6398bdbe41, 0xc696963c7eed2dd1,
    0xfc1e1de5cf543ca2, 0x3b25a55f43294bcb,
    0x49ef0eb713f39ebe, 0x6e3569326c784337,
    0x49c2c37f07965404, 0xdc33745ec97be906,
    0x69a028bb3ded71a3, 0xc40832ea0d68ce0c,
    0xf50a3fa490c30190, 0x792667c6da79e0fa,
    0x577001b891185938, 0xed4c0226b55e6f86,
    0x544f8158315b05b4, 0x696361ae3db1c721,
    0x3bc3a19cd1e38e9, 0x4ab48a04065c723,
    0x62eb0d64283f9c76, 0x3ba5d0bd324f8394,
    0xca8f44ec7ee36479, 0x7e998b13cf4e1ecb,
    0x9e3fedd8c321a67e, 0xc5cfe94ef3ea101e,
    0xbba1f1d158724a12, 0x2a8a6e45ae8edc97,
    0xf52d09d71a3293bd, 0x593c2626705f9c56,
    0x6f8b2fb00c77836c, 0xb6dfb9c0f956447,
    0x4724bd4189bd5eac, 0x58edec91ec2cb657,
    0x2f2967b66737e3ed, 0xbd79e0d20082ee74,
    0xecd8590680a3aa11, 0xe80e6f4820cc9495,
    0x3109058d147fdcdd, 0xbd4b46f0599fd415,
    0x6c9e18ac7007c91a, 0x3e2cf6bc604ddb0,
    0x84db8346b786151c, 0xe612641865679a63,
    0x4fcb7e8f3f60c07e, 0xe3be5e330f38f09d,
    0x5cadf5bfd3072cc5, 0x73d9732fc7c8f7f6,
    0x2867e7fddcdd9afa, 0xb281e1fd541501b8,
    0x1f225a7ca91a4226, 0x3375788de9b06958,
    0x52d6b1641c83ae, 0xc0678c5dbd23a49a,
    0xf840b7ba963646e0, 0xb650e5a93bc3d898,
    0xa3e51f138ab4cebe, 0xc66f336c36b10137,
    0xb80b0047445d4184, 0xa60dc059157491e5,
    0x87c89837ad68db2f, 0x29babe4598c311fb,
    0xf4296dd6fef3d67a, 0x1899e4a65f58660c,
    0x5ec05dcff72e7f8f, 0x76707543f4fa1f73,
    0x6a06494a791c53a8, 0x487db9d17636892,
    0x45a9d2845d3c42b6, 0xb8a2392ba45a9b2,
    0x8e6cac7768d7141e, 0x3207d795430cd926,
    0x7f44e6bd49e807b8, 0x5f16206c9c6209a6,
    0x36dba887c37a8c0f, 0xc2494954da2c9789,
    0xf2db9baa10b7bd6c, 0x6f92829494e5acc7,
    0xcb772339ba1f17f9, 0xff2a760414536efb,
    0xfef5138519684aba, 0x7eb258665fc25d69,
    0xef2f773ffbd97a61, 0xaafb550ffacfd8fa,
    0x95ba2a53f983cf38, 0xdd945a747bf26183,
    0x94f971119aeef9e4, 0x7a37cd5601aab85d,
    0xac62e055c10ab33a, 0x577b986b314d6009,
    0xed5a7e85fda0b80b, 0x14588f13be847307,
    0x596eb2d8ae258fc8, 0x6fca5f8ed9aef3bb,
    0x25de7bb9480d5854, 0xaf561aa79a10ae6a,
    0x1b2ba1518094da04, 0x90fb44d2f05d0842,
    0x353a1607ac744a53, 0x42889b8997915ce8,
    0x69956135febada11, 0x43fab9837e699095,
    0x94f967e45e03f4bb, 0x1d1be0eebac278f5,
    0x6462d92a69731732, 0x7d7b8f7503cfdcfe,
    0x5cda735244c3d43e, 0x3a0888136afa64a7,
    0x88aaa1845b8fdd0, 0x8aad549e57273d45,
    0x36ac54e2f678864b, 0x84576a1bb416a7dd,
    0x656d44a2a11c51d5, 0x9f644ae5a4b1b325,
    0x873d5d9f0dde1fee, 0xa90cb506d155a7ea,
    0x9a7f12442d588f2, 0xc11ed6d538aeb2f,
    0x8f1668c8a86da5fa, 0xf96e017d694487bc,
    0x37c981dcc395a9ac, 0x85bbe253f47b1417,
    0x93956d7478ccec8e, 0x387ac8d1970027b2,
    0x6997b05fcc0319e, 0x441fece3bdf81f03,
    0xd527e81cad7626c3, 0x8a71e223d8d3b074,
    0xf6872d5667844e49, 0xb428f8ac016561db,
    0xe13336d701beba52, 0xecc0024661173473,
    0x27f002d7f95d0190, 0x31ec038df7b441f4,
    0x7e67047175a15271, 0xf0062c6e984d386,
    0x52c07b78a3e60868, 0xa7709a56ccdf8a82,
    0x88a66076400bb691, 0x6acff893d00ea435,
    0x583f6b8c4124d43, 0xc3727a337a8b704a,
    0x744f18c0592e4c5c, 0x1162def06f79df73,
    0x8addcb5645ac2ba8, 0x6d953e2bd7173692,
    0xc8fa8db6ccdd0437, 0x1d9c9892400a22a2,
    0x2503beb6d00cab4b, 0x2e44ae64840fd61d,
    0x5ceaecfed289e5d2, 0x7425a83e872c5f47,
    0xd12f124e28f77719, 0x82bd6b70d99aaa6f,
    0x636cc64d1001550b, 0x3c47f7e05401aa4e,
    0x65acfaec34810a71, 0x7f1839a741a14d0d,
    0x1ede48111209a050, 0x934aed0aab460432,
    0xf81da84d5617853f, 0x36251260ab9d668e,
    0xc1d72b7c6b426019, 0xb24cf65b8612f81f,
    0xdee033f26797b627, 0x169840ef017da3b1,
    0x8e1f289560ee864e, 0xf1a6f2bab92a27e2,
    0xae10af696774b1db, 0xacca6da1e0a8ef29,
    0x17fd090a58d32af3, 0xddfc4b4cef07f5b0,
    0x4abdaf101564f98e, 0x9d6d1ad41abe37f1,
    0x84c86189216dc5ed, 0x32fd3cf5b4e49bb4,
    0x3fbc8c33221dc2a1, 0xfabaf3feaa5334a,
    0x29cb4d87f2a7400e, 0x743e20e9ef511012,
    0x914da9246b255416, 0x1ad089b6c2f7548e,
    0xa184ac2473b529b1, 0xc9e5d72d90a2741e,
    0x7e2fa67c7a658892, 0xddbb901b98feeab7,
    0x552a74227f3ea565, 0xd53a88958f87275f,
    0x8a892abaf368f137, 0x2d2b7569b0432d85,
    0x9c3b29620e29fc73, 0x8349f3ba91b47b8f,
    0x241c70a936219a73, 0xed238cd383aa0110,
    0xf4363804324a40aa, 0xb143c6053edcd0d5,
    0xdd94b7868e94050a, 0xca7cf2b4191c8326,
    0xfd1c2f611f63a3f0, 0xbc633b39673c8cec,
    0xd5be0503e085d813, 0x4b2d8644d8a74e18,
    0xddf8e7d60ed1219e, 0xcabb90e5c942b503,
    0x3d6a751f3b936243, 0xcc512670a783ad4,
    0x27fb2b80668b24c5, 0xb1f9f660802dedf6,
    0x5e7873f8a0396973, 0xdb0b487b6423e1e8,
    0x91ce1a9a3d2cda62, 0x7641a140cc7810fb,
    0xa9e904c87fcb0a9d, 0x546345fa9fbdcd44,
    0xa97c177947ad4095, 0x49ed8eabcccc485d,
    0x5c68f256bfff5a74, 0x73832eec6fff3111,
    0xc831fd53c5ff7eab, 0xba3e7ca8b77f5e55,
    0x28ce1bd2e55f35eb, 0x7980d163cf5b81b3,
    0xd7e105bcc332621f, 0x8dd9472bf3fefaa7,
    0xb14f98f6f0feb951, 0x6ed1bf9a569f33d3,
    0xa862f80ec4700c8, 0xcd27bb612758c0fa,
    0x8038d51cb897789c, 0xe0470a63e6bd56c3,
    0x1858ccfce06cac74, 0xf37801e0c43ebc8,
    0xd30560258f54e6ba, 0x47c6b82ef32a2069,
    0x4cdc331d57fa5441, 0xe0133fe4adf8e952,
    0x58180fddd97723a6, 0x570f09eaa7ea7648,};

} // namespace simdjson

#endif // SIMDJSON_JSONCHARUTILS_TABLES_H
/* end file src/jsoncharutils_tables.h */
/* simdprune_tables.h already included: #include "simdprune_tables.h" */

#if SIMDJSON_IMPLEMENTATION_ARM64
/* begin file src/arm64/implementation.cpp */
/* begin file src/arm64/begin_implementation.h */
#define SIMDJSON_IMPLEMENTATION arm64
/* arm64/implementation.h already included: #include "arm64/implementation.h" */
/* begin file src/arm64/intrinsics.h */
#ifndef SIMDJSON_ARM64_INTRINSICS_H
#define SIMDJSON_ARM64_INTRINSICS_H


// This should be the correct header whether
// you use visual studio or other compilers.
#include <arm_neon.h>

#endif //  SIMDJSON_ARM64_INTRINSICS_H
/* end file src/arm64/intrinsics.h */
/* begin file src/arm64/bitmanipulation.h */
#ifndef SIMDJSON_ARM64_BITMANIPULATION_H
#define SIMDJSON_ARM64_BITMANIPULATION_H

namespace {
namespace arm64 {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
NO_SANITIZE_UNDEFINED
simdjson_really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long ret;
  // Search the mask data from least significant bit (LSB) 
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else // SIMDJSON_REGULAR_VISUAL_STUDIO
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
}

/* result might be undefined when input_num is zero */
simdjson_really_inline int leading_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB) 
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif// SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_really_inline int count_ones(uint64_t input_num) {
   return vaddv_u8(vcnt_u8(vcreate_u8(input_num)));
}

simdjson_really_inline bool add_overflow(uint64_t value1, uint64_t value2, uint64_t *result) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  *result = value1 + value2;
  return *result < value1;
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

} // namespace arm64
} // unnamed namespace

#endif // SIMDJSON_ARM64_BITMANIPULATION_H
/* end file src/arm64/bitmanipulation.h */
/* begin file src/arm64/bitmask.h */
#ifndef SIMDJSON_ARM64_BITMASK_H
#define SIMDJSON_ARM64_BITMASK_H

namespace {
namespace arm64 {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_really_inline uint64_t prefix_xor(uint64_t bitmask) {
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

} // namespace arm64
} // namespace simdjson
SIMDJSON_UNTARGET_REGION

#endif
/* end file src/arm64/bitmask.h */
/* begin file src/arm64/simd.h */
#ifndef SIMDJSON_ARM64_SIMD_H
#define SIMDJSON_ARM64_SIMD_H

/* simdprune_tables.h already included: #include "simdprune_tables.h" */
/* arm64/bitmanipulation.h already included: #include "arm64/bitmanipulation.h" */
#include <type_traits>


namespace {
namespace arm64 {
namespace simd {

#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
namespace {
// Start of private section with Visual Studio workaround


/**
 * make_uint8x16_t initializes a SIMD register (uint8x16_t).
 * This is needed because, incredibly, the syntax uint8x16_t x = {1,2,3...}
 * is not recognized under Visual Studio! This is a workaround.
 * Using a std::initializer_list<uint8_t>  as a parameter resulted in
 * inefficient code. With the current approach, if the parameters are
 * compile-time constants,
 * GNU GCC compiles it to ldr, the same as uint8x16_t x = {1,2,3...}.
 * You should not use this function except for compile-time constants:
 * it is not efficient.
 */
simdjson_really_inline uint8x16_t make_uint8x16_t(uint8_t x1,  uint8_t x2,  uint8_t x3,  uint8_t x4,
                                         uint8_t x5,  uint8_t x6,  uint8_t x7,  uint8_t x8,
                                         uint8_t x9,  uint8_t x10, uint8_t x11, uint8_t x12,
                                         uint8_t x13, uint8_t x14, uint8_t x15, uint8_t x16) {
  // Doing a load like so end ups generating worse code.
  // uint8_t array[16] = {x1, x2, x3, x4, x5, x6, x7, x8,
  //                     x9, x10,x11,x12,x13,x14,x15,x16};
  // return vld1q_u8(array);
  uint8x16_t x{};
  // incredibly, Visual Studio does not allow x[0] = x1
  x = vsetq_lane_u8(x1, x, 0);
  x = vsetq_lane_u8(x2, x, 1);
  x = vsetq_lane_u8(x3, x, 2);
  x = vsetq_lane_u8(x4, x, 3);
  x = vsetq_lane_u8(x5, x, 4);
  x = vsetq_lane_u8(x6, x, 5);
  x = vsetq_lane_u8(x7, x, 6);
  x = vsetq_lane_u8(x8, x, 7);
  x = vsetq_lane_u8(x9, x, 8);
  x = vsetq_lane_u8(x10, x, 9);
  x = vsetq_lane_u8(x11, x, 10);
  x = vsetq_lane_u8(x12, x, 11);
  x = vsetq_lane_u8(x13, x, 12);
  x = vsetq_lane_u8(x14, x, 13);
  x = vsetq_lane_u8(x15, x, 14);
  x = vsetq_lane_u8(x16, x, 15);
  return x;
}


// We have to do the same work for make_int8x16_t
simdjson_really_inline int8x16_t make_int8x16_t(int8_t x1,  int8_t x2,  int8_t x3,  int8_t x4,
                                       int8_t x5,  int8_t x6,  int8_t x7,  int8_t x8,
                                       int8_t x9,  int8_t x10, int8_t x11, int8_t x12,
                                       int8_t x13, int8_t x14, int8_t x15, int8_t x16) {
  // Doing a load like so end ups generating worse code.
  // int8_t array[16] = {x1, x2, x3, x4, x5, x6, x7, x8,
  //                     x9, x10,x11,x12,x13,x14,x15,x16};
  // return vld1q_s8(array);
  int8x16_t x{};
  // incredibly, Visual Studio does not allow x[0] = x1
  x = vsetq_lane_s8(x1, x, 0);
  x = vsetq_lane_s8(x2, x, 1);
  x = vsetq_lane_s8(x3, x, 2);
  x = vsetq_lane_s8(x4, x, 3);
  x = vsetq_lane_s8(x5, x, 4);
  x = vsetq_lane_s8(x6, x, 5);
  x = vsetq_lane_s8(x7, x, 6);
  x = vsetq_lane_s8(x8, x, 7);
  x = vsetq_lane_s8(x9, x, 8);
  x = vsetq_lane_s8(x10, x, 9);
  x = vsetq_lane_s8(x11, x, 10);
  x = vsetq_lane_s8(x12, x, 11);
  x = vsetq_lane_s8(x13, x, 12);
  x = vsetq_lane_s8(x14, x, 13);
  x = vsetq_lane_s8(x15, x, 14);
  x = vsetq_lane_s8(x16, x, 15);
  return x;
}

// End of private section with Visual Studio workaround
} // namespace
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO


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
    simdjson_really_inline base_u8(const uint8x16_t _value) : value(_value) {}
    simdjson_really_inline operator const uint8x16_t&() const { return this->value; }
    simdjson_really_inline operator uint8x16_t&() { return this->value; }

    // Bit operations
    simdjson_really_inline simd8<T> operator|(const simd8<T> other) const { return vorrq_u8(*this, other); }
    simdjson_really_inline simd8<T> operator&(const simd8<T> other) const { return vandq_u8(*this, other); }
    simdjson_really_inline simd8<T> operator^(const simd8<T> other) const { return veorq_u8(*this, other); }
    simdjson_really_inline simd8<T> bit_andnot(const simd8<T> other) const { return vbicq_u8(*this, other); }
    simdjson_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }
    simdjson_really_inline simd8<T>& operator|=(const simd8<T> other) { auto this_cast = (simd8<T>*)this; *this_cast = *this_cast | other; return *this_cast; }
    simdjson_really_inline simd8<T>& operator&=(const simd8<T> other) { auto this_cast = (simd8<T>*)this; *this_cast = *this_cast & other; return *this_cast; }
    simdjson_really_inline simd8<T>& operator^=(const simd8<T> other) { auto this_cast = (simd8<T>*)this; *this_cast = *this_cast ^ other; return *this_cast; }

    simdjson_really_inline Mask operator==(const simd8<T> other) const { return vceqq_u8(*this, other); }

    template<int N=1>
    simdjson_really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return vextq_u8(prev_chunk, *this, 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base_u8<bool> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    static simdjson_really_inline simd8<bool> splat(bool _value) { return vmovq_n_u8(uint8_t(-(!!_value))); }

    simdjson_really_inline simd8(const uint8x16_t _value) : base_u8<bool>(_value) {}
    // False constructor
    simdjson_really_inline simd8() : simd8(vdupq_n_u8(0)) {}
    // Splat constructor
    simdjson_really_inline simd8(bool _value) : simd8(splat(_value)) {}

    // We return uint32_t instead of uint16_t because that seems to be more efficient for most
    // purposes (cutting it down to uint16_t costs performance in some compilers).
    simdjson_really_inline uint32_t to_bitmask() const {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
      const uint8x16_t bit_mask =  make_uint8x16_t(0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                                   0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80);
#else
      const uint8x16_t bit_mask =  {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                    0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
#endif
      auto minput = *this & bit_mask;
      uint8x16_t tmp = vpaddq_u8(minput, minput);
      tmp = vpaddq_u8(tmp, tmp);
      tmp = vpaddq_u8(tmp, tmp);
      return vgetq_lane_u16(vreinterpretq_u16_u8(tmp), 0);
    }
    simdjson_really_inline bool any() const { return vmaxvq_u8(*this) != 0; }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base_u8<uint8_t> {
    static simdjson_really_inline uint8x16_t splat(uint8_t _value) { return vmovq_n_u8(_value); }
    static simdjson_really_inline uint8x16_t zero() { return vdupq_n_u8(0); }
    static simdjson_really_inline uint8x16_t load(const uint8_t* values) { return vld1q_u8(values); }

    simdjson_really_inline simd8(const uint8x16_t _value) : base_u8<uint8_t>(_value) {}
    // Zero constructor
    simdjson_really_inline simd8() : simd8(zero()) {}
    // Array constructor
    simdjson_really_inline simd8(const uint8_t values[16]) : simd8(load(values)) {}
    // Splat constructor
    simdjson_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Member-by-member initialization
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
    simdjson_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(make_uint8x16_t(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
#else
    simdjson_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(uint8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
#endif

    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Store to array
    simdjson_really_inline void store(uint8_t dst[16]) const { return vst1q_u8(dst, *this); }

    // Saturated math
    simdjson_really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return vqaddq_u8(*this, other); }
    simdjson_really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return vqsubq_u8(*this, other); }

    // Addition/subtraction are the same for signed and unsigned
    simdjson_really_inline simd8<uint8_t> operator+(const simd8<uint8_t> other) const { return vaddq_u8(*this, other); }
    simdjson_really_inline simd8<uint8_t> operator-(const simd8<uint8_t> other) const { return vsubq_u8(*this, other); }
    simdjson_really_inline simd8<uint8_t>& operator+=(const simd8<uint8_t> other) { *this = *this + other; return *this; }
    simdjson_really_inline simd8<uint8_t>& operator-=(const simd8<uint8_t> other) { *this = *this - other; return *this; }

    // Order-specific operations
    simdjson_really_inline uint8_t max() const { return vmaxvq_u8(*this); }
    simdjson_really_inline uint8_t min() const { return vminvq_u8(*this); }
    simdjson_really_inline simd8<uint8_t> max(const simd8<uint8_t> other) const { return vmaxq_u8(*this, other); }
    simdjson_really_inline simd8<uint8_t> min(const simd8<uint8_t> other) const { return vminq_u8(*this, other); }
    simdjson_really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return vcleq_u8(*this, other); }
    simdjson_really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return vcgeq_u8(*this, other); }
    simdjson_really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return vcltq_u8(*this, other); }
    simdjson_really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return vcgtq_u8(*this, other); }
    // Same as >, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    simdjson_really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this > other); }
    // Same as <, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    simdjson_really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this < other); }

    // Bit-specific operations
    simdjson_really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return vtstq_u8(*this, bits); }
    simdjson_really_inline bool any_bits_set_anywhere() const { return this->max() != 0; }
    simdjson_really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return (*this & bits).any_bits_set_anywhere(); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shr() const { return vshrq_n_u8(*this, N); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shl() const { return vshlq_n_u8(*this, N); }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
    }


    // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted as a bitset).
    // Passing a 0 value for mask would be equivalent to writing out every byte to output.
    // Only the first 16 - count_ones(mask) bytes of the result are significant but 16 bytes
    // get written.
    // Design consideration: it seems like a function with the
    // signature simd8<L> compress(uint16_t mask) would be
    // sensible, but the AVX ISA makes this kind of approach difficult.
    template<typename L>
    simdjson_really_inline void compress(uint16_t mask, L * output) const {
      // this particular implementation was inspired by work done by @animetosho
      // we do it in two steps, first 8 bytes and then second 8 bytes
      uint8_t mask1 = uint8_t(mask); // least significant 8 bits
      uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
      // next line just loads the 64-bit values thintable_epi8[mask1] and
      // thintable_epi8[mask2] into a 128-bit register, using only
      // two instructions on most compilers.
      uint64x2_t shufmask64 = {thintable_epi8[mask1], thintable_epi8[mask2]};
      uint8x16_t shufmask = vreinterpretq_u8_u64(shufmask64);
      // we increment by 0x08 the second half of the mask
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
      uint8x16_t inc = make_uint8x16_t(0, 0, 0, 0, 0, 0, 0, 0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08);
#else
      uint8x16_t inc = {0, 0, 0, 0, 0, 0, 0, 0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};
#endif
      shufmask = vaddq_u8(shufmask, inc);
      // this is the version "nearly pruned"
      uint8x16_t pruned = vqtbl1q_u8(*this, shufmask);
      // we still need to put the two halves together.
      // we compute the popcount of the first half:
      int pop1 = BitsSetTable256mul2[mask1];
      // then load the corresponding mask, what it does is to write
      // only the first pop1 bytes from the first 8 bytes, and then
      // it fills in with the bytes from the second 8 bytes + some filling
      // at the end.
      uint8x16_t compactmask = vld1q_u8((const uint8_t *)(pshufb_combine_table + pop1 * 8));
      uint8x16_t answer = vqtbl1q_u8(pruned, compactmask);
      vst1q_u8((uint8_t*) output, answer);
    }

    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(
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
    simdjson_really_inline simd8<uint8_t> apply_lookup_16_to(const simd8<T> original) {
      return vqtbl1q_u8(*this, simd8<uint8_t>(original));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> {
    int8x16_t value;

    static simdjson_really_inline simd8<int8_t> splat(int8_t _value) { return vmovq_n_s8(_value); }
    static simdjson_really_inline simd8<int8_t> zero() { return vdupq_n_s8(0); }
    static simdjson_really_inline simd8<int8_t> load(const int8_t values[16]) { return vld1q_s8(values); }

    // Conversion from/to SIMD register
    simdjson_really_inline simd8(const int8x16_t _value) : value{_value} {}
    simdjson_really_inline operator const int8x16_t&() const { return this->value; }
    simdjson_really_inline operator int8x16_t&() { return this->value; }

    // Zero constructor
    simdjson_really_inline simd8() : simd8(zero()) {}
    // Splat constructor
    simdjson_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const int8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
    simdjson_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3, int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(make_int8x16_t(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
#else
    simdjson_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3, int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(int8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
#endif
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Store to array
    simdjson_really_inline void store(int8_t dst[16]) const { return vst1q_s8(dst, *this); }

    // Explicit conversion to/from unsigned
    //
    // Under Visual Studio/ARM64 uint8x16_t and int8x16_t are apparently the same type.
    // In theory, we could check this occurence with std::same_as and std::enabled_if but it is C++14
    // and relatively ugly and hard to read.
#ifndef SIMDJSON_REGULAR_VISUAL_STUDIO
    simdjson_really_inline explicit simd8(const uint8x16_t other): simd8(vreinterpretq_s8_u8(other)) {}
#endif
    simdjson_really_inline explicit operator simd8<uint8_t>() const { return vreinterpretq_u8_s8(this->value); }

    // Math
    simdjson_really_inline simd8<int8_t> operator+(const simd8<int8_t> other) const { return vaddq_s8(*this, other); }
    simdjson_really_inline simd8<int8_t> operator-(const simd8<int8_t> other) const { return vsubq_s8(*this, other); }
    simdjson_really_inline simd8<int8_t>& operator+=(const simd8<int8_t> other) { *this = *this + other; return *this; }
    simdjson_really_inline simd8<int8_t>& operator-=(const simd8<int8_t> other) { *this = *this - other; return *this; }

    // Order-sensitive comparisons
    simdjson_really_inline simd8<int8_t> max(const simd8<int8_t> other) const { return vmaxq_s8(*this, other); }
    simdjson_really_inline simd8<int8_t> min(const simd8<int8_t> other) const { return vminq_s8(*this, other); }
    simdjson_really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return vcgtq_s8(*this, other); }
    simdjson_really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return vcltq_s8(*this, other); }
    simdjson_really_inline simd8<bool> operator==(const simd8<int8_t> other) const { return vceqq_s8(*this, other); }

    template<int N=1>
    simdjson_really_inline simd8<int8_t> prev(const simd8<int8_t> prev_chunk) const {
      return vextq_s8(prev_chunk, *this, 16 - N);
    }

    // Perform a lookup assuming no value is larger than 16
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
    }
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(
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
    simdjson_really_inline simd8<int8_t> apply_lookup_16_to(const simd8<T> original) {
      return vqtbl1q_s8(*this, simd8<uint8_t>(original));
    }
  };

  template<typename T>
  struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 4, "ARM kernel should use four registers per 64-byte block.");
    const simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T>& o) = delete; // no copy allowed
    simd8x64<T>& operator=(const simd8<T> other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed
    
    simdjson_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2, const simd8<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    simdjson_really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+16), simd8<T>::load(ptr+32), simd8<T>::load(ptr+48)} {}

    simdjson_really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store(ptr+sizeof(simd8<T>)*3);
    }

    simdjson_really_inline simd8<T> reduce_or() const {
      return (this->chunks[0] | this->chunks[1]) | (this->chunks[2] | this->chunks[3]);
    }


    simdjson_really_inline void compress(uint64_t mask, T * output) const {
      this->chunks[0].compress(uint16_t(mask), output);
      this->chunks[1].compress(uint16_t(mask >> 16), output + 16 - count_ones(mask & 0xFFFF));
      this->chunks[2].compress(uint16_t(mask >> 32), output + 32 - count_ones(mask & 0xFFFFFFFF));
      this->chunks[3].compress(uint16_t(mask >> 48), output + 48 - count_ones(mask & 0xFFFFFFFFFFFF));
    }

    simdjson_really_inline uint64_t to_bitmask() const {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
      const uint8x16_t bit_mask = make_uint8x16_t(
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      );
#else
      const uint8x16_t bit_mask = {
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      };
#endif
      // Add each of the elements next to each other, successively, to stuff each 8 byte mask into one.
      uint8x16_t sum0 = vpaddq_u8(this->chunks[0] & bit_mask, this->chunks[1] & bit_mask);
      uint8x16_t sum1 = vpaddq_u8(this->chunks[2] & bit_mask, this->chunks[3] & bit_mask);
      sum0 = vpaddq_u8(sum0, sum1);
      sum0 = vpaddq_u8(sum0, sum0);
      return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
    }

    simdjson_really_inline simd8x64<T> bit_or(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return simd8x64<T>(
        this->chunks[0] | mask,
        this->chunks[1] | mask,
        this->chunks[2] | mask,
        this->chunks[3] | mask
      );
    }

    simdjson_really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] == mask,
        this->chunks[1] == mask,
        this->chunks[2] == mask,
        this->chunks[3] == mask
      ).to_bitmask();
    }

    simdjson_really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] <= mask,
        this->chunks[1] <= mask,
        this->chunks[2] <= mask,
        this->chunks[3] <= mask
      ).to_bitmask();
    }
  }; // struct simd8x64<T>

} // namespace simd
} // namespace arm64
} // unnamed namespace

#endif // SIMDJSON_ARM64_SIMD_H
/* end file src/arm64/simd.h */
/* end file src/arm64/simd.h */
/* begin file src/arm64/dom_parser_implementation.h */
#ifndef SIMDJSON_ARM64_DOM_PARSER_IMPLEMENTATION_H
#define SIMDJSON_ARM64_DOM_PARSER_IMPLEMENTATION_H

/* begin file src/generic/dom_parser_implementation.h */
/* isadetection.h already included: #include "isadetection.h" */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  simdjson_really_inline dom_parser_implementation();
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation & operator=(const dom_parser_implementation &) = delete;

  SIMDJSON_WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  SIMDJSON_WARN_UNUSED error_code check_for_unclosed_array() noexcept;
  SIMDJSON_WARN_UNUSED error_code stage2(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage2_next(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_capacity(size_t capacity) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_max_depth(size_t max_depth) noexcept final;
};

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/generic/stage1/allocate.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {
namespace allocate {

//
// Allocates stage 1 internal state and outputs in the parser
//
simdjson_really_inline error_code set_capacity(internal::dom_parser_implementation &parser, size_t capacity) {
  size_t max_structures = SIMDJSON_ROUNDUP_N(capacity, 64) + 2 + 7;
  parser.structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] );
  if (!parser.structural_indexes) { return MEMALLOC; }
  parser.structural_indexes[0] = 0;
  parser.n_structural_indexes = 0;
  return SUCCESS;
}

} // namespace allocate
} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/allocate.h */
/* begin file src/generic/stage2/allocate.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace allocate {

//
// Allocates stage 2 internal state and outputs in the parser
//
simdjson_really_inline error_code set_max_depth(dom_parser_implementation &parser, size_t max_depth) {
  parser.open_containers.reset(new (std::nothrow) open_container[max_depth]);
  parser.is_array.reset(new (std::nothrow) bool[max_depth]);

  if (!parser.is_array || !parser.open_containers) {
    return MEMALLOC;
  }
  return SUCCESS;
}

} // namespace allocate
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/allocate.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

simdjson_really_inline dom_parser_implementation::dom_parser_implementation() {}

// Leaving these here so they can be inlined if so desired
SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  error_code err = stage1::allocate::set_capacity(*this, capacity);
  if (err) { _capacity = 0; return err; }
  _capacity = capacity;
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  error_code err = stage2::allocate::set_max_depth(*this, max_depth);
  if (err) { _max_depth = 0; return err; }
  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/allocate.h */

#endif // SIMDJSON_ARM64_DOM_PARSER_IMPLEMENTATION_H
/* end file src/generic/stage2/allocate.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

SIMDJSON_WARN_UNUSED error_code implementation::create_dom_parser_implementation(
  size_t capacity,
  size_t max_depth,
  std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  dst.reset( new (std::nothrow) dom_parser_implementation() );
  if (!dst) { return MEMALLOC; }
  dst->set_capacity(capacity);
  dst->set_max_depth(max_depth);
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/arm64/end_implementation.h */
#undef SIMDJSON_IMPLEMENTATION
/* end file src/arm64/end_implementation.h */
/* end file src/arm64/end_implementation.h */
/* begin file src/arm64/dom_parser_implementation.cpp */
/* begin file src/arm64/begin_implementation.h */
#define SIMDJSON_IMPLEMENTATION arm64
/* arm64/implementation.h already included: #include "arm64/implementation.h" */
/* arm64/intrinsics.h already included: #include "arm64/intrinsics.h" */
/* arm64/bitmanipulation.h already included: #include "arm64/bitmanipulation.h" */
/* arm64/bitmask.h already included: #include "arm64/bitmask.h" */
/* arm64/simd.h already included: #include "arm64/simd.h" */
/* end file src/arm64/begin_implementation.h */
/* arm64/dom_parser_implementation.h already included: #include "arm64/dom_parser_implementation.h" */
/* begin file src/generic/stage2/jsoncharutils.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_really_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return structural_or_whitespace_negated[c];
}

simdjson_really_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = digit_to_val32[630 + src[0]];
  uint32_t v2 = digit_to_val32[420 + src[1]];
  uint32_t v3 = digit_to_val32[210 + src[2]];
  uint32_t v4 = digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_really_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#ifdef SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_really_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_really_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

simdjson_really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
#ifdef _M_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // _M_ARM64
#else // defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
  __uint128_t r = ((__uint128_t)value1) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/jsoncharutils.h */

//
// Stage 1
//
namespace {
namespace SIMDJSON_IMPLEMENTATION {

using namespace simd;

struct json_character_block {
  static simdjson_really_inline json_character_block classify(const simd::simd8x64<uint8_t>& in);

  simdjson_really_inline uint64_t whitespace() const { return _whitespace; }
  simdjson_really_inline uint64_t op() const { return _op; }
  simdjson_really_inline uint64_t scalar() { return ~(op() | whitespace()); }

  uint64_t _whitespace;
  uint64_t _op;
};

simdjson_really_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t>& in) {
  // Functional programming causes trouble with Visual Studio.
  // Keeping this version in comments since it is much nicer:
  // auto v = in.map<uint8_t>([&](simd8<uint8_t> chunk) {
  //  auto nib_lo = chunk & 0xf;
  //  auto nib_hi = chunk.shr<4>();
  //  auto shuf_lo = nib_lo.lookup_16<uint8_t>(16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
  //  auto shuf_hi = nib_hi.lookup_16<uint8_t>(8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0);
  //  return shuf_lo & shuf_hi;
  // });
  const simd8<uint8_t> table1(16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
  const simd8<uint8_t> table2(8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0);

  simd8x64<uint8_t> v(
     (in.chunks[0] & 0xf).lookup_16(table1) & (in.chunks[0].shr<4>()).lookup_16(table2),
     (in.chunks[1] & 0xf).lookup_16(table1) & (in.chunks[1].shr<4>()).lookup_16(table2),
     (in.chunks[2] & 0xf).lookup_16(table1) & (in.chunks[2].shr<4>()).lookup_16(table2),
     (in.chunks[3] & 0xf).lookup_16(table1) & (in.chunks[3].shr<4>()).lookup_16(table2)
  );


  // We compute whitespace and op separately. If the code later only use one or the
  // other, given the fact that all functions are aggressively inlined, we can
  // hope that useless computations will be omitted. This is namely case when
  // minifying (we only need whitespace). *However* if we only need spaces,
  // it is likely that we will still compute 'v' above with two lookup_16: one
  // could do it a bit cheaper. This is in contrast with the x64 implementations
  // where we can, efficiently, do the white space and structural matching
  // separately. One reason for this difference is that on ARM NEON, the table
  // lookups either zero or leave unchanged the characters exceeding 0xF whereas
  // on x64, the equivalent instruction (pshufb) automatically applies a mask,
  // ignoring the 4 most significant bits. Thus the x64 implementation is
  // optimized differently. This being said, if you use this code strictly
  // just for minification (or just to identify the structural characters),
  // there is a small untaken optimization opportunity here. We deliberately
  // do not pick it up.

  uint64_t op = simd8x64<bool>(
        v.chunks[0].any_bits_set(0x7),
        v.chunks[1].any_bits_set(0x7),
        v.chunks[2].any_bits_set(0x7),
        v.chunks[3].any_bits_set(0x7)
  ).to_bitmask();

  uint64_t whitespace = simd8x64<bool>(
        v.chunks[0].any_bits_set(0x18),
        v.chunks[1].any_bits_set(0x18),
        v.chunks[2].any_bits_set(0x18),
        v.chunks[3].any_bits_set(0x18)
  ).to_bitmask();

  return { whitespace, op };
}

simdjson_really_inline bool is_ascii(const simd8x64<uint8_t>& input) {
    simd8<uint8_t> bits = input.reduce_or();
    return bits.max() < 0b10000000u;
}

SIMDJSON_UNUSED simdjson_really_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
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

simdjson_really_inline simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    simd8<bool> is_third_byte  = prev2 >= uint8_t(0b11100000u);
    simd8<bool> is_fourth_byte = prev3 >= uint8_t(0b11110000u);
    return is_third_byte ^ is_fourth_byte;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/generic/stage1/utf8_lookup4_algorithm.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace utf8_validation {

using namespace simd;

  simdjson_really_inline simd8<uint8_t> check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1) {
// Bit 0 = Too Short (lead byte/ASCII followed by lead byte/ASCII)
// Bit 1 = Too Long (ASCII followed by continuation)
// Bit 2 = Overlong 3-byte
// Bit 4 = Surrogate
// Bit 5 = Overlong 2-byte
// Bit 7 = Two Continuations
    constexpr const uint8_t TOO_SHORT   = 1<<0; // 11______ 0_______
                                                // 11______ 11______
    constexpr const uint8_t TOO_LONG    = 1<<1; // 0_______ 10______
    constexpr const uint8_t OVERLONG_3  = 1<<2; // 11100000 100_____
    constexpr const uint8_t SURROGATE   = 1<<4; // 11101101 101_____
    constexpr const uint8_t OVERLONG_2  = 1<<5; // 1100000_ 10______
    constexpr const uint8_t TWO_CONTS   = 1<<7; // 10______ 10______
    constexpr const uint8_t TOO_LARGE   = 1<<3; // 11110100 1001____
                                                // 11110100 101_____
                                                // 11110101 1001____
                                                // 11110101 101_____
                                                // 1111011_ 1001____
                                                // 1111011_ 101_____
                                                // 11111___ 1001____
                                                // 11111___ 101_____
    constexpr const uint8_t TOO_LARGE_1000 = 1<<6;
                                                // 11110101 1000____
                                                // 1111011_ 1000____
                                                // 11111___ 1000____
    constexpr const uint8_t OVERLONG_4  = 1<<6; // 11110000 1000____

    const simd8<uint8_t> byte_1_high = prev1.shr<4>().lookup_16<uint8_t>(
      // 0_______ ________ <ASCII in byte 1>
      TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
      TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
      // 10______ ________ <continuation in byte 1>
      TWO_CONTS, TWO_CONTS, TWO_CONTS, TWO_CONTS,
      // 1100____ ________ <two byte lead in byte 1>
      TOO_SHORT | OVERLONG_2,
      // 1101____ ________ <two byte lead in byte 1>
      TOO_SHORT,
      // 1110____ ________ <three byte lead in byte 1>
      TOO_SHORT | OVERLONG_3 | SURROGATE,
      // 1111____ ________ <four+ byte lead in byte 1>
      TOO_SHORT | TOO_LARGE | TOO_LARGE_1000 | OVERLONG_4
    );
    constexpr const uint8_t CARRY = TOO_SHORT | TOO_LONG | TWO_CONTS; // These all have ____ in byte 1 .
    const simd8<uint8_t> byte_1_low = (prev1 & 0x0F).lookup_16<uint8_t>(
      // ____0000 ________
      CARRY | OVERLONG_3 | OVERLONG_2 | OVERLONG_4,
      // ____0001 ________
      CARRY | OVERLONG_2,
      // ____001_ ________
      CARRY,
      CARRY,

      // ____0100 ________
      CARRY | TOO_LARGE,
      // ____0101 ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      // ____011_ ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,

      // ____1___ ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      // ____1101 ________
      CARRY | TOO_LARGE | TOO_LARGE_1000 | SURROGATE,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000
    );
    const simd8<uint8_t> byte_2_high = input.shr<4>().lookup_16<uint8_t>(
      // ________ 0_______ <ASCII in byte 2>
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,

      // ________ 1000____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE_1000 | OVERLONG_4,
      // ________ 1001____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE,
      // ________ 101_____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,

      // ________ 11______
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT
    );
    return (byte_1_high & byte_1_low & byte_2_high);
  }
  simdjson_really_inline simd8<uint8_t> check_multibyte_lengths(const simd8<uint8_t> input,
      const simd8<uint8_t> prev_input, const simd8<uint8_t> sc) {
    simd8<uint8_t> prev2 = input.prev<2>(prev_input);
    simd8<uint8_t> prev3 = input.prev<3>(prev_input);
    simd8<uint8_t> must23 = simd8<uint8_t>(must_be_2_3_continuation(prev2, prev3));
    simd8<uint8_t> must23_80 = must23 & uint8_t(0x80);
    return must23_80 ^ sc;
  }

  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  simdjson_really_inline simd8<uint8_t> is_incomplete(const simd8<uint8_t> input) {
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
    simdjson_really_inline void check_utf8_bytes(const simd8<uint8_t> input, const simd8<uint8_t> prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      simd8<uint8_t> prev1 = input.prev<1>(prev_input);
      simd8<uint8_t> sc = check_special_cases(input, prev1);
      this->error |= check_multibyte_lengths(input, prev_input, sc);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short.
    simdjson_really_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error |= this->prev_incomplete;
    }

    simdjson_really_inline void check_next_input(const simd8x64<uint8_t>& input) {
      if(simdjson_likely(is_ascii(input))) {
        this->error |= this->prev_incomplete;
      } else {
        // you might think that a for-loop would work, but under Visual Studio, it is not good enough.
        static_assert((simd8x64<uint8_t>::NUM_CHUNKS == 2) || (simd8x64<uint8_t>::NUM_CHUNKS == 4),
            "We support either two or four chunks per 64-byte block.");
        if(simd8x64<uint8_t>::NUM_CHUNKS == 2) {
          this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
          this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
        } else if(simd8x64<uint8_t>::NUM_CHUNKS == 4) {
          this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
          this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
          this->check_utf8_bytes(input.chunks[2], input.chunks[1]);
          this->check_utf8_bytes(input.chunks[3], input.chunks[2]);
        }
        this->prev_incomplete = is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1]);
        this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1];

      }
    }

    simdjson_really_inline error_code errors() {
      return this->error.any_bits_set_anywhere() ? error_code::UTF8_ERROR : error_code::SUCCESS;
    }

  }; // struct utf8_checker
} // namespace utf8_validation

using utf8_validation::utf8_checker;

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/utf8_lookup4_algorithm.h */
/* begin file src/generic/stage1/json_structural_indexer.h */
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

/* begin file src/generic/stage1/buf_block_reader.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {

// Walks through a buffer in block-sized increments, loading the last part with spaces
template<size_t STEP_SIZE>
struct buf_block_reader {
public:
  simdjson_really_inline buf_block_reader(const uint8_t *_buf, size_t _len);
  simdjson_really_inline size_t block_index();
  simdjson_really_inline bool has_full_block() const;
  simdjson_really_inline const uint8_t *full_block() const;
  /**
   * Get the last block, padded with spaces.
   *
   * There will always be a last block, with at least 1 byte, unless len == 0 (in which case this
   * function fills the buffer with spaces and returns 0. In particular, if len == STEP_SIZE there
   * will be 0 full_blocks and 1 remainder block with STEP_SIZE bytes and no spaces for padding.
   *
   * @return the number of effective characters in the last block.
   */
  simdjson_really_inline size_t get_remainder(uint8_t *dst) const;
  simdjson_really_inline void advance();
private:
  const uint8_t *buf;
  const size_t len;
  const size_t lenminusstep;
  size_t idx;
};

// Routines to print masks and text for debugging bitmask operations
SIMDJSON_UNUSED static char * format_input_text_64(const uint8_t *text) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    buf[i] = int8_t(text[i]) < ' ' ? '_' : int8_t(text[i]);
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

// Routines to print masks and text for debugging bitmask operations
SIMDJSON_UNUSED static char * format_input_text(const simd8x64<uint8_t>& in) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  in.store((uint8_t*)buf);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    if (buf[i] < ' ') { buf[i] = '_'; }
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

SIMDJSON_UNUSED static char * format_mask(uint64_t mask) {
  static char *buf = (char*)malloc(64 + 1);
  for (size_t i=0; i<64; i++) {
    buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
  }
  buf[64] = '\0';
  return buf;
}

template<size_t STEP_SIZE>
simdjson_really_inline buf_block_reader<STEP_SIZE>::buf_block_reader(const uint8_t *_buf, size_t _len) : buf{_buf}, len{_len}, lenminusstep{len < STEP_SIZE ? 0 : len - STEP_SIZE}, idx{0} {}

template<size_t STEP_SIZE>
simdjson_really_inline size_t buf_block_reader<STEP_SIZE>::block_index() { return idx; }

template<size_t STEP_SIZE>
simdjson_really_inline bool buf_block_reader<STEP_SIZE>::has_full_block() const {
  return idx < lenminusstep;
}

template<size_t STEP_SIZE>
simdjson_really_inline const uint8_t *buf_block_reader<STEP_SIZE>::full_block() const {
  return &buf[idx];
}

template<size_t STEP_SIZE>
simdjson_really_inline size_t buf_block_reader<STEP_SIZE>::get_remainder(uint8_t *dst) const {
  memset(dst, 0x20, STEP_SIZE); // memset STEP_SIZE because it's more efficient to write out 8 or 16 bytes at once.
  memcpy(dst, buf + idx, len - idx);
  return len - idx;
}

template<size_t STEP_SIZE>
simdjson_really_inline void buf_block_reader<STEP_SIZE>::advance() {
  idx += STEP_SIZE;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/buf_block_reader.h */
/* begin file src/generic/stage1/json_string_scanner.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

struct json_string_block {
  // Escaped characters (characters following an escape() character)
  simdjson_really_inline uint64_t escaped() const { return _escaped; }
  // Escape characters (backslashes that are not escaped--i.e. in \\, includes only the first \)
  simdjson_really_inline uint64_t escape() const { return _backslash & ~_escaped; }
  // Real (non-backslashed) quotes
  simdjson_really_inline uint64_t quote() const { return _quote; }
  // Start quotes of strings
  simdjson_really_inline uint64_t string_start() const { return _quote & _in_string; }
  // End quotes of strings
  simdjson_really_inline uint64_t string_end() const { return _quote & ~_in_string; }
  // Only characters inside the string (not including the quotes)
  simdjson_really_inline uint64_t string_content() const { return _in_string & ~_quote; }
  // Return a mask of whether the given characters are inside a string (only works on non-quotes)
  simdjson_really_inline uint64_t non_quote_inside_string(uint64_t mask) const { return mask & _in_string; }
  // Return a mask of whether the given characters are inside a string (only works on non-quotes)
  simdjson_really_inline uint64_t non_quote_outside_string(uint64_t mask) const { return mask & ~_in_string; }
  // Tail of string (everything except the start quote)
  simdjson_really_inline uint64_t string_tail() const { return _in_string ^ _quote; }

  // backslash characters
  uint64_t _backslash;
  // escaped characters (backslashed--does not include the hex characters after \u)
  uint64_t _escaped;
  // real quotes (non-backslashed ones)
  uint64_t _quote;
  // string characters (includes start quote but not end quote)
  uint64_t _in_string;
};

// Scans blocks for string characters, storing the state necessary to do so
class json_string_scanner {
public:
  simdjson_really_inline json_string_block next(const simd::simd8x64<uint8_t>& in);
  simdjson_really_inline error_code finish(bool streaming);

private:
  // Intended to be defined by the implementation
  simdjson_really_inline uint64_t find_escaped(uint64_t escape);
  simdjson_really_inline uint64_t find_escaped_branchless(uint64_t escape);

  // Whether the last iteration was still inside a string (all 1's = true, all 0's = false).
  uint64_t prev_in_string = 0ULL;
  // Whether the first character of the next iteration is escaped.
  uint64_t prev_escaped = 0ULL;
};

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
simdjson_really_inline uint64_t json_string_scanner::find_escaped_branchless(uint64_t backslash) {
  // If there was overflow, pretend the first character isn't a backslash
  backslash &= ~prev_escaped;
  uint64_t follows_escape = backslash << 1 | prev_escaped;

  // Get sequences starting on even bits by clearing out the odd series using +
  const uint64_t even_bits = 0x5555555555555555ULL;
  uint64_t odd_sequence_starts = backslash & ~even_bits & ~follows_escape;
  uint64_t sequences_starting_on_even_bits;
  prev_escaped = add_overflow(odd_sequence_starts, backslash, &sequences_starting_on_even_bits);
  uint64_t invert_mask = sequences_starting_on_even_bits << 1; // The mask we want to return is the *escaped* bits, not escapes.

  // Mask every other backslashed character as an escaped character
  // Flip the mask for sequences that start on even bits, to correct them
  return (even_bits ^ invert_mask) & follows_escape;
}

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
simdjson_really_inline json_string_block json_string_scanner::next(const simd::simd8x64<uint8_t>& in) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = find_escaped(backslash);
  const uint64_t quote = in.eq('"') & ~escaped;

  //
  // prefix_xor flips on bits inside the string (and flips off the end quote).
  //
  // Then we xor with prev_in_string: if we were in a string already, its effect is flipped
  // (characters inside strings are outside, and characters outside strings are inside).
  //
  const uint64_t in_string = prefix_xor(quote) ^ prev_in_string;

  //
  // Check if we're still in a string at the end of the box so the next block will know
  //
  // right shift of a signed value expected to be well-defined and standard
  // compliant as of C++20, John Regher from Utah U. says this is fine code
  //
  prev_in_string = uint64_t(static_cast<int64_t>(in_string) >> 63);

  // Use ^ to turn the beginning quote off, and the end quote on.
  return {
    backslash,
    escaped,
    quote,
    in_string
  };
}

simdjson_really_inline error_code json_string_scanner::finish(bool streaming) {
  if (prev_in_string and (not streaming)) {
    return UNCLOSED_STRING;
  }
  return SUCCESS;
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/json_string_scanner.h */
/* begin file src/generic/stage1/json_scanner.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

/**
 * A block of scanned json, with information on operators and scalars.
 *
 * We seek to identify pseudo-structural characters. Anything that is inside
 * a string must be omitted (hence  & ~_string.string_tail()).
 * Otherwise, pseudo-structural characters come in two forms.
 * 1. We have the structural characters ([,],{,},:, comma). The 
 *    term 'structural character' is from the JSON RFC.
 * 2. We have the 'scalar pseudo-structural characters'.
 *    Scalars are quotes, and any character except structural characters and white space.
 *
 * To identify the scalar pseudo-structural characters, we must look at what comes
 * before them: it must be a space, a quote or a structural characters.
 * Starting with simdjson v0.3, we identify them by
 * negation: we identify everything that is followed by a non-quote scalar,
 * and we negate that. Whatever remains must be a 'scalar pseudo-structural character'.
 */
struct json_block {
public:
  /**
   * The start of structurals.
   * In simdjson prior to v0.3, these were called the pseudo-structural characters.
   **/
  simdjson_really_inline uint64_t structural_start() { return potential_structural_start() & ~_string.string_tail(); }
  /** All JSON whitespace (i.e. not in a string) */
  simdjson_really_inline uint64_t whitespace() { return non_quote_outside_string(_characters.whitespace()); }

  // Helpers

  /** Whether the given characters are inside a string (only works on non-quotes) */
  simdjson_really_inline uint64_t non_quote_inside_string(uint64_t mask) { return _string.non_quote_inside_string(mask); }
  /** Whether the given characters are outside a string (only works on non-quotes) */
  simdjson_really_inline uint64_t non_quote_outside_string(uint64_t mask) { return _string.non_quote_outside_string(mask); }

  // string and escape characters
  json_string_block _string;
  // whitespace, structural characters ('operators'), scalars
  json_character_block _characters;
  // whether the previous character was a scalar
  uint64_t _follows_potential_nonquote_scalar;
private:
  // Potential structurals (i.e. disregarding strings)

  /**
   * structural elements ([,],{,},:, comma) plus scalar starts like 123, true and "abc".
   * They may reside inside a string.
   **/
  simdjson_really_inline uint64_t potential_structural_start() { return _characters.op() | potential_scalar_start(); }
  /**
   * The start of non-operator runs, like 123, true and "abc".
   * It main reside inside a string.
   **/
  simdjson_really_inline uint64_t potential_scalar_start() {
    // The term "scalar" refers to anything except structural characters and white space
    // (so letters, numbers, quotes).
    // Whenever it is preceded by something that is not a structural element ({,},[,],:, ") nor a white-space
    // then we know that it is irrelevant structurally.
    return _characters.scalar() & ~follows_potential_scalar();
  }
  /**
   * Whether the given character is immediately after a non-operator like 123, true.
   * The characters following a quote are not included.
   */
  simdjson_really_inline uint64_t follows_potential_scalar() {
    // _follows_potential_nonquote_scalar: is defined as marking any character that follows a character
    // that is not a structural element ({,},[,],:, comma) nor a quote (") and that is not a
    // white space.
    // It is understood that within quoted region, anything at all could be marked (irrelevant).
    return _follows_potential_nonquote_scalar;
  }
};

/**
 * Scans JSON for important bits: structural characters or 'operators', strings, and scalars.
 *
 * The scanner starts by calculating two distinct things:
 * - string characters (taking \" into account)
 * - structural characters or 'operators' ([]{},:, comma)
 *   and scalars (runs of non-operators like 123, true and "abc")
 *
 * To minimize data dependency (a key component of the scanner's speed), it finds these in parallel:
 * in particular, the operator/scalar bit will find plenty of things that are actually part of
 * strings. When we're done, json_block will fuse the two together by masking out tokens that are
 * part of a string.
 */
class json_scanner {
public:
  json_scanner() {}
  simdjson_really_inline json_block next(const simd::simd8x64<uint8_t>& in);
  simdjson_really_inline error_code finish(bool streaming);

private:
  // Whether the last character of the previous iteration is part of a scalar token
  // (anything except whitespace or a structural character/'operator').
  uint64_t prev_scalar = 0ULL;
  json_string_scanner string_scanner{};
};


//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
simdjson_really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

simdjson_really_inline json_block json_scanner::next(const simd::simd8x64<uint8_t>& in) {
  json_string_block strings = string_scanner.next(in);
  // identifies the white-space and the structurat characters
  json_character_block characters = json_character_block::classify(in);
  // The term "scalar" refers to anything except structural characters and white space
  // (so letters, numbers, quotes).
  // We want  follows_scalar to mark anything that follows a non-quote scalar (so letters and numbers).
  //
  // A terminal quote should either be followed by a structural character (comma, brace, bracket, colon)
  // or nothing. However, we still want ' "a string"true ' to mark the 't' of 'true' as a potential
  // pseudo-structural character just like we would if we had  ' "a string" true '; otherwise we
  // may need to add an extra check when parsing strings.
  //
  // Performance: there are many ways to skin this cat.
  const uint64_t nonquote_scalar = characters.scalar() & ~strings.quote();
  uint64_t follows_nonquote_scalar = follows(nonquote_scalar, prev_scalar);
  return {
    strings,
    characters,
    follows_nonquote_scalar
  };
}

simdjson_really_inline error_code json_scanner::finish(bool streaming) {
  return string_scanner.finish(streaming);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/json_scanner.h */
/* begin file src/generic/stage1/json_minifier.h */
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class json_minifier {
public:
  template<size_t STEP_SIZE>
  static error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept;

private:
  simdjson_really_inline json_minifier(uint8_t *_dst)
  : dst{_dst}
  {}
  template<size_t STEP_SIZE>
  simdjson_really_inline void step(const uint8_t *block_buf, buf_block_reader<STEP_SIZE> &reader) noexcept;
  simdjson_really_inline void next(const simd::simd8x64<uint8_t>& in, json_block block);
  simdjson_really_inline error_code finish(uint8_t *dst_start, size_t &dst_len);
  json_scanner scanner{};
  uint8_t *dst;
};

simdjson_really_inline void json_minifier::next(const simd::simd8x64<uint8_t>& in, json_block block) {
  uint64_t mask = block.whitespace();
  in.compress(mask, dst);
  dst += 64 - count_ones(mask);
}

simdjson_really_inline error_code json_minifier::finish(uint8_t *dst_start, size_t &dst_len) {
  *dst = '\0';
  error_code error = scanner.finish(false);
  if (error) { dst_len = 0; return error; }
  dst_len = dst - dst_start;
  return SUCCESS;
}

template<>
simdjson_really_inline void json_minifier::step<128>(const uint8_t *block_buf, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  simd::simd8x64<uint8_t> in_2(block_buf+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1);
  this->next(in_2, block_2);
  reader.advance();
}

template<>
simdjson_really_inline void json_minifier::step<64>(const uint8_t *block_buf, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  json_block block_1 = scanner.next(in_1);
  this->next(block_buf, block_1);
  reader.advance();
}

template<size_t STEP_SIZE>
error_code json_minifier::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept {
  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_minifier minifier(dst);

  // Index the first n-1 blocks
  while (reader.has_full_block()) {
    minifier.step<STEP_SIZE>(reader.full_block(), reader);
  }

  // Index the last (remainder) block, padded with spaces
  uint8_t block[STEP_SIZE];
  if (simdjson_likely(reader.get_remainder(block)) > 0) {
    minifier.step<STEP_SIZE>(block, reader);
  }

  return minifier.finish(dst, dst_len);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/json_minifier.h */
/* begin file src/generic/stage1/find_next_document_index.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {

/**
  * This algorithm is used to quickly identify the last structural position that
  * makes up a complete document.
  *
  * It does this by going backwards and finding the last *document boundary* (a
  * place where one value follows another without a comma between them). If the
  * last document (the characters after the boundary) has an equal number of
  * start and end brackets, it is considered complete.
  *
  * Simply put, we iterate over the structural characters, starting from
  * the end. We consider that we found the end of a JSON document when the
  * first element of the pair is NOT one of these characters: '{' '[' ';' ','
  * and when the second element is NOT one of these characters: '}' '}' ';' ','.
  *
  * This simple comparison works most of the time, but it does not cover cases
  * where the batch's structural indexes contain a perfect amount of documents.
  * In such a case, we do not have access to the structural index which follows
  * the last document, therefore, we do not have access to the second element in
  * the pair, and that means we cannot identify the last document. To fix this
  * issue, we keep a count of the open and closed curly/square braces we found
  * while searching for the pair. When we find a pair AND the count of open and
  * closed curly/square braces is the same, we know that we just passed a
  * complete document, therefore the last json buffer location is the end of the
  * batch.
  */
simdjson_really_inline uint32_t find_next_document_index(dom_parser_implementation &parser) {
  // TODO don't count separately, just figure out depth
  auto arr_cnt = 0;
  auto obj_cnt = 0;
  for (auto i = parser.n_structural_indexes - 1; i > 0; i--) {
    auto idxb = parser.structural_indexes[i];
    switch (parser.buf[idxb]) {
    case ':':
    case ',':
      continue;
    case '}':
      obj_cnt--;
      continue;
    case ']':
      arr_cnt--;
      continue;
    case '{':
      obj_cnt++;
      break;
    case '[':
      arr_cnt++;
      break;
    }
    auto idxa = parser.structural_indexes[i - 1];
    switch (parser.buf[idxa]) {
    case '{':
    case '[':
    case ':':
    case ',':
      continue;
    }
    // Last document is complete, so the next document will appear after!
    if (!arr_cnt && !obj_cnt) {
      return parser.n_structural_indexes;
    }
    // Last document is incomplete; mark the document at i + 1 as the next one
    return i;
  }
  return 0;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/find_next_document_index.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class bit_indexer {
public:
  uint32_t *tail;

  simdjson_really_inline bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // base_ptr[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so base_ptr
  // needs to be large enough to handle this
  simdjson_really_inline void write(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
        return;
    int cnt = static_cast<int>(count_ones(bits));

    // Do the first 8 all together
    for (int i=0; i<8; i++) {
      this->tail[i] = idx + trailing_zeroes(bits);
      bits = clear_lowest_bit(bits);
    }

    // Do the next 8 all together (we hope in most cases it won't happen at all
    // and the branch is easily predicted).
    if (simdjson_unlikely(cnt > 8)) {
      for (int i=8; i<16; i++) {
        this->tail[i] = idx + trailing_zeroes(bits);
        bits = clear_lowest_bit(bits);
      }

      // Most files don't have 16+ structurals per block, so we take several basically guaranteed
      // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
      // or the start of a value ("abc" true 123) every four characters.
      if (simdjson_unlikely(cnt > 16)) {
        int i = 16;
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

class json_structural_indexer {
public:
  /**
   * Find the important bits of JSON in a 128-byte chunk, and add them to structural_indexes.
   *
   * @param partial Setting the partial parameter to true allows the find_structural_bits to
   *   tolerate unclosed strings. The caller should still ensure that the input is valid UTF-8. If
   *   you are processing substrings, you may want to call on a function like trimmed_length_safe_utf8.
   */
  template<size_t STEP_SIZE>
  static error_code index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, bool partial) noexcept;

private:
  simdjson_really_inline json_structural_indexer(uint32_t *structural_indexes);
  template<size_t STEP_SIZE>
  simdjson_really_inline void step(const uint8_t *block, buf_block_reader<STEP_SIZE> &reader) noexcept;
  simdjson_really_inline void next(const simd::simd8x64<uint8_t>& in, json_block block, size_t idx);
  simdjson_really_inline error_code finish(dom_parser_implementation &parser, size_t idx, size_t len, bool partial);

  json_scanner scanner{};
  utf8_checker checker{};
  bit_indexer indexer;
  uint64_t prev_structurals = 0;
  uint64_t unescaped_chars_error = 0;
};

simdjson_really_inline json_structural_indexer::json_structural_indexer(uint32_t *structural_indexes) : indexer{structural_indexes} {}

// Skip the last character if it is partial
simdjson_really_inline size_t trim_partial_utf8(const uint8_t *buf, size_t len) {
  if (simdjson_unlikely(len < 3)) {
    switch (len) {
      case 2:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 2 bytes left
        return len;
      case 1:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        return len;
      case 0:
        return len;
    }
  }
  if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
  if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 1 byte left
  if (buf[len-3] >= 0b11110000) { return len-3; } // 4-byte characters with only 3 bytes left
  return len;
}

//
// PERF NOTES:
// We pipe 2 inputs through these stages:
// 1. Load JSON into registers. This takes a long time and is highly parallelizable, so we load
//    2 inputs' worth at once so that by the time step 2 is looking for them input, it's available.
// 2. Scan the JSON for critical data: strings, scalars and operators. This is the critical path.
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
template<size_t STEP_SIZE>
error_code json_structural_indexer::index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, bool partial) noexcept {
  if (simdjson_unlikely(len > parser.capacity())) { return CAPACITY; }
  if (partial) { len = trim_partial_utf8(buf, len); }

  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_structural_indexer indexer(parser.structural_indexes.get());

  // Read all but the last block
  while (reader.has_full_block()) {
    indexer.step<STEP_SIZE>(reader.full_block(), reader);
  }

  // Take care of the last block (will always be there unless file is empty)
  uint8_t block[STEP_SIZE];
  if (simdjson_unlikely(reader.get_remainder(block) == 0)) { return EMPTY; }
  indexer.step<STEP_SIZE>(block, reader);

  return indexer.finish(parser, reader.block_index(), len, partial);
}

template<>
simdjson_really_inline void json_structural_indexer::step<128>(const uint8_t *block, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  simd::simd8x64<uint8_t> in_2(block+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1, reader.block_index());
  this->next(in_2, block_2, reader.block_index()+64);
  reader.advance();
}

template<>
simdjson_really_inline void json_structural_indexer::step<64>(const uint8_t *block, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  json_block block_1 = scanner.next(in_1);
  this->next(in_1, block_1, reader.block_index());
  reader.advance();
}

simdjson_really_inline void json_structural_indexer::next(const simd::simd8x64<uint8_t>& in, json_block block, size_t idx) {
  uint64_t unescaped = in.lteq(0x1F);
  checker.check_next_input(in);
  indexer.write(uint32_t(idx-64), prev_structurals); // Output *last* iteration's structurals to the parser
  prev_structurals = block.structural_start();
  unescaped_chars_error |= block.non_quote_inside_string(unescaped);
}

simdjson_really_inline error_code json_structural_indexer::finish(dom_parser_implementation &parser, size_t idx, size_t len, bool partial) {
  // Write out the final iteration's structurals
  indexer.write(uint32_t(idx-64), prev_structurals);

  error_code error = scanner.finish(partial);
  if (simdjson_unlikely(error != SUCCESS)) { return error; }

  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }

  parser.n_structural_indexes = uint32_t(indexer.tail - parser.structural_indexes.get());
  /***
   * This is related to https://github.com/simdjson/simdjson/issues/906
   * Basically, we want to make sure that if the parsing continues beyond the last (valid)
   * structural character, it quickly stops.
   * Only three structural characters can be repeated without triggering an error in JSON:  [,] and }.
   * We repeat the padding character (at 'len'). We don't know what it is, but if the parsing
   * continues, then it must be [,] or }.
   * Suppose it is ] or }. We backtrack to the first character, what could it be that would
   * not trigger an error? It could be ] or } but no, because you can't start a document that way.
   * It can't be a comma, a colon or any simple value. So the only way we could continue is
   * if the repeated character is [. But if so, the document must start with [. But if the document
   * starts with [, it should end with ]. If we enforce that rule, then we would get
   * ][[ which is invalid.
   **/
  parser.structural_indexes[parser.n_structural_indexes] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 1] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 2] = 0;
  parser.next_structural_index = 0;
  // a valid JSON file cannot have zero structural indexes - we should have found something
  if (simdjson_unlikely(parser.n_structural_indexes == 0u)) {
    return EMPTY;
  }
  if (simdjson_unlikely(parser.structural_indexes[parser.n_structural_indexes - 1] > len)) {
    return UNEXPECTED_ERROR;
  }
  if (partial) {
    auto new_structural_indexes = find_next_document_index(parser);
    if (new_structural_indexes == 0 && parser.n_structural_indexes > 0) {
      return CAPACITY; // If the buffer is partial but the document is incomplete, it's too big to parse.
    }
    parser.n_structural_indexes = new_structural_indexes;
  }
  return checker.errors();
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/find_next_document_index.h */
/* begin file src/generic/stage1/utf8_validator.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

/**
 * Validates that the string is actual UTF-8.
 */
template<class checker>
bool generic_validate_utf8(const uint8_t * input, size_t length) {
    checker c{};
    buf_block_reader<64> reader(input, length);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      c.check_next_input(in);
      reader.advance();
    }
    uint8_t block[64]{};
    reader.get_remainder(block);
    simd::simd8x64<uint8_t> in(block);
    c.check_next_input(in);
    reader.advance();
    return c.errors() == error_code::SUCCESS;
}

bool generic_validate_utf8(const char * input, size_t length) {
    return generic_validate_utf8<utf8_checker>((const uint8_t *)input,length);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/utf8_validator.h */

//
// Stage 2
//

/* begin file src/arm64/stringparsing.h */
#ifndef SIMDJSON_ARM64_STRINGPARSING_H
#define SIMDJSON_ARM64_STRINGPARSING_H

/* arm64/simd.h already included: #include "arm64/simd.h" */
/* arm64/bitmanipulation.h already included: #include "arm64/bitmanipulation.h" */

namespace {
namespace arm64 {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_really_inline bool has_backslash() { return bs_bits != 0; }
  simdjson_really_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_really_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + sizeof(v0));
  v0.store(dst);
  v1.store(dst + sizeof(v0));

  // Getting a 64-bit bitmask is much cheaper than multiple 16-bit bitmasks on ARM; therefore, we
  // smash them together into a 64-byte mask and get the bitmask from there.
  uint64_t bs_and_quote = simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
    uint32_t(bs_and_quote),      // bs_bits
    uint32_t(bs_and_quote >> 32) // quote_bits
  };
}

} // namespace arm64
} // unnamed namespace

/* begin file src/generic/stage2/stringparsing.h */
// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace stringparsing {

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
SIMDJSON_WARN_UNUSED
simdjson_really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
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

SIMDJSON_WARN_UNUSED simdjson_really_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst) {
  src++;
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return nullptr;
}

SIMDJSON_UNUSED SIMDJSON_WARN_UNUSED simdjson_really_inline error_code parse_string_to_buffer(const uint8_t *src, uint8_t *&current_string_buf_loc, std::string_view &s) {
  if (src[0] != '"') { return STRING_ERROR; }
  auto end = stringparsing::parse_string(src, current_string_buf_loc);
  if (!end) { return STRING_ERROR; }
  s = std::string_view((const char *)current_string_buf_loc, end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}

} // namespace stringparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/stringparsing.h */

#endif // SIMDJSON_ARM64_STRINGPARSING_H
/* end file src/generic/stage2/stringparsing.h */
/* begin file src/arm64/numberparsing.h */
#ifndef SIMDJSON_ARM64_NUMBERPARSING_H
#define SIMDJSON_ARM64_NUMBERPARSING_H

namespace {
namespace arm64 {

// we don't have SSE, so let us use a scalar function
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  uint64_t val;
  memcpy(&val, chars, sizeof(uint64_t));
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return uint32_t((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}

} // namespace arm64
} // unnamed namespace

#define SWAR_NUMBER_PARSING

/* begin file src/generic/stage2/numberparsing.h */
#include <cmath>
#include <limits>

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace numberparsing {

#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [FASTFLOAT_SMALLEST_POWER,
// FASTFLOAT_LARGEST_POWER] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / power_of_ten[-power];
    } else {
      d = d * power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It wa
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }

  // We are going to need to do some 64-bit arithmetic to get a more precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= FASTFLOAT_SMALLEST_POWER
  // and power <= FASTFLOAT_LARGEST_POWER
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  uint64_t factor_mantissa = mantissa_64[power - FASTFLOAT_SMALLEST_POWER];
  
  // The exponent is 1024 + 63 + power 
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power
  //
  // The 65536 is (1<<16) and corresponds to 
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to 
  // floor(log(5**power)/log(2)) 
  //
  // Note that this is not magic: 152170/(1<<16) is 
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a 
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;
  

  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  value128 product = full_multiplication(i, factor_mantissa);
  uint64_t lower = product.low;
  uint64_t upper = product.high;

  // We know that upper has at most one leading zero because
  // both i and  factor_mantissa have a leading one. This means
  // that the result is at least as large as ((1<<63)*(1<<63))/(1<<64).

  // As long as the first 9 bits of "upper" are not "1", then we
  // know that we have an exact computed value for the leading
  // 55 bits because any imprecision would play out as a +1, in
  // the worst case.
  if (simdjson_unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
    uint64_t factor_mantissa_low =
        mantissa_128[power - FASTFLOAT_SMALLEST_POWER];
    // next, we compute the 64-bit x 128-bit multiplication, getting a 192-bit
    // result (three 64-bit values)
    product = full_multiplication(i, factor_mantissa_low);
    uint64_t product_low = product.low;
    uint64_t product_middle2 = product.high;
    uint64_t product_middle1 = lower;
    uint64_t product_high = upper;
    uint64_t product_middle = product_middle1 + product_middle2;
    if (product_middle < product_middle1) {
      product_high++; // overflow carry
    }
    // We want to check whether mantissa *i + i would affect our result.
    // This does happen, e.g. with 7.3177701707893310e+15.
    if (((product_middle + 1 == 0) && ((product_high & 0x1FF) == 0x1FF) &&
         (product_low + i < product_low))) { // let us be prudent and bail out.
      return false;
    }
    upper = product_high;
    lower = product_middle;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).

  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  if (simdjson_unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
               ((mantissa & 3) == 1))) {
    // if mantissa & 1 == 1 we might need to round up.
    //
    // Scenarios:
    // 1. We are not in the middle. Then we should round up.
    //
    // 2. We are right in the middle. Whether we round up depends
    // on the last significant bit: if it is "one" then we round
    // up (round to even) otherwise, we do not.
    //
    // So if the last significant bit is 1, we can safely round up.
    // Hence we only need to bail out if (mantissa & 3) == 1.
    // Otherwise we may need more accuracy or analysis to determine whether
    // we are exactly between two floating-point numbers.
    // It can be triggered with 1e23.
    // Note: because the factor_mantissa and factor_mantissa_low are
    // almost always rounded down (except for small positive powers),
    // almost always should round up.
    return false;
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    lz--; // undo previous addition
  }
  mantissa &= ~(1ULL << 52);
  uint64_t real_exponent = exponent - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    return false;
  }
  mantissa |= real_exponent << 52;
  mantissa |= (((uint64_t)negative) << 63);
  memcpy(&d, &mantissa, sizeof(d));
  return true;
}

static bool parse_float_strtod(const uint8_t *ptr, double *outDouble) {
  char *endptr;
  *outDouble = strtod((const char *)ptr, &endptr);
  // Some libraries will set errno = ERANGE when the value is subnormal,
  // yet we may want to be able to parse subnormal values.
  // However, we do not want to tolerate NAN or infinite values.
  //
  // Values like infinity or NaN are not allowed in the JSON specification.
  // If you consume a large value and you map it to "infinity", you will no
  // longer be able to serialize back a standard-compliant JSON. And there is
  // no realistic application where you might need values so large than they
  // can't fit in binary64. The maximal value is about  1.7976931348623157 x
  // 10^308 It is an unimaginable large number. There will never be any piece of
  // engineering involving as many as 10^308 parts. It is estimated that there
  // are about 10^80 atoms in the universe.  The estimate for the total number
  // of electrons is similar. Using a double-precision floating-point value, we
  // can represent easily the number of atoms in the universe. We could  also
  // represent the number of ways you can pick any three individual atoms at
  // random in the universe. If you ever encounter a number much larger than
  // 10^308, you know that you have a bug. RapidJSON will reject a document with
  // a float that does not fit in binary64. JSON for Modern C++ (nlohmann/json)
  // will flat out throw an exception.
  //
  if ((endptr == (const char *)ptr) || (!std::isfinite(*outDouble))) {
    return false;
  }
  return true;
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(SIMDJSON_UNUSED const uint8_t * src, W writer) {
  double d;
  if (parse_float_strtod(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow. 
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off 
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do 
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time. 
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in 
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline int significant_digits(const uint8_t * start_digits, int digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - int(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, int digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  // digit count is off by 1 because of the decimal (assuming there was one).
  if (simdjson_unlikely(digit_count-1 >= 19 && significant_digits(start_digits, digit_count) >= 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < FASTFLOAT_SMALLEST_POWER) || (exponent > FASTFLOAT_LARGEST_POWER)) {
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_strtod(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool clean_end = is_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (!clean_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// SAX functions
namespace {
// Parse any number from 0 to 18,446,744,073,709,551,615
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  if (digit_count > 20) { return NUMBER_ERROR; }
  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return i;
}

// Parse any number from 0 to 18,446,744,073,709,551,615
// Call this version of the method if you regularly expect 8- or 16-digit numbers.
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_large_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
      if (parse_digit(*p, i)) { // digit 17
        p++;
        if (parse_digit(*p, i)) { // digit 18
          p++;
          if (parse_digit(*p, i)) { // digit 19
            p++;
            if (parse_digit(*p, i)) { // digit 20
              p++;
              if (parse_digit(*p, i)) { return NUMBER_ERROR; } // 21 digits is an error
              // Positive overflow check:
              // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
              //   biggest uint64_t.
              // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
              //   If we got here, it's a 20 digit number starting with the digit "1".
              // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
              //   than 1,553,255,926,290,448,384.
              // - That is smaller than the smallest possible 20-digit number the user could write:
              //   10,000,000,000,000,000,000.
              // - Therefore, if the number is positive and lower than that, it's overflow.
              // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
              //
              if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
            }
          }
        }
      }
    } // 16 digits
  } else { // 8 digits
    // Less than 8 digits can't overflow, simpler logic here.
    if (parse_digit(*p, i)) { p++; } else { return NUMBER_ERROR; }
    while (parse_digit(*p, i)) { p++; }
  }

  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }
  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - src);
  if (digit_count == 0 || ('0' == *src && digit_count > 1)) { return NUMBER_ERROR; }
  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return NUMBER_ERROR; }
  if (digit_count == longest_digit_count) {
    if(negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return NUMBER_ERROR; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return negative ? (~i+1) : i;
}

SIMDJSON_UNUSED simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src || (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 >= 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src >= 19;
    }
  } else {
    overflow = p-src >= 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
    overflow = overflow || exponent < FASTFLOAT_SMALLEST_POWER || exponent > FASTFLOAT_LARGEST_POWER;
  }

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_strtod(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/numberparsing.h */

#endif // SIMDJSON_ARM64_NUMBERPARSING_H
/* end file src/generic/stage2/numberparsing.h */
/* begin file src/generic/stage2/tape_builder.h */
/* begin file src/generic/stage2/json_iterator.h */
/* begin file src/generic/stage2/logger.h */
// This is for an internal-only stage 2 specific logger.
// Set LOG_ENABLED = true to log what stage 2 is doing!
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace logger {

  static constexpr const char * DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";

  static constexpr const bool LOG_ENABLED = false;
  static constexpr const int LOG_EVENT_LEN = 20;
  static constexpr const int LOG_BUFFER_LEN = 30;
  static constexpr const int LOG_SMALL_BUFFER_LEN = 10;
  static constexpr const int LOG_INDEX_LEN = 5;

  static int log_depth; // Not threadsafe. Log only.

  // Helper to turn unprintable or newline characters into spaces
  static simdjson_really_inline char printable_char(char c) {
    if (c >= 0x20) {
      return c;
    } else {
      return ' ';
    }
  }

  // Print the header and set up log_start
  static simdjson_really_inline void log_start() {
    if (LOG_ENABLED) {
      log_depth = 0;
      printf("\n");
      printf("| %-*s | %-*s | %-*s | %-*s | Detail |\n", LOG_EVENT_LEN, "Event", LOG_BUFFER_LEN, "Buffer", LOG_SMALL_BUFFER_LEN, "Next", 5, "Next#");
      printf("|%.*s|%.*s|%.*s|%.*s|--------|\n", LOG_EVENT_LEN+2, DASHES, LOG_BUFFER_LEN+2, DASHES, LOG_SMALL_BUFFER_LEN+2, DASHES, 5+2, DASHES);
    }
  }

  // Logs a single line of 
  template<typename S>
  static simdjson_really_inline void log_line(S &structurals, const char *title_prefix, const char *title, const char *detail) {
    if (LOG_ENABLED) {
      printf("| %*s%s%-*s ", log_depth*2, "", title_prefix, LOG_EVENT_LEN - log_depth*2 - int(strlen(title_prefix)), title);
      auto current_index = structurals.at_beginning() ? nullptr : structurals.next_structural-1;
      auto next_index = structurals.next_structural;
      auto current = current_index ? &structurals.buf[*current_index] : (const uint8_t*)"                                                       ";
      auto next = &structurals.buf[*next_index];
      {
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_BUFFER_LEN;i++) {
          printf("%c", printable_char(current[i]));
        }
        printf(" ");
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_SMALL_BUFFER_LEN;i++) {
          printf("%c", printable_char(next[i]));
        }
        printf(" ");
      }
      if (current_index) {
        printf("| %*u ", LOG_INDEX_LEN, *current_index);
      } else {
        printf("| %-*s ", LOG_INDEX_LEN, "");
      }
      // printf("| %*u ", LOG_INDEX_LEN, structurals.next_tape_index());
      printf("| %-s ", detail);
      printf("|\n");
    }
  }

} // namespace logger
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/logger.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

class json_iterator {
public:
  const uint8_t* const buf;
  uint32_t *next_structural;
  dom_parser_implementation &dom_parser;
  uint32_t depth{0};

  /**
   * Walk the JSON document.
   *
   * The visitor receives callbacks when values are encountered. All callbacks pass the iterator as
   * the first parameter; some callbacks have other parameters as well:
   *
   * - visit_document_start() - at the beginning.
   * - visit_document_end() - at the end (if things were successful).
   *
   * - visit_array_start() - at the start `[` of a non-empty array.
   * - visit_array_end() - at the end `]` of a non-empty array.
   * - visit_empty_array() - when an empty array is encountered.
   *
   * - visit_object_end() - at the start `]` of a non-empty object.
   * - visit_object_start() - at the end `]` of a non-empty object.
   * - visit_empty_object() - when an empty object is encountered.
   * - visit_key(const uint8_t *key) - when a key in an object field is encountered. key is
   *                                   guaranteed to point at the first quote of the string (`"key"`).
   * - visit_primitive(const uint8_t *value) - when a value is a string, number, boolean or null.
   * - visit_root_primitive(iter, uint8_t *value) - when the top-level value is a string, number, boolean or null.
   *
   * - increment_count(iter) - each time a value is found in an array or object.
   */
  template<bool STREAMING, typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code walk_document(V &visitor) noexcept;

  /**
   * Create an iterator capable of walking a JSON document.
   *
   * The document must have already passed through stage 1.
   */
  simdjson_really_inline json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index);

  /**
   * Look at the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *peek() const noexcept;
  /**
   * Advance to the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *advance() noexcept;
  /**
   * Get the remaining length of the document, from the start of the current token.
   */
  simdjson_really_inline size_t remaining_len() const noexcept;
  /**
   * Check if we are at the end of the document.
   *
   * If this is true, there are no more tokens.
   */
  simdjson_really_inline bool at_eof() const noexcept;
  /**
   * Check if we are at the beginning of the document.
   */
  simdjson_really_inline bool at_beginning() const noexcept;
  simdjson_really_inline uint8_t last_structural() const noexcept;

  /**
   * Log that a value has been found.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_value(const char *type) const noexcept;
  /**
   * Log the start of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_start_value(const char *type) const noexcept;
  /**
   * Log the end of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_end_value(const char *type) const noexcept;
  /**
   * Log an error.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_error(const char *error) const noexcept;

  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_primitive(V &visitor, const uint8_t *value) noexcept;
  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_primitive(V &visitor, const uint8_t *value) noexcept;
};

template<bool STREAMING, typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::walk_document(V &visitor) noexcept {
  logger::log_start();

  //
  // Start the document
  //
  if (at_eof()) { return EMPTY; }
  log_start_value("document");
  SIMDJSON_TRY( visitor.visit_document_start(*this) );

  //
  // Read first value
  //
  {
    auto value = advance();

    // Make sure the outer hash or array is closed before continuing; otherwise, there are ways we
    // could get into memory corruption. See https://github.com/simdjson/simdjson/issues/906
    if (!STREAMING) {
      switch (*value) {
        case '{': if (last_structural() != '}') { return TAPE_ERROR; }; break;
        case '[': if (last_structural() != ']') { return TAPE_ERROR; }; break;
      }
    }

    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_root_primitive(*this, value) ); break;
    }
  }
  goto document_end;

//
// Object parser states
//
object_begin:
  log_start_value("object");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = false;
  SIMDJSON_TRY( visitor.visit_object_start(*this) );

  {
    auto key = advance();
    if (*key != '"') { log_error("Object does not start with a key"); return TAPE_ERROR; }
    SIMDJSON_TRY( visitor.increment_count(*this) );
    SIMDJSON_TRY( visitor.visit_key(*this, key) );
  }

object_field:
  if (simdjson_unlikely( *advance() != ':' )) { log_error("Missing colon after key in object"); return TAPE_ERROR; }
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

object_continue:
  switch (*advance()) {
    case ',':
      SIMDJSON_TRY( visitor.increment_count(*this) );
      {
        auto key = advance();
        if (simdjson_unlikely( *key != '"' )) { log_error("Key string missing at beginning of field in object"); return TAPE_ERROR; }
        SIMDJSON_TRY( visitor.visit_key(*this, key) );
      }
      goto object_field;
    case '}': log_end_value("object"); SIMDJSON_TRY( visitor.visit_object_end(*this) ); goto scope_end;
    default: log_error("No comma between object fields"); return TAPE_ERROR;
  }

scope_end:
  depth--;
  if (depth == 0) { goto document_end; }
  if (dom_parser.is_array[depth]) { goto array_continue; }
  goto object_continue;

//
// Array parser states
//
array_begin:
  log_start_value("array");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = true;
  SIMDJSON_TRY( visitor.visit_array_start(*this) );
  SIMDJSON_TRY( visitor.increment_count(*this) );

array_value:
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

array_continue:
  switch (*advance()) {
    case ',': SIMDJSON_TRY( visitor.increment_count(*this) ); goto array_value;
    case ']': log_end_value("array"); SIMDJSON_TRY( visitor.visit_array_end(*this) ); goto scope_end;
    default: log_error("Missing comma between array values"); return TAPE_ERROR;
  }

document_end:
  log_end_value("document");
  SIMDJSON_TRY( visitor.visit_document_end(*this) );

  dom_parser.next_structural_index = uint32_t(next_structural - &dom_parser.structural_indexes[0]);

  // If we didn't make it to the end, it's an error
  if ( !STREAMING && dom_parser.next_structural_index != dom_parser.n_structural_indexes ) {
    log_error("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
    return TAPE_ERROR;
  }

  return SUCCESS;

} // walk_document()

simdjson_really_inline json_iterator::json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index)
  : buf{_dom_parser.buf},
    next_structural{&_dom_parser.structural_indexes[start_structural_index]},
    dom_parser{_dom_parser} {
}

simdjson_really_inline const uint8_t *json_iterator::peek() const noexcept {
  return &buf[*(next_structural)];
}
simdjson_really_inline const uint8_t *json_iterator::advance() noexcept {
  return &buf[*(next_structural++)];
}
simdjson_really_inline size_t json_iterator::remaining_len() const noexcept {
  return dom_parser.len - *(next_structural-1);
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return next_structural == &dom_parser.structural_indexes[dom_parser.n_structural_indexes];
}
simdjson_really_inline bool json_iterator::at_beginning() const noexcept {
  return next_structural == dom_parser.structural_indexes.get();
}
simdjson_really_inline uint8_t json_iterator::last_structural() const noexcept {
  return buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]];
}

simdjson_really_inline void json_iterator::log_value(const char *type) const noexcept {
  logger::log_line(*this, "", type, "");
}

simdjson_really_inline void json_iterator::log_start_value(const char *type) const noexcept {
  logger::log_line(*this, "+", type, "");
  if (logger::LOG_ENABLED) { logger::log_depth++; }
}

simdjson_really_inline void json_iterator::log_end_value(const char *type) const noexcept {
  if (logger::LOG_ENABLED) { logger::log_depth--; }
  logger::log_line(*this, "-", type, "");
}

simdjson_really_inline void json_iterator::log_error(const char *error) const noexcept {
  logger::log_line(*this, "", "ERROR", error);
}

template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_root_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_root_string(*this, value);
    case 't': return visitor.visit_root_true_atom(*this, value);
    case 'f': return visitor.visit_root_false_atom(*this, value);
    case 'n': return visitor.visit_root_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_root_number(*this, value);
    default:
      log_error("Document starts with a non-value character");
      return TAPE_ERROR;
  }
}
template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_string(*this, value);
    case 't': return visitor.visit_true_atom(*this, value);
    case 'f': return visitor.visit_false_atom(*this, value);
    case 'n': return visitor.visit_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_number(*this, value);
    default:
      log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/logger.h */
/* begin file src/generic/stage2/tape_writer.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_writer {
  /** The next place to write to tape */
  uint64_t *next_tape_loc;
  
  /** Write a signed 64-bit value to tape. */
  simdjson_really_inline void append_s64(int64_t value) noexcept;

  /** Write an unsigned 64-bit value to tape. */
  simdjson_really_inline void append_u64(uint64_t value) noexcept;

  /** Write a double value to tape. */
  simdjson_really_inline void append_double(double value) noexcept;

  /**
   * Append a tape entry (an 8-bit type,and 56 bits worth of value).
   */
  simdjson_really_inline void append(uint64_t val, internal::tape_type t) noexcept;

  /**
   * Skip the current tape entry without writing.
   *
   * Used to skip the start of the container, since we'll come back later to fill it in when the
   * container ends.
   */
  simdjson_really_inline void skip() noexcept;

  /**
   * Skip the number of tape entries necessary to write a large u64 or i64.
   */
  simdjson_really_inline void skip_large_integer() noexcept;

  /**
   * Skip the number of tape entries necessary to write a double.
   */
  simdjson_really_inline void skip_double() noexcept;

  /**
   * Write a value to a known location on tape.
   *
   * Used to go back and write out the start of a container after the container ends.
   */
  simdjson_really_inline static void write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept;

private:
  /**
   * Append both the tape entry, and a supplementary value following it. Used for types that need
   * all 64 bits, such as double and uint64_t.
   */
  template<typename T>
  simdjson_really_inline void append2(uint64_t val, T val2, internal::tape_type t) noexcept;
}; // struct number_writer

simdjson_really_inline void tape_writer::append_s64(int64_t value) noexcept {
  append2(0, value, internal::tape_type::INT64);
}

simdjson_really_inline void tape_writer::append_u64(uint64_t value) noexcept {
  append(0, internal::tape_type::UINT64);
  *next_tape_loc = value;
  next_tape_loc++;
}

/** Write a double value to tape. */
simdjson_really_inline void tape_writer::append_double(double value) noexcept {
  append2(0, value, internal::tape_type::DOUBLE);
}

simdjson_really_inline void tape_writer::skip() noexcept {
  next_tape_loc++;
}

simdjson_really_inline void tape_writer::skip_large_integer() noexcept {
  next_tape_loc += 2;
}

simdjson_really_inline void tape_writer::skip_double() noexcept {
  next_tape_loc += 2;
}

simdjson_really_inline void tape_writer::append(uint64_t val, internal::tape_type t) noexcept {
  *next_tape_loc = val | ((uint64_t(char(t))) << 56);
  next_tape_loc++;
}

template<typename T>
simdjson_really_inline void tape_writer::append2(uint64_t val, T val2, internal::tape_type t) noexcept {
  append(val, t);
  static_assert(sizeof(val2) == sizeof(*next_tape_loc), "Type is not 64 bits!");
  memcpy(next_tape_loc, &val2, sizeof(val2));
  next_tape_loc++;
}

simdjson_really_inline void tape_writer::write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept {
  tape_loc = val | ((uint64_t(char(t))) << 56);
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/tape_writer.h */
/* begin file src/generic/stage2/atomparsing.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_really_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
SIMDJSON_WARN_UNUSED
simdjson_really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | is_not_structural_or_whitespace(src[4])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | is_not_structural_or_whitespace(src[5])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | is_not_structural_or_whitespace(src[4])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/atomparsing.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_builder {
  template<bool STREAMING>
  SIMDJSON_WARN_UNUSED static simdjson_really_inline error_code parse_document(
    dom_parser_implementation &dom_parser,
    dom::document &doc) noexcept;

  /** Called when a non-empty document starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_document_start(json_iterator &iter) noexcept;
  /** Called when a non-empty document ends without error. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_document_end(json_iterator &iter) noexcept;

  /** Called when a non-empty array starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_array_start(json_iterator &iter) noexcept;
  /** Called when a non-empty array ends. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_array_end(json_iterator &iter) noexcept;
  /** Called when an empty array is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_empty_array(json_iterator &iter) noexcept;

  /** Called when a non-empty object starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_object_start(json_iterator &iter) noexcept;
  /**
   * Called when a key in a field is encountered.
   *
   * primitive, visit_object_start, visit_empty_object, visit_array_start, or visit_empty_array
   * will be called after this with the field value.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_key(json_iterator &iter, const uint8_t *key) noexcept;
  /** Called when a non-empty object ends. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_object_end(json_iterator &iter) noexcept;
  /** Called when an empty object is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_empty_object(json_iterator &iter) noexcept;

  /**
   * Called when a string, number, boolean or null is found.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_primitive(json_iterator &iter, const uint8_t *value) noexcept;
  /**
   * Called when a string, number, boolean or null is found at the top level of a document (i.e.
   * when there is no array or object and the entire document is a single string, number, boolean or
   * null.
   *
   * This is separate from primitive() because simdjson's normal primitive parsing routines assume
   * there is at least one more token after the value, which is only true in an array or object.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_string(json_iterator &iter, const uint8_t *value, bool key = false) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_number(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_string(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_number(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

  /** Called each time a new field or element in an array or object is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code increment_count(json_iterator &iter) noexcept;

  /** Next location to write to tape */
  tape_writer tape;
private:
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc;

  simdjson_really_inline tape_builder(dom::document &doc) noexcept;

  simdjson_really_inline uint32_t next_tape_index(json_iterator &iter) const noexcept;
  simdjson_really_inline void start_container(json_iterator &iter) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  simdjson_really_inline uint8_t *on_start_string(json_iterator &iter) noexcept;
  simdjson_really_inline void on_end_string(uint8_t *dst) noexcept;
}; // class tape_builder

template<bool STREAMING>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::parse_document(
    dom_parser_implementation &dom_parser,
    dom::document &doc) noexcept {
  dom_parser.doc = &doc;
  json_iterator iter(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
  tape_builder builder(doc);
  return iter.walk_document<STREAMING>(builder);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_root_primitive(*this, value);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_primitive(*this, value);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_empty_object(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_empty_array(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_document_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_object_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_array_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_object_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_array_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_document_end(json_iterator &iter) noexcept {
  constexpr uint32_t start_tape_index = 0;
  tape.append(start_tape_index, internal::tape_type::ROOT);
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter), internal::tape_type::ROOT);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_key(json_iterator &iter, const uint8_t *key) noexcept {
  return visit_string(iter, key, true);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::increment_count(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].count++; // we have a key value pair in the object at parser.dom_parser.depth - 1
  return SUCCESS;
}

simdjson_really_inline tape_builder::tape_builder(dom::document &doc) noexcept : tape{doc.tape.get()}, current_string_buf_loc{doc.string_buf.get()} {}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_string(json_iterator &iter, const uint8_t *value, bool key) noexcept {
  iter.log_value(key ? "key" : "string");
  uint8_t *dst = on_start_string(iter);
  dst = stringparsing::parse_string(value, dst);
  if (dst == nullptr) {
    iter.log_error("Invalid escape in string");
    return STRING_ERROR;
  }
  on_end_string(dst);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_string(json_iterator &iter, const uint8_t *value) noexcept {
  return visit_string(iter, value);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_number(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("number");
  return numberparsing::parse_number(value, tape);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_number(json_iterator &iter, const uint8_t *value) noexcept {
  //
  // We need to make a copy to make sure that the string is space terminated.
  // This is not about padding the input, which should already padded up
  // to len + SIMDJSON_PADDING. However, we have no control at this stage
  // on how the padding was done. What if the input string was padded with nulls?
  // It is quite common for an input string to have an extra null character (C string).
  // We do not want to allow 9\0 (where \0 is the null character) inside a JSON
  // document, but the string "9\0" by itself is fine. So we make a copy and
  // pad the input with spaces when we know that there is just one input element.
  // This copy is relatively expensive, but it will almost never be called in
  // practice unless you are in the strange scenario where you have many JSON
  // documents made of single atoms.
  //
  uint8_t *copy = static_cast<uint8_t *>(malloc(iter.remaining_len() + SIMDJSON_PADDING));
  if (copy == nullptr) { return MEMALLOC; }
  memcpy(copy, value, iter.remaining_len());
  memset(copy + iter.remaining_len(), ' ', SIMDJSON_PADDING);
  error_code error = visit_number(iter, copy);
  free(copy);
  return error;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  if (!atomparsing::is_valid_true_atom(value)) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  if (!atomparsing::is_valid_true_atom(value, iter.remaining_len())) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  if (!atomparsing::is_valid_false_atom(value)) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  if (!atomparsing::is_valid_false_atom(value, iter.remaining_len())) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  if (!atomparsing::is_valid_null_atom(value)) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  if (!atomparsing::is_valid_null_atom(value, iter.remaining_len())) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

// private:

simdjson_really_inline uint32_t tape_builder::next_tape_index(json_iterator &iter) const noexcept {
  return uint32_t(tape.next_tape_loc - iter.dom_parser.doc->tape.get());
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  auto start_index = next_tape_index(iter);
  tape.append(start_index+2, start);
  tape.append(start_index, end);
  return SUCCESS;
}

simdjson_really_inline void tape_builder::start_container(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].tape_index = next_tape_index(iter);
  iter.dom_parser.open_containers[iter.depth].count = 0;
  tape.skip(); // We don't actually *write* the start element until the end.
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  // Write the ending tape element, pointing at the start location
  const uint32_t start_tape_index = iter.dom_parser.open_containers[iter.depth].tape_index;
  tape.append(start_tape_index, end);
  // Write the start tape element, pointing at the end location (and including count)
  // count can overflow if it exceeds 24 bits... so we saturate
  // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
  const uint32_t count = iter.dom_parser.open_containers[iter.depth].count;
  const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter) | (uint64_t(cntsat) << 32), start);
  return SUCCESS;
}

simdjson_really_inline uint8_t *tape_builder::on_start_string(json_iterator &iter) noexcept {
  // we advance the point, accounting for the fact that we have a NULL termination
  tape.append(current_string_buf_loc - iter.dom_parser.doc->string_buf.get(), internal::tape_type::STRING);
  return current_string_buf_loc + sizeof(uint32_t);
}

simdjson_really_inline void tape_builder::on_end_string(uint8_t *dst) noexcept {
  uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
  // TODO check for overflow in case someone has a crazy string (>=4GB?)
  // But only add the overflow check when the document itself exceeds 4GB
  // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
  memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
  // NULL termination is still handy if you expect all your strings to
  // be NULL terminated? It comes at a small cost
  *dst = 0;
  current_string_buf_loc = dst + 1;
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/atomparsing.h */

//
// Implementation-specific overrides
//
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

simdjson_really_inline uint64_t json_string_scanner::find_escaped(uint64_t backslash) {
  // On ARM, we don't short-circuit this if there are no backslashes, because the branch gives us no
  // benefit and therefore makes things worse.
  // if (!backslash) { uint64_t escaped = prev_escaped; prev_escaped = 0; return escaped; }
  return find_escaped_branchless(backslash);
}

} // namespace stage1

SIMDJSON_WARN_UNUSED error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return arm64::stage1::json_minifier::minify<64>(buf, len, dst, dst_len);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, bool streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return arm64::stage1::json_structural_indexer::index<64>(buf, len, *this, streaming);
}

SIMDJSON_WARN_UNUSED bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return arm64::stage1::generic_validate_utf8(buf,len);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, false);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/arm64/end_implementation.h */
#undef SIMDJSON_IMPLEMENTATION
/* end file src/arm64/end_implementation.h */
/* end file src/arm64/end_implementation.h */
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
/* begin file src/fallback/implementation.cpp */
/* begin file src/fallback/begin_implementation.h */
#define SIMDJSON_IMPLEMENTATION fallback
/* fallback/implementation.h already included: #include "fallback/implementation.h" */
/* begin file src/fallback/bitmanipulation.h */
#ifndef SIMDJSON_FALLBACK_BITMANIPULATION_H
#define SIMDJSON_FALLBACK_BITMANIPULATION_H

#include <limits>

namespace {
namespace fallback {

#if defined(_MSC_VER) && !defined(_M_ARM64) && !defined(_M_X64)
static inline unsigned char _BitScanForward64(unsigned long* ret, uint64_t x) {
  unsigned long x0 = (unsigned long)x, top, bottom;
  _BitScanForward(&top, (unsigned long)(x >> 32));
  _BitScanForward(&bottom, x0);
  *ret = x0 ? bottom : 32 + top;
  return x != 0;
}
static unsigned char _BitScanReverse64(unsigned long* ret, uint64_t x) {
  unsigned long x1 = (unsigned long)(x >> 32), top, bottom;
  _BitScanReverse(&top, x1);
  _BitScanReverse(&bottom, (unsigned long)x);
  *ret = x1 ? top + 32 : bottom;
  return x != 0;
}
#endif

/* result might be undefined when input_num is zero */
simdjson_really_inline int leading_zeroes(uint64_t input_num) {
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

} // namespace fallback
} // unnamed namespace

#endif // SIMDJSON_FALLBACK_BITMANIPULATION_H
/* end file src/fallback/bitmanipulation.h */
/* end file src/fallback/bitmanipulation.h */
/* begin file src/fallback/dom_parser_implementation.h */
#ifndef SIMDJSON_FALLBACK_DOM_PARSER_IMPLEMENTATION_H
#define SIMDJSON_FALLBACK_DOM_PARSER_IMPLEMENTATION_H

/* begin file src/generic/dom_parser_implementation.h */
/* isadetection.h already included: #include "isadetection.h" */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  simdjson_really_inline dom_parser_implementation();
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation & operator=(const dom_parser_implementation &) = delete;

  SIMDJSON_WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  SIMDJSON_WARN_UNUSED error_code check_for_unclosed_array() noexcept;
  SIMDJSON_WARN_UNUSED error_code stage2(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage2_next(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_capacity(size_t capacity) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_max_depth(size_t max_depth) noexcept final;
};

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/generic/stage1/allocate.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {
namespace allocate {

//
// Allocates stage 1 internal state and outputs in the parser
//
simdjson_really_inline error_code set_capacity(internal::dom_parser_implementation &parser, size_t capacity) {
  size_t max_structures = SIMDJSON_ROUNDUP_N(capacity, 64) + 2 + 7;
  parser.structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] );
  if (!parser.structural_indexes) { return MEMALLOC; }
  parser.structural_indexes[0] = 0;
  parser.n_structural_indexes = 0;
  return SUCCESS;
}

} // namespace allocate
} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/allocate.h */
/* begin file src/generic/stage2/allocate.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace allocate {

//
// Allocates stage 2 internal state and outputs in the parser
//
simdjson_really_inline error_code set_max_depth(dom_parser_implementation &parser, size_t max_depth) {
  parser.open_containers.reset(new (std::nothrow) open_container[max_depth]);
  parser.is_array.reset(new (std::nothrow) bool[max_depth]);

  if (!parser.is_array || !parser.open_containers) {
    return MEMALLOC;
  }
  return SUCCESS;
}

} // namespace allocate
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/allocate.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

simdjson_really_inline dom_parser_implementation::dom_parser_implementation() {}

// Leaving these here so they can be inlined if so desired
SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  error_code err = stage1::allocate::set_capacity(*this, capacity);
  if (err) { _capacity = 0; return err; }
  _capacity = capacity;
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  error_code err = stage2::allocate::set_max_depth(*this, max_depth);
  if (err) { _max_depth = 0; return err; }
  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/allocate.h */

#endif // SIMDJSON_FALLBACK_DOM_PARSER_IMPLEMENTATION_H
/* end file src/generic/stage2/allocate.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

SIMDJSON_WARN_UNUSED error_code implementation::create_dom_parser_implementation(
  size_t capacity,
  size_t max_depth,
  std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  dst.reset( new (std::nothrow) dom_parser_implementation() );
  if (!dst) { return MEMALLOC; }
  dst->set_capacity(capacity);
  dst->set_max_depth(max_depth);
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/fallback/end_implementation.h */
#undef SIMDJSON_IMPLEMENTATION
/* end file src/fallback/end_implementation.h */
/* end file src/fallback/end_implementation.h */
/* begin file src/fallback/dom_parser_implementation.cpp */
/* begin file src/fallback/begin_implementation.h */
#define SIMDJSON_IMPLEMENTATION fallback
/* fallback/implementation.h already included: #include "fallback/implementation.h" */
/* fallback/bitmanipulation.h already included: #include "fallback/bitmanipulation.h" */
/* end file src/fallback/begin_implementation.h */
/* fallback/dom_parser_implementation.h already included: #include "fallback/dom_parser_implementation.h" */
/* begin file src/generic/stage2/jsoncharutils.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_really_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return structural_or_whitespace_negated[c];
}

simdjson_really_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = digit_to_val32[630 + src[0]];
  uint32_t v2 = digit_to_val32[420 + src[1]];
  uint32_t v3 = digit_to_val32[210 + src[2]];
  uint32_t v4 = digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_really_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#ifdef SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_really_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_really_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

simdjson_really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
#ifdef _M_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // _M_ARM64
#else // defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
  __uint128_t r = ((__uint128_t)value1) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/jsoncharutils.h */

//
// Stage 1
//
/* begin file src/generic/stage1/find_next_document_index.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {

/**
  * This algorithm is used to quickly identify the last structural position that
  * makes up a complete document.
  *
  * It does this by going backwards and finding the last *document boundary* (a
  * place where one value follows another without a comma between them). If the
  * last document (the characters after the boundary) has an equal number of
  * start and end brackets, it is considered complete.
  *
  * Simply put, we iterate over the structural characters, starting from
  * the end. We consider that we found the end of a JSON document when the
  * first element of the pair is NOT one of these characters: '{' '[' ';' ','
  * and when the second element is NOT one of these characters: '}' '}' ';' ','.
  *
  * This simple comparison works most of the time, but it does not cover cases
  * where the batch's structural indexes contain a perfect amount of documents.
  * In such a case, we do not have access to the structural index which follows
  * the last document, therefore, we do not have access to the second element in
  * the pair, and that means we cannot identify the last document. To fix this
  * issue, we keep a count of the open and closed curly/square braces we found
  * while searching for the pair. When we find a pair AND the count of open and
  * closed curly/square braces is the same, we know that we just passed a
  * complete document, therefore the last json buffer location is the end of the
  * batch.
  */
simdjson_really_inline uint32_t find_next_document_index(dom_parser_implementation &parser) {
  // TODO don't count separately, just figure out depth
  auto arr_cnt = 0;
  auto obj_cnt = 0;
  for (auto i = parser.n_structural_indexes - 1; i > 0; i--) {
    auto idxb = parser.structural_indexes[i];
    switch (parser.buf[idxb]) {
    case ':':
    case ',':
      continue;
    case '}':
      obj_cnt--;
      continue;
    case ']':
      arr_cnt--;
      continue;
    case '{':
      obj_cnt++;
      break;
    case '[':
      arr_cnt++;
      break;
    }
    auto idxa = parser.structural_indexes[i - 1];
    switch (parser.buf[idxa]) {
    case '{':
    case '[':
    case ':':
    case ',':
      continue;
    }
    // Last document is complete, so the next document will appear after!
    if (!arr_cnt && !obj_cnt) {
      return parser.n_structural_indexes;
    }
    // Last document is incomplete; mark the document at i + 1 as the next one
    return i;
  }
  return 0;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/find_next_document_index.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class structural_scanner {
public:

simdjson_really_inline structural_scanner(dom_parser_implementation &_parser, bool _partial)
  : buf{_parser.buf},
    next_structural_index{_parser.structural_indexes.get()},
    parser{_parser},
    len{static_cast<uint32_t>(_parser.len)},
    partial{_partial} {
}

simdjson_really_inline void add_structural() {
  *next_structural_index = idx;
  next_structural_index++;
}

simdjson_really_inline bool is_continuation(uint8_t c) {
  return (c & 0b11000000) == 0b10000000;
}

simdjson_really_inline void validate_utf8_character() {
  // Continuation
  if (simdjson_unlikely((buf[idx] & 0b01000000) == 0)) {
    // extra continuation
    error = UTF8_ERROR;
    idx++;
    return;
  }

  // 2-byte
  if ((buf[idx] & 0b00100000) == 0) {
    // missing continuation
    if (simdjson_unlikely(idx+1 > len || !is_continuation(buf[idx+1]))) {
      if (idx+1 > len && partial) { idx = len; return; }
      error = UTF8_ERROR;
      idx++;
      return;
    }
    // overlong: 1100000_ 10______
    if (buf[idx] <= 0b11000001) { error = UTF8_ERROR; }
    idx += 2;
    return;
  }

  // 3-byte
  if ((buf[idx] & 0b00010000) == 0) {
    // missing continuation
    if (simdjson_unlikely(idx+2 > len || !is_continuation(buf[idx+1]) || !is_continuation(buf[idx+2]))) {
      if (idx+2 > len && partial) { idx = len; return; }
      error = UTF8_ERROR;
      idx++;
      return;
    }
    // overlong: 11100000 100_____ ________
    if (buf[idx] == 0b11100000 && buf[idx+1] <= 0b10011111) { error = UTF8_ERROR; }
    // surrogates: U+D800-U+DFFF 11101101 101_____
    if (buf[idx] == 0b11101101 && buf[idx+1] >= 0b10100000) { error = UTF8_ERROR; }
    idx += 3;
    return;
  }

  // 4-byte
  // missing continuation
  if (simdjson_unlikely(idx+3 > len || !is_continuation(buf[idx+1]) || !is_continuation(buf[idx+2]) || !is_continuation(buf[idx+3]))) {
    if (idx+2 > len && partial) { idx = len; return; }
    error = UTF8_ERROR;
    idx++;
    return;
  }
  // overlong: 11110000 1000____ ________ ________
  if (buf[idx] == 0b11110000 && buf[idx+1] <= 0b10001111) { error = UTF8_ERROR; }
  // too large: > U+10FFFF:
  // 11110100 (1001|101_)____
  // 1111(1___|011_|0101) 10______
  // also includes 5, 6, 7 and 8 byte characters:
  // 11111___
  if (buf[idx] == 0b11110100 && buf[idx+1] >= 0b10010000) { error = UTF8_ERROR; }
  if (buf[idx] >= 0b11110101) { error = UTF8_ERROR; }
  idx += 4;
}

simdjson_really_inline void validate_string() {
  idx++; // skip first quote
  while (idx < len && buf[idx] != '"') {
    if (buf[idx] == '\\') {
      idx += 2;
    } else if (simdjson_unlikely(buf[idx] & 0b10000000)) {
      validate_utf8_character();
    } else {
      if (buf[idx] < 0x20) { error = UNESCAPED_CHARS; }
      idx++;
    }
  }
  if (idx >= len && !partial) { error = UNCLOSED_STRING; }
}

simdjson_really_inline bool is_whitespace_or_operator(uint8_t c) {
  switch (c) {
    case '{': case '}': case '[': case ']': case ',': case ':':
    case ' ': case '\r': case '\n': case '\t':
      return true;
    default:
      return false;
  }
}

//
// Parse the entire input in STEP_SIZE-byte chunks.
//
simdjson_really_inline error_code scan() {
  for (;idx<len;idx++) {
    switch (buf[idx]) {
      // String
      case '"':
        add_structural();
        validate_string();
        break;
      // Operator
      case '{': case '}': case '[': case ']': case ',': case ':':
        add_structural();
        break;
      // Whitespace
      case ' ': case '\r': case '\n': case '\t':
        break;
      // Primitive or invalid character (invalid characters will be checked in stage 2)
      default:
        // Anything else, add the structural and go until we find the next one
        add_structural();
        while (idx+1<len && !is_whitespace_or_operator(buf[idx+1])) {
          idx++;
        };
        break;
    }
  }
  *next_structural_index = len;
  // We pad beyond.
  // https://github.com/simdjson/simdjson/issues/906
  next_structural_index[1] = len;
  next_structural_index[2] = 0;
  parser.n_structural_indexes = uint32_t(next_structural_index - parser.structural_indexes.get());
  parser.next_structural_index = 0;

  if (simdjson_unlikely(parser.n_structural_indexes == 0)) {
    return EMPTY;
  }

  if (partial) {
    auto new_structural_indexes = find_next_document_index(parser);
    if (new_structural_indexes == 0 && parser.n_structural_indexes > 0) {
      return CAPACITY; // If the buffer is partial but the document is incomplete, it's too big to parse.
    }
    parser.n_structural_indexes = new_structural_indexes;
  }

  return error;
}

private:
  const uint8_t *buf;
  uint32_t *next_structural_index;
  dom_parser_implementation &parser;
  uint32_t len;
  uint32_t idx{0};
  error_code error{SUCCESS};
  bool partial;
}; // structural_scanner

} // namespace stage1

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, bool partial) noexcept {
  this->buf = _buf;
  this->len = _len;
  stage1::structural_scanner scanner(*this, partial);
  return scanner.scan();
}

// big table for the minifier
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

SIMDJSON_WARN_UNUSED error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  size_t i = 0, pos = 0;
  uint8_t quote = 0;
  uint8_t nonescape = 1;

  while (i < len) {
    unsigned char c = buf[i];
    uint8_t *meta = jump_table + 3 * c;

    quote = quote ^ (meta[0] & nonescape);
    dst[pos] = c;
    pos += meta[2] | quote;

    i += 1;
    nonescape = uint8_t(~nonescape) | (meta[1]);
  }
  dst_len = pos; // we intentionally do not work with a reference
  // for fear of aliasing
  return SUCCESS;
}

// credit: based on code from Google Fuchsia (Apache Licensed)
SIMDJSON_WARN_UNUSED bool implementation::validate_utf8(const char *buf, size_t len) const noexcept { 
  const uint8_t *data = (const uint8_t *)buf;
  uint64_t pos = 0;
  uint64_t next_pos = 0;
  uint32_t code_point = 0;
  while (pos < len) {
    // check of the next 8 bytes are ascii.
    next_pos = pos + 16;
    if (next_pos <= len) { // if it is safe to read 8 more bytes, check that they are ascii
      uint64_t v1;
      memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2};
      if ((v & 0x8080808080808080) == 0) {
        pos = next_pos;
        continue;
      }
    }
    unsigned char byte = data[pos];
    if (byte < 0b10000000) {
      pos++;
      continue;
    } else if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80 || 0x7ff < code_point) { return false; }
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if (code_point < 0x800 || 0xffff < code_point ||
          (0xd7ff < code_point && code_point < 0xe000)) {
        return false;
      }
    } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point =
          (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point < 0xffff || 0x10ffff < code_point) { return false; }
    } else {
      // we may have a continuation
      return false;
    }
    pos = next_pos;
  }
  return true;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

//
// Stage 2
//
/* begin file src/fallback/stringparsing.h */
#ifndef SIMDJSON_FALLBACK_STRINGPARSING_H
#define SIMDJSON_FALLBACK_STRINGPARSING_H


namespace {
namespace fallback {

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 1;
  simdjson_really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() { return c == '"'; }
  simdjson_really_inline bool has_backslash() { return c == '\\'; }
  simdjson_really_inline int quote_index() { return c == '"' ? 0 : 1; }
  simdjson_really_inline int backslash_index() { return c == '\\' ? 0 : 1; }

  uint8_t c;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // store to dest unconditionally - we can overwrite the bits we don't like later
  dst[0] = src[0];
  return { src[0] };
}

} // namespace fallback
} // unnamed namespace

/* begin file src/generic/stage2/stringparsing.h */
// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace stringparsing {

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
SIMDJSON_WARN_UNUSED
simdjson_really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
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

SIMDJSON_WARN_UNUSED simdjson_really_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst) {
  src++;
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return nullptr;
}

SIMDJSON_UNUSED SIMDJSON_WARN_UNUSED simdjson_really_inline error_code parse_string_to_buffer(const uint8_t *src, uint8_t *&current_string_buf_loc, std::string_view &s) {
  if (src[0] != '"') { return STRING_ERROR; }
  auto end = stringparsing::parse_string(src, current_string_buf_loc);
  if (!end) { return STRING_ERROR; }
  s = std::string_view((const char *)current_string_buf_loc, end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}

} // namespace stringparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/stringparsing.h */

#endif // SIMDJSON_FALLBACK_STRINGPARSING_H
/* end file src/generic/stage2/stringparsing.h */
/* begin file src/fallback/numberparsing.h */
#ifndef SIMDJSON_FALLBACK_NUMBERPARSING_H
#define SIMDJSON_FALLBACK_NUMBERPARSING_H

#ifdef JSON_TEST_NUMBERS // for unit testing
void found_invalid_number(const uint8_t *buf);
void found_integer(int64_t result, const uint8_t *buf);
void found_unsigned_integer(uint64_t result, const uint8_t *buf);
void found_float(double result, const uint8_t *buf);
#endif

namespace {
namespace SIMDJSON_IMPLEMENTATION {
static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const char *chars) {
  uint32_t result = 0;
  for (int i=0;i<8;i++) {
    result = result*10 + (chars[i] - '0');
  }
  return result;
}
static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  return parse_eight_digits_unrolled((const char *)chars);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#define SWAR_NUMBER_PARSING
/* begin file src/generic/stage2/numberparsing.h */
#include <cmath>
#include <limits>

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace numberparsing {

#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [FASTFLOAT_SMALLEST_POWER,
// FASTFLOAT_LARGEST_POWER] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / power_of_ten[-power];
    } else {
      d = d * power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It wa
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }

  // We are going to need to do some 64-bit arithmetic to get a more precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= FASTFLOAT_SMALLEST_POWER
  // and power <= FASTFLOAT_LARGEST_POWER
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  uint64_t factor_mantissa = mantissa_64[power - FASTFLOAT_SMALLEST_POWER];
  
  // The exponent is 1024 + 63 + power 
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power
  //
  // The 65536 is (1<<16) and corresponds to 
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to 
  // floor(log(5**power)/log(2)) 
  //
  // Note that this is not magic: 152170/(1<<16) is 
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a 
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;
  

  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  value128 product = full_multiplication(i, factor_mantissa);
  uint64_t lower = product.low;
  uint64_t upper = product.high;

  // We know that upper has at most one leading zero because
  // both i and  factor_mantissa have a leading one. This means
  // that the result is at least as large as ((1<<63)*(1<<63))/(1<<64).

  // As long as the first 9 bits of "upper" are not "1", then we
  // know that we have an exact computed value for the leading
  // 55 bits because any imprecision would play out as a +1, in
  // the worst case.
  if (simdjson_unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
    uint64_t factor_mantissa_low =
        mantissa_128[power - FASTFLOAT_SMALLEST_POWER];
    // next, we compute the 64-bit x 128-bit multiplication, getting a 192-bit
    // result (three 64-bit values)
    product = full_multiplication(i, factor_mantissa_low);
    uint64_t product_low = product.low;
    uint64_t product_middle2 = product.high;
    uint64_t product_middle1 = lower;
    uint64_t product_high = upper;
    uint64_t product_middle = product_middle1 + product_middle2;
    if (product_middle < product_middle1) {
      product_high++; // overflow carry
    }
    // We want to check whether mantissa *i + i would affect our result.
    // This does happen, e.g. with 7.3177701707893310e+15.
    if (((product_middle + 1 == 0) && ((product_high & 0x1FF) == 0x1FF) &&
         (product_low + i < product_low))) { // let us be prudent and bail out.
      return false;
    }
    upper = product_high;
    lower = product_middle;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).

  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  if (simdjson_unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
               ((mantissa & 3) == 1))) {
    // if mantissa & 1 == 1 we might need to round up.
    //
    // Scenarios:
    // 1. We are not in the middle. Then we should round up.
    //
    // 2. We are right in the middle. Whether we round up depends
    // on the last significant bit: if it is "one" then we round
    // up (round to even) otherwise, we do not.
    //
    // So if the last significant bit is 1, we can safely round up.
    // Hence we only need to bail out if (mantissa & 3) == 1.
    // Otherwise we may need more accuracy or analysis to determine whether
    // we are exactly between two floating-point numbers.
    // It can be triggered with 1e23.
    // Note: because the factor_mantissa and factor_mantissa_low are
    // almost always rounded down (except for small positive powers),
    // almost always should round up.
    return false;
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    lz--; // undo previous addition
  }
  mantissa &= ~(1ULL << 52);
  uint64_t real_exponent = exponent - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    return false;
  }
  mantissa |= real_exponent << 52;
  mantissa |= (((uint64_t)negative) << 63);
  memcpy(&d, &mantissa, sizeof(d));
  return true;
}

static bool parse_float_strtod(const uint8_t *ptr, double *outDouble) {
  char *endptr;
  *outDouble = strtod((const char *)ptr, &endptr);
  // Some libraries will set errno = ERANGE when the value is subnormal,
  // yet we may want to be able to parse subnormal values.
  // However, we do not want to tolerate NAN or infinite values.
  //
  // Values like infinity or NaN are not allowed in the JSON specification.
  // If you consume a large value and you map it to "infinity", you will no
  // longer be able to serialize back a standard-compliant JSON. And there is
  // no realistic application where you might need values so large than they
  // can't fit in binary64. The maximal value is about  1.7976931348623157 x
  // 10^308 It is an unimaginable large number. There will never be any piece of
  // engineering involving as many as 10^308 parts. It is estimated that there
  // are about 10^80 atoms in the universe.  The estimate for the total number
  // of electrons is similar. Using a double-precision floating-point value, we
  // can represent easily the number of atoms in the universe. We could  also
  // represent the number of ways you can pick any three individual atoms at
  // random in the universe. If you ever encounter a number much larger than
  // 10^308, you know that you have a bug. RapidJSON will reject a document with
  // a float that does not fit in binary64. JSON for Modern C++ (nlohmann/json)
  // will flat out throw an exception.
  //
  if ((endptr == (const char *)ptr) || (!std::isfinite(*outDouble))) {
    return false;
  }
  return true;
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(SIMDJSON_UNUSED const uint8_t * src, W writer) {
  double d;
  if (parse_float_strtod(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow. 
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off 
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do 
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time. 
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in 
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline int significant_digits(const uint8_t * start_digits, int digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - int(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, int digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  // digit count is off by 1 because of the decimal (assuming there was one).
  if (simdjson_unlikely(digit_count-1 >= 19 && significant_digits(start_digits, digit_count) >= 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < FASTFLOAT_SMALLEST_POWER) || (exponent > FASTFLOAT_LARGEST_POWER)) {
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_strtod(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool clean_end = is_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (!clean_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// SAX functions
namespace {
// Parse any number from 0 to 18,446,744,073,709,551,615
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  if (digit_count > 20) { return NUMBER_ERROR; }
  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return i;
}

// Parse any number from 0 to 18,446,744,073,709,551,615
// Call this version of the method if you regularly expect 8- or 16-digit numbers.
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_large_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
      if (parse_digit(*p, i)) { // digit 17
        p++;
        if (parse_digit(*p, i)) { // digit 18
          p++;
          if (parse_digit(*p, i)) { // digit 19
            p++;
            if (parse_digit(*p, i)) { // digit 20
              p++;
              if (parse_digit(*p, i)) { return NUMBER_ERROR; } // 21 digits is an error
              // Positive overflow check:
              // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
              //   biggest uint64_t.
              // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
              //   If we got here, it's a 20 digit number starting with the digit "1".
              // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
              //   than 1,553,255,926,290,448,384.
              // - That is smaller than the smallest possible 20-digit number the user could write:
              //   10,000,000,000,000,000,000.
              // - Therefore, if the number is positive and lower than that, it's overflow.
              // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
              //
              if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
            }
          }
        }
      }
    } // 16 digits
  } else { // 8 digits
    // Less than 8 digits can't overflow, simpler logic here.
    if (parse_digit(*p, i)) { p++; } else { return NUMBER_ERROR; }
    while (parse_digit(*p, i)) { p++; }
  }

  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }
  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - src);
  if (digit_count == 0 || ('0' == *src && digit_count > 1)) { return NUMBER_ERROR; }
  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return NUMBER_ERROR; }
  if (digit_count == longest_digit_count) {
    if(negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return NUMBER_ERROR; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return negative ? (~i+1) : i;
}

SIMDJSON_UNUSED simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src || (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 >= 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src >= 19;
    }
  } else {
    overflow = p-src >= 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
    overflow = overflow || exponent < FASTFLOAT_SMALLEST_POWER || exponent > FASTFLOAT_LARGEST_POWER;
  }

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_strtod(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/numberparsing.h */

#endif // SIMDJSON_FALLBACK_NUMBERPARSING_H
/* end file src/generic/stage2/numberparsing.h */
/* begin file src/generic/stage2/tape_builder.h */
/* begin file src/generic/stage2/json_iterator.h */
/* begin file src/generic/stage2/logger.h */
// This is for an internal-only stage 2 specific logger.
// Set LOG_ENABLED = true to log what stage 2 is doing!
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace logger {

  static constexpr const char * DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";

  static constexpr const bool LOG_ENABLED = false;
  static constexpr const int LOG_EVENT_LEN = 20;
  static constexpr const int LOG_BUFFER_LEN = 30;
  static constexpr const int LOG_SMALL_BUFFER_LEN = 10;
  static constexpr const int LOG_INDEX_LEN = 5;

  static int log_depth; // Not threadsafe. Log only.

  // Helper to turn unprintable or newline characters into spaces
  static simdjson_really_inline char printable_char(char c) {
    if (c >= 0x20) {
      return c;
    } else {
      return ' ';
    }
  }

  // Print the header and set up log_start
  static simdjson_really_inline void log_start() {
    if (LOG_ENABLED) {
      log_depth = 0;
      printf("\n");
      printf("| %-*s | %-*s | %-*s | %-*s | Detail |\n", LOG_EVENT_LEN, "Event", LOG_BUFFER_LEN, "Buffer", LOG_SMALL_BUFFER_LEN, "Next", 5, "Next#");
      printf("|%.*s|%.*s|%.*s|%.*s|--------|\n", LOG_EVENT_LEN+2, DASHES, LOG_BUFFER_LEN+2, DASHES, LOG_SMALL_BUFFER_LEN+2, DASHES, 5+2, DASHES);
    }
  }

  // Logs a single line of 
  template<typename S>
  static simdjson_really_inline void log_line(S &structurals, const char *title_prefix, const char *title, const char *detail) {
    if (LOG_ENABLED) {
      printf("| %*s%s%-*s ", log_depth*2, "", title_prefix, LOG_EVENT_LEN - log_depth*2 - int(strlen(title_prefix)), title);
      auto current_index = structurals.at_beginning() ? nullptr : structurals.next_structural-1;
      auto next_index = structurals.next_structural;
      auto current = current_index ? &structurals.buf[*current_index] : (const uint8_t*)"                                                       ";
      auto next = &structurals.buf[*next_index];
      {
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_BUFFER_LEN;i++) {
          printf("%c", printable_char(current[i]));
        }
        printf(" ");
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_SMALL_BUFFER_LEN;i++) {
          printf("%c", printable_char(next[i]));
        }
        printf(" ");
      }
      if (current_index) {
        printf("| %*u ", LOG_INDEX_LEN, *current_index);
      } else {
        printf("| %-*s ", LOG_INDEX_LEN, "");
      }
      // printf("| %*u ", LOG_INDEX_LEN, structurals.next_tape_index());
      printf("| %-s ", detail);
      printf("|\n");
    }
  }

} // namespace logger
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/logger.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

class json_iterator {
public:
  const uint8_t* const buf;
  uint32_t *next_structural;
  dom_parser_implementation &dom_parser;
  uint32_t depth{0};

  /**
   * Walk the JSON document.
   *
   * The visitor receives callbacks when values are encountered. All callbacks pass the iterator as
   * the first parameter; some callbacks have other parameters as well:
   *
   * - visit_document_start() - at the beginning.
   * - visit_document_end() - at the end (if things were successful).
   *
   * - visit_array_start() - at the start `[` of a non-empty array.
   * - visit_array_end() - at the end `]` of a non-empty array.
   * - visit_empty_array() - when an empty array is encountered.
   *
   * - visit_object_end() - at the start `]` of a non-empty object.
   * - visit_object_start() - at the end `]` of a non-empty object.
   * - visit_empty_object() - when an empty object is encountered.
   * - visit_key(const uint8_t *key) - when a key in an object field is encountered. key is
   *                                   guaranteed to point at the first quote of the string (`"key"`).
   * - visit_primitive(const uint8_t *value) - when a value is a string, number, boolean or null.
   * - visit_root_primitive(iter, uint8_t *value) - when the top-level value is a string, number, boolean or null.
   *
   * - increment_count(iter) - each time a value is found in an array or object.
   */
  template<bool STREAMING, typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code walk_document(V &visitor) noexcept;

  /**
   * Create an iterator capable of walking a JSON document.
   *
   * The document must have already passed through stage 1.
   */
  simdjson_really_inline json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index);

  /**
   * Look at the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *peek() const noexcept;
  /**
   * Advance to the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *advance() noexcept;
  /**
   * Get the remaining length of the document, from the start of the current token.
   */
  simdjson_really_inline size_t remaining_len() const noexcept;
  /**
   * Check if we are at the end of the document.
   *
   * If this is true, there are no more tokens.
   */
  simdjson_really_inline bool at_eof() const noexcept;
  /**
   * Check if we are at the beginning of the document.
   */
  simdjson_really_inline bool at_beginning() const noexcept;
  simdjson_really_inline uint8_t last_structural() const noexcept;

  /**
   * Log that a value has been found.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_value(const char *type) const noexcept;
  /**
   * Log the start of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_start_value(const char *type) const noexcept;
  /**
   * Log the end of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_end_value(const char *type) const noexcept;
  /**
   * Log an error.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_error(const char *error) const noexcept;

  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_primitive(V &visitor, const uint8_t *value) noexcept;
  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_primitive(V &visitor, const uint8_t *value) noexcept;
};

template<bool STREAMING, typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::walk_document(V &visitor) noexcept {
  logger::log_start();

  //
  // Start the document
  //
  if (at_eof()) { return EMPTY; }
  log_start_value("document");
  SIMDJSON_TRY( visitor.visit_document_start(*this) );

  //
  // Read first value
  //
  {
    auto value = advance();

    // Make sure the outer hash or array is closed before continuing; otherwise, there are ways we
    // could get into memory corruption. See https://github.com/simdjson/simdjson/issues/906
    if (!STREAMING) {
      switch (*value) {
        case '{': if (last_structural() != '}') { return TAPE_ERROR; }; break;
        case '[': if (last_structural() != ']') { return TAPE_ERROR; }; break;
      }
    }

    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_root_primitive(*this, value) ); break;
    }
  }
  goto document_end;

//
// Object parser states
//
object_begin:
  log_start_value("object");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = false;
  SIMDJSON_TRY( visitor.visit_object_start(*this) );

  {
    auto key = advance();
    if (*key != '"') { log_error("Object does not start with a key"); return TAPE_ERROR; }
    SIMDJSON_TRY( visitor.increment_count(*this) );
    SIMDJSON_TRY( visitor.visit_key(*this, key) );
  }

object_field:
  if (simdjson_unlikely( *advance() != ':' )) { log_error("Missing colon after key in object"); return TAPE_ERROR; }
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

object_continue:
  switch (*advance()) {
    case ',':
      SIMDJSON_TRY( visitor.increment_count(*this) );
      {
        auto key = advance();
        if (simdjson_unlikely( *key != '"' )) { log_error("Key string missing at beginning of field in object"); return TAPE_ERROR; }
        SIMDJSON_TRY( visitor.visit_key(*this, key) );
      }
      goto object_field;
    case '}': log_end_value("object"); SIMDJSON_TRY( visitor.visit_object_end(*this) ); goto scope_end;
    default: log_error("No comma between object fields"); return TAPE_ERROR;
  }

scope_end:
  depth--;
  if (depth == 0) { goto document_end; }
  if (dom_parser.is_array[depth]) { goto array_continue; }
  goto object_continue;

//
// Array parser states
//
array_begin:
  log_start_value("array");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = true;
  SIMDJSON_TRY( visitor.visit_array_start(*this) );
  SIMDJSON_TRY( visitor.increment_count(*this) );

array_value:
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

array_continue:
  switch (*advance()) {
    case ',': SIMDJSON_TRY( visitor.increment_count(*this) ); goto array_value;
    case ']': log_end_value("array"); SIMDJSON_TRY( visitor.visit_array_end(*this) ); goto scope_end;
    default: log_error("Missing comma between array values"); return TAPE_ERROR;
  }

document_end:
  log_end_value("document");
  SIMDJSON_TRY( visitor.visit_document_end(*this) );

  dom_parser.next_structural_index = uint32_t(next_structural - &dom_parser.structural_indexes[0]);

  // If we didn't make it to the end, it's an error
  if ( !STREAMING && dom_parser.next_structural_index != dom_parser.n_structural_indexes ) {
    log_error("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
    return TAPE_ERROR;
  }

  return SUCCESS;

} // walk_document()

simdjson_really_inline json_iterator::json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index)
  : buf{_dom_parser.buf},
    next_structural{&_dom_parser.structural_indexes[start_structural_index]},
    dom_parser{_dom_parser} {
}

simdjson_really_inline const uint8_t *json_iterator::peek() const noexcept {
  return &buf[*(next_structural)];
}
simdjson_really_inline const uint8_t *json_iterator::advance() noexcept {
  return &buf[*(next_structural++)];
}
simdjson_really_inline size_t json_iterator::remaining_len() const noexcept {
  return dom_parser.len - *(next_structural-1);
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return next_structural == &dom_parser.structural_indexes[dom_parser.n_structural_indexes];
}
simdjson_really_inline bool json_iterator::at_beginning() const noexcept {
  return next_structural == dom_parser.structural_indexes.get();
}
simdjson_really_inline uint8_t json_iterator::last_structural() const noexcept {
  return buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]];
}

simdjson_really_inline void json_iterator::log_value(const char *type) const noexcept {
  logger::log_line(*this, "", type, "");
}

simdjson_really_inline void json_iterator::log_start_value(const char *type) const noexcept {
  logger::log_line(*this, "+", type, "");
  if (logger::LOG_ENABLED) { logger::log_depth++; }
}

simdjson_really_inline void json_iterator::log_end_value(const char *type) const noexcept {
  if (logger::LOG_ENABLED) { logger::log_depth--; }
  logger::log_line(*this, "-", type, "");
}

simdjson_really_inline void json_iterator::log_error(const char *error) const noexcept {
  logger::log_line(*this, "", "ERROR", error);
}

template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_root_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_root_string(*this, value);
    case 't': return visitor.visit_root_true_atom(*this, value);
    case 'f': return visitor.visit_root_false_atom(*this, value);
    case 'n': return visitor.visit_root_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_root_number(*this, value);
    default:
      log_error("Document starts with a non-value character");
      return TAPE_ERROR;
  }
}
template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_string(*this, value);
    case 't': return visitor.visit_true_atom(*this, value);
    case 'f': return visitor.visit_false_atom(*this, value);
    case 'n': return visitor.visit_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_number(*this, value);
    default:
      log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/logger.h */
/* begin file src/generic/stage2/tape_writer.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_writer {
  /** The next place to write to tape */
  uint64_t *next_tape_loc;
  
  /** Write a signed 64-bit value to tape. */
  simdjson_really_inline void append_s64(int64_t value) noexcept;

  /** Write an unsigned 64-bit value to tape. */
  simdjson_really_inline void append_u64(uint64_t value) noexcept;

  /** Write a double value to tape. */
  simdjson_really_inline void append_double(double value) noexcept;

  /**
   * Append a tape entry (an 8-bit type,and 56 bits worth of value).
   */
  simdjson_really_inline void append(uint64_t val, internal::tape_type t) noexcept;

  /**
   * Skip the current tape entry without writing.
   *
   * Used to skip the start of the container, since we'll come back later to fill it in when the
   * container ends.
   */
  simdjson_really_inline void skip() noexcept;

  /**
   * Skip the number of tape entries necessary to write a large u64 or i64.
   */
  simdjson_really_inline void skip_large_integer() noexcept;

  /**
   * Skip the number of tape entries necessary to write a double.
   */
  simdjson_really_inline void skip_double() noexcept;

  /**
   * Write a value to a known location on tape.
   *
   * Used to go back and write out the start of a container after the container ends.
   */
  simdjson_really_inline static void write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept;

private:
  /**
   * Append both the tape entry, and a supplementary value following it. Used for types that need
   * all 64 bits, such as double and uint64_t.
   */
  template<typename T>
  simdjson_really_inline void append2(uint64_t val, T val2, internal::tape_type t) noexcept;
}; // struct number_writer

simdjson_really_inline void tape_writer::append_s64(int64_t value) noexcept {
  append2(0, value, internal::tape_type::INT64);
}

simdjson_really_inline void tape_writer::append_u64(uint64_t value) noexcept {
  append(0, internal::tape_type::UINT64);
  *next_tape_loc = value;
  next_tape_loc++;
}

/** Write a double value to tape. */
simdjson_really_inline void tape_writer::append_double(double value) noexcept {
  append2(0, value, internal::tape_type::DOUBLE);
}

simdjson_really_inline void tape_writer::skip() noexcept {
  next_tape_loc++;
}

simdjson_really_inline void tape_writer::skip_large_integer() noexcept {
  next_tape_loc += 2;
}

simdjson_really_inline void tape_writer::skip_double() noexcept {
  next_tape_loc += 2;
}

simdjson_really_inline void tape_writer::append(uint64_t val, internal::tape_type t) noexcept {
  *next_tape_loc = val | ((uint64_t(char(t))) << 56);
  next_tape_loc++;
}

template<typename T>
simdjson_really_inline void tape_writer::append2(uint64_t val, T val2, internal::tape_type t) noexcept {
  append(val, t);
  static_assert(sizeof(val2) == sizeof(*next_tape_loc), "Type is not 64 bits!");
  memcpy(next_tape_loc, &val2, sizeof(val2));
  next_tape_loc++;
}

simdjson_really_inline void tape_writer::write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept {
  tape_loc = val | ((uint64_t(char(t))) << 56);
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/tape_writer.h */
/* begin file src/generic/stage2/atomparsing.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_really_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
SIMDJSON_WARN_UNUSED
simdjson_really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | is_not_structural_or_whitespace(src[4])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | is_not_structural_or_whitespace(src[5])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | is_not_structural_or_whitespace(src[4])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/atomparsing.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_builder {
  template<bool STREAMING>
  SIMDJSON_WARN_UNUSED static simdjson_really_inline error_code parse_document(
    dom_parser_implementation &dom_parser,
    dom::document &doc) noexcept;

  /** Called when a non-empty document starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_document_start(json_iterator &iter) noexcept;
  /** Called when a non-empty document ends without error. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_document_end(json_iterator &iter) noexcept;

  /** Called when a non-empty array starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_array_start(json_iterator &iter) noexcept;
  /** Called when a non-empty array ends. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_array_end(json_iterator &iter) noexcept;
  /** Called when an empty array is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_empty_array(json_iterator &iter) noexcept;

  /** Called when a non-empty object starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_object_start(json_iterator &iter) noexcept;
  /**
   * Called when a key in a field is encountered.
   *
   * primitive, visit_object_start, visit_empty_object, visit_array_start, or visit_empty_array
   * will be called after this with the field value.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_key(json_iterator &iter, const uint8_t *key) noexcept;
  /** Called when a non-empty object ends. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_object_end(json_iterator &iter) noexcept;
  /** Called when an empty object is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_empty_object(json_iterator &iter) noexcept;

  /**
   * Called when a string, number, boolean or null is found.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_primitive(json_iterator &iter, const uint8_t *value) noexcept;
  /**
   * Called when a string, number, boolean or null is found at the top level of a document (i.e.
   * when there is no array or object and the entire document is a single string, number, boolean or
   * null.
   *
   * This is separate from primitive() because simdjson's normal primitive parsing routines assume
   * there is at least one more token after the value, which is only true in an array or object.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_string(json_iterator &iter, const uint8_t *value, bool key = false) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_number(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_string(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_number(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

  /** Called each time a new field or element in an array or object is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code increment_count(json_iterator &iter) noexcept;

  /** Next location to write to tape */
  tape_writer tape;
private:
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc;

  simdjson_really_inline tape_builder(dom::document &doc) noexcept;

  simdjson_really_inline uint32_t next_tape_index(json_iterator &iter) const noexcept;
  simdjson_really_inline void start_container(json_iterator &iter) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  simdjson_really_inline uint8_t *on_start_string(json_iterator &iter) noexcept;
  simdjson_really_inline void on_end_string(uint8_t *dst) noexcept;
}; // class tape_builder

template<bool STREAMING>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::parse_document(
    dom_parser_implementation &dom_parser,
    dom::document &doc) noexcept {
  dom_parser.doc = &doc;
  json_iterator iter(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
  tape_builder builder(doc);
  return iter.walk_document<STREAMING>(builder);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_root_primitive(*this, value);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_primitive(*this, value);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_empty_object(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_empty_array(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_document_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_object_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_array_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_object_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_array_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_document_end(json_iterator &iter) noexcept {
  constexpr uint32_t start_tape_index = 0;
  tape.append(start_tape_index, internal::tape_type::ROOT);
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter), internal::tape_type::ROOT);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_key(json_iterator &iter, const uint8_t *key) noexcept {
  return visit_string(iter, key, true);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::increment_count(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].count++; // we have a key value pair in the object at parser.dom_parser.depth - 1
  return SUCCESS;
}

simdjson_really_inline tape_builder::tape_builder(dom::document &doc) noexcept : tape{doc.tape.get()}, current_string_buf_loc{doc.string_buf.get()} {}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_string(json_iterator &iter, const uint8_t *value, bool key) noexcept {
  iter.log_value(key ? "key" : "string");
  uint8_t *dst = on_start_string(iter);
  dst = stringparsing::parse_string(value, dst);
  if (dst == nullptr) {
    iter.log_error("Invalid escape in string");
    return STRING_ERROR;
  }
  on_end_string(dst);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_string(json_iterator &iter, const uint8_t *value) noexcept {
  return visit_string(iter, value);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_number(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("number");
  return numberparsing::parse_number(value, tape);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_number(json_iterator &iter, const uint8_t *value) noexcept {
  //
  // We need to make a copy to make sure that the string is space terminated.
  // This is not about padding the input, which should already padded up
  // to len + SIMDJSON_PADDING. However, we have no control at this stage
  // on how the padding was done. What if the input string was padded with nulls?
  // It is quite common for an input string to have an extra null character (C string).
  // We do not want to allow 9\0 (where \0 is the null character) inside a JSON
  // document, but the string "9\0" by itself is fine. So we make a copy and
  // pad the input with spaces when we know that there is just one input element.
  // This copy is relatively expensive, but it will almost never be called in
  // practice unless you are in the strange scenario where you have many JSON
  // documents made of single atoms.
  //
  uint8_t *copy = static_cast<uint8_t *>(malloc(iter.remaining_len() + SIMDJSON_PADDING));
  if (copy == nullptr) { return MEMALLOC; }
  memcpy(copy, value, iter.remaining_len());
  memset(copy + iter.remaining_len(), ' ', SIMDJSON_PADDING);
  error_code error = visit_number(iter, copy);
  free(copy);
  return error;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  if (!atomparsing::is_valid_true_atom(value)) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  if (!atomparsing::is_valid_true_atom(value, iter.remaining_len())) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  if (!atomparsing::is_valid_false_atom(value)) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  if (!atomparsing::is_valid_false_atom(value, iter.remaining_len())) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  if (!atomparsing::is_valid_null_atom(value)) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  if (!atomparsing::is_valid_null_atom(value, iter.remaining_len())) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

// private:

simdjson_really_inline uint32_t tape_builder::next_tape_index(json_iterator &iter) const noexcept {
  return uint32_t(tape.next_tape_loc - iter.dom_parser.doc->tape.get());
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  auto start_index = next_tape_index(iter);
  tape.append(start_index+2, start);
  tape.append(start_index, end);
  return SUCCESS;
}

simdjson_really_inline void tape_builder::start_container(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].tape_index = next_tape_index(iter);
  iter.dom_parser.open_containers[iter.depth].count = 0;
  tape.skip(); // We don't actually *write* the start element until the end.
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  // Write the ending tape element, pointing at the start location
  const uint32_t start_tape_index = iter.dom_parser.open_containers[iter.depth].tape_index;
  tape.append(start_tape_index, end);
  // Write the start tape element, pointing at the end location (and including count)
  // count can overflow if it exceeds 24 bits... so we saturate
  // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
  const uint32_t count = iter.dom_parser.open_containers[iter.depth].count;
  const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter) | (uint64_t(cntsat) << 32), start);
  return SUCCESS;
}

simdjson_really_inline uint8_t *tape_builder::on_start_string(json_iterator &iter) noexcept {
  // we advance the point, accounting for the fact that we have a NULL termination
  tape.append(current_string_buf_loc - iter.dom_parser.doc->string_buf.get(), internal::tape_type::STRING);
  return current_string_buf_loc + sizeof(uint32_t);
}

simdjson_really_inline void tape_builder::on_end_string(uint8_t *dst) noexcept {
  uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
  // TODO check for overflow in case someone has a crazy string (>=4GB?)
  // But only add the overflow check when the document itself exceeds 4GB
  // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
  memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
  // NULL termination is still handy if you expect all your strings to
  // be NULL terminated? It comes at a small cost
  *dst = 0;
  current_string_buf_loc = dst + 1;
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/atomparsing.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, false);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/fallback/end_implementation.h */
#undef SIMDJSON_IMPLEMENTATION
/* end file src/fallback/end_implementation.h */
/* end file src/fallback/end_implementation.h */
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
/* begin file src/haswell/implementation.cpp */
/* begin file src/haswell/begin_implementation.h */
#define SIMDJSON_IMPLEMENTATION haswell
#define SIMDJSON_TARGET_HASWELL SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt")

/* haswell/implementation.h already included: #include "haswell/implementation.h" */
/* begin file src/haswell/intrinsics.h */
#ifndef SIMDJSON_HASWELL_INTRINSICS_H
#define SIMDJSON_HASWELL_INTRINSICS_H


#ifdef SIMDJSON_VISUAL_STUDIO
// under clang within visual studio, this will include <x86intrin.h>
#include <intrin.h>  // visual studio or clang
#else
#include <x86intrin.h> // elsewhere
#endif // SIMDJSON_VISUAL_STUDIO

#ifdef SIMDJSON_CLANG_VISUAL_STUDIO
/**
 * You are not supposed, normally, to include these
 * headers directly. Instead you should either include intrin.h
 * or x86intrin.h. However, when compiling with clang
 * under Windows (i.e., when _MSC_VER is set), these headers
 * only get included *if* the corresponding features are detected
 * from macros:
 * e.g., if __AVX2__ is set... in turn,  we normally set these
 * macros by compiling against the corresponding architecture
 * (e.g., arch:AVX2, -mavx2, etc.) which compiles the whole
 * software with these advanced instructions. In simdjson, we
 * want to compile the whole program for a generic target,
 * and only target our specific kernels. As a workaround,
 * we directly include the needed headers. These headers would
 * normally guard against such usage, but we carefully included
 * <x86intrin.h>  (or <intrin.h>) before, so the headers
 * are fooled.
 */
#include <bmiintrin.h>   // for _blsr_u64
#include <lzcntintrin.h> // for  __lzcnt64
#include <immintrin.h>   // for most things (AVX2, AVX512, _popcnt64)
#include <smmintrin.h>
#include <tmmintrin.h>
#include <avxintrin.h>
#include <avx2intrin.h>
#include <wmmintrin.h>   // for  _mm_clmulepi64_si128
// unfortunately, we may not get _blsr_u64, but, thankfully, clang
// has it as a macro.
#ifndef _blsr_u64
// we roll our own
SIMDJSON_TARGET_HASWELL
static simdjson_really_inline uint64_t _blsr_u64(uint64_t n) {
  return (n - 1) & n;
}
SIMDJSON_UNTARGET_REGION
#endif //  _blsr_u64
#endif // SIMDJSON_CLANG_VISUAL_STUDIO

#endif // SIMDJSON_HASWELL_INTRINSICS_H
/* end file src/haswell/intrinsics.h */

SIMDJSON_TARGET_HASWELL

/* begin file src/haswell/bitmanipulation.h */
#ifndef SIMDJSON_HASWELL_BITMANIPULATION_H
#define SIMDJSON_HASWELL_BITMANIPULATION_H

namespace {
namespace SIMDJSON_IMPLEMENTATION {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
NO_SANITIZE_UNDEFINED
simdjson_really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  return (int)_tzcnt_u64(input_num);
#else // SIMDJSON_REGULAR_VISUAL_STUDIO
  ////////
  // You might expect the next line to be equivalent to 
  // return (int)_tzcnt_u64(input_num);
  // but the generated code differs and might be less efficient?
  ////////
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return _blsr_u64(input_num);
}

/* result might be undefined when input_num is zero */
simdjson_really_inline int leading_zeroes(uint64_t input_num) {
  return int(_lzcnt_u64(input_num));
}

#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
simdjson_really_inline unsigned __int64 count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num);// Visual Studio wants two underscores
}
#else
simdjson_really_inline long long int count_ones(uint64_t input_num) {
  return _popcnt64(input_num);
}
#endif

simdjson_really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#endif // SIMDJSON_HASWELL_BITMANIPULATION_H
/* end file src/haswell/bitmanipulation.h */
/* begin file src/haswell/bitmask.h */
#ifndef SIMDJSON_HASWELL_BITMASK_H
#define SIMDJSON_HASWELL_BITMASK_H

namespace {
namespace SIMDJSON_IMPLEMENTATION {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_really_inline uint64_t prefix_xor(const uint64_t bitmask) {
  // There should be no such thing with a processor supporting avx2
  // but not clmul.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return _mm_cvtsi128_si64(result);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#endif // SIMDJSON_HASWELL_BITMASK_H
/* end file src/haswell/bitmask.h */
/* begin file src/haswell/simd.h */
#ifndef SIMDJSON_HASWELL_SIMD_H
#define SIMDJSON_HASWELL_SIMD_H

/* simdprune_tables.h already included: #include "simdprune_tables.h" */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace simd {

  // Forward-declared so they can be used by splat and friends.
  template<typename Child>
  struct base {
    __m256i value;

    // Zero constructor
    simdjson_really_inline base() : value{__m256i()} {}

    // Conversion from SIMD register
    simdjson_really_inline base(const __m256i _value) : value(_value) {}

    // Conversion to SIMD register
    simdjson_really_inline operator const __m256i&() const { return this->value; }
    simdjson_really_inline operator __m256i&() { return this->value; }

    // Bit operations
    simdjson_really_inline Child operator|(const Child other) const { return _mm256_or_si256(*this, other); }
    simdjson_really_inline Child operator&(const Child other) const { return _mm256_and_si256(*this, other); }
    simdjson_really_inline Child operator^(const Child other) const { return _mm256_xor_si256(*this, other); }
    simdjson_really_inline Child bit_andnot(const Child other) const { return _mm256_andnot_si256(other, *this); }
    simdjson_really_inline Child& operator|=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast | other; return *this_cast; }
    simdjson_really_inline Child& operator&=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast & other; return *this_cast; }
    simdjson_really_inline Child& operator^=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    typedef uint32_t bitmask_t;
    typedef uint64_t bitmask2_t;

    simdjson_really_inline base8() : base<simd8<T>>() {}
    simdjson_really_inline base8(const __m256i _value) : base<simd8<T>>(_value) {}

    simdjson_really_inline Mask operator==(const simd8<T> other) const { return _mm256_cmpeq_epi8(*this, other); }

    static const int SIZE = sizeof(base<T>::value);

    template<int N=1>
    simdjson_really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return _mm256_alignr_epi8(*this, _mm256_permute2x128_si256(prev_chunk, *this, 0x21), 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static simdjson_really_inline simd8<bool> splat(bool _value) { return _mm256_set1_epi8(uint8_t(-(!!_value))); }

    simdjson_really_inline simd8<bool>() : base8() {}
    simdjson_really_inline simd8<bool>(const __m256i _value) : base8<bool>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8<bool>(bool _value) : base8<bool>(splat(_value)) {}

    simdjson_really_inline int to_bitmask() const { return _mm256_movemask_epi8(*this); }
    simdjson_really_inline bool any() const { return !_mm256_testz_si256(*this, *this); }
    simdjson_really_inline simd8<bool> operator~() const { return *this ^ true; }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static simdjson_really_inline simd8<T> splat(T _value) { return _mm256_set1_epi8(_value); }
    static simdjson_really_inline simd8<T> zero() { return _mm256_setzero_si256(); }
    static simdjson_really_inline simd8<T> load(const T values[32]) {
      return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(values));
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static simdjson_really_inline simd8<T> repeat_16(
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

    simdjson_really_inline base8_numeric() : base8<T>() {}
    simdjson_really_inline base8_numeric(const __m256i _value) : base8<T>(_value) {}

    // Store to array
    simdjson_really_inline void store(T dst[32]) const { return _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), *this); }

    // Addition/subtraction are the same for signed and unsigned
    simdjson_really_inline simd8<T> operator+(const simd8<T> other) const { return _mm256_add_epi8(*this, other); }
    simdjson_really_inline simd8<T> operator-(const simd8<T> other) const { return _mm256_sub_epi8(*this, other); }
    simdjson_really_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *(simd8<T>*)this; }
    simdjson_really_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *(simd8<T>*)this; }

    // Override to distinguish from bool version
    simdjson_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return _mm256_shuffle_epi8(lookup_table, *this);
    }

    // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted as a bitset).
    // Passing a 0 value for mask would be equivalent to writing out every byte to output.
    // Only the first 32 - count_ones(mask) bytes of the result are significant but 32 bytes
    // get written.
    // Design consideration: it seems like a function with the
    // signature simd8<L> compress(uint32_t mask) would be
    // sensible, but the AVX ISA makes this kind of approach difficult.
    template<typename L>
    simdjson_really_inline void compress(uint32_t mask, L * output) const {
      // this particular implementation was inspired by work done by @animetosho
      // we do it in four steps, first 8 bytes and then second 8 bytes...
      uint8_t mask1 = uint8_t(mask); // least significant 8 bits
      uint8_t mask2 = uint8_t(mask >> 8); // second least significant 8 bits
      uint8_t mask3 = uint8_t(mask >> 16); // ...
      uint8_t mask4 = uint8_t(mask >> 24); // ...
      // next line just loads the 64-bit values thintable_epi8[mask1] and
      // thintable_epi8[mask2] into a 128-bit register, using only
      // two instructions on most compilers.
      __m256i shufmask =  _mm256_set_epi64x(thintable_epi8[mask4], thintable_epi8[mask3], 
        thintable_epi8[mask2], thintable_epi8[mask1]);
      // we increment by 0x08 the second half of the mask and so forth
      shufmask =
      _mm256_add_epi8(shufmask, _mm256_set_epi32(0x18181818, 0x18181818, 
         0x10101010, 0x10101010, 0x08080808, 0x08080808, 0, 0));
      // this is the version "nearly pruned"
      __m256i pruned = _mm256_shuffle_epi8(*this, shufmask);
      // we still need to put the  pieces back together.
      // we compute the popcount of the first words:
      int pop1 = BitsSetTable256mul2[mask1];
      int pop3 = BitsSetTable256mul2[mask3];

      // then load the corresponding mask
      // could be done with _mm256_loadu2_m128i but many standard libraries omit this intrinsic.
      __m256i v256 = _mm256_castsi128_si256(
        _mm_loadu_si128((const __m128i *)(pshufb_combine_table + pop1 * 8)));
      __m256i compactmask = _mm256_insertf128_si256(v256,
         _mm_loadu_si128((const __m128i *)(pshufb_combine_table + pop3 * 8)), 1);
      __m256i almostthere =  _mm256_shuffle_epi8(pruned, compactmask);
      // We just need to write out the result.
      // This is the tricky bit that is hard to do
      // if we want to return a SIMD register, since there
      // is no single-instruction approach to recombine
      // the two 128-bit lanes with an offset.
      __m128i v128;
      v128 = _mm256_castsi256_si128(almostthere);
      _mm_storeu_si128( (__m128i *)output, v128);
      v128 = _mm256_extractf128_si256(almostthere, 1);
      _mm_storeu_si128( (__m128i *)(output + 16 - count_ones(mask & 0xFFFF)), v128);
    }

    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(
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
    simdjson_really_inline simd8() : base8_numeric<int8_t>() {}
    simdjson_really_inline simd8(const __m256i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const int8_t values[32]) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
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
    simdjson_really_inline static simd8<int8_t> repeat_16(
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
    simdjson_really_inline simd8<int8_t> max(const simd8<int8_t> other) const { return _mm256_max_epi8(*this, other); }
    simdjson_really_inline simd8<int8_t> min(const simd8<int8_t> other) const { return _mm256_min_epi8(*this, other); }
    simdjson_really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return _mm256_cmpgt_epi8(*this, other); }
    simdjson_really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return _mm256_cmpgt_epi8(other, *this); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    simdjson_really_inline simd8() : base8_numeric<uint8_t>() {}
    simdjson_really_inline simd8(const __m256i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const uint8_t values[32]) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
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
    simdjson_really_inline static simd8<uint8_t> repeat_16(
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
    simdjson_really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return _mm256_adds_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return _mm256_subs_epu8(*this, other); }

    // Order-specific operations
    simdjson_really_inline simd8<uint8_t> max(const simd8<uint8_t> other) const { return _mm256_max_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> min(const simd8<uint8_t> other) const { return _mm256_min_epu8(other, *this); }
    // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return this->saturating_sub(other); }
    // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return other.saturating_sub(*this); }
    simdjson_really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return other.max(*this) == other; }
    simdjson_really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return other.min(*this) == other; }
    simdjson_really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }
    simdjson_really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return this->lt_bits(other).any_bits_set(); }

    // Bit-specific operations
    simdjson_really_inline simd8<bool> bits_not_set() const { return *this == uint8_t(0); }
    simdjson_really_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const { return (*this & bits).bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set() const { return ~this->bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return ~this->bits_not_set(bits); }
    simdjson_really_inline bool is_ascii() const { return _mm256_movemask_epi8(*this) == 0; }
    simdjson_really_inline bool bits_not_set_anywhere() const { return _mm256_testz_si256(*this, *this); }
    simdjson_really_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    simdjson_really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const { return _mm256_testz_si256(*this, bits); }
    simdjson_really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(_mm256_srli_epi16(*this, N)) & uint8_t(0xFFu >> N); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(_mm256_slli_epi16(*this, N)) & uint8_t(0xFFu << N); }
    // Get one of the bits and make a bitmask out of it.
    // e.g. value.get_bit<7>() gets the high bit
    template<int N>
    simdjson_really_inline int get_bit() const { return _mm256_movemask_epi8(_mm256_slli_epi16(*this, 7-N)); }
  };

  template<typename T>
  struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 2, "Haswell kernel should use two registers per 64-byte block.");
    const simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T>& o) = delete; // no copy allowed
    simd8x64<T>& operator=(const simd8<T> other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed

    simdjson_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1) : chunks{chunk0, chunk1} {}
    simdjson_really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+32)} {}

    simdjson_really_inline void compress(uint64_t mask, T * output) const {
      uint32_t mask1 = uint32_t(mask);
      uint32_t mask2 = uint32_t(mask >> 32);
      this->chunks[0].compress(mask1, output);
      this->chunks[1].compress(mask2, output + 32 - count_ones(mask1));
    }

    simdjson_really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
    }

    simdjson_really_inline uint64_t to_bitmask() const {
      uint64_t r_lo = uint32_t(this->chunks[0].to_bitmask());
      uint64_t r_hi =                       this->chunks[1].to_bitmask();
      return r_lo | (r_hi << 32);
    }

    simdjson_really_inline simd8<T> reduce_or() const {
      return this->chunks[0] | this->chunks[1];
    }

    simdjson_really_inline simd8x64<T> bit_or(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return simd8x64<T>(
        this->chunks[0] | mask,
        this->chunks[1] | mask
      );
    }

    simdjson_really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] == mask,
        this->chunks[1] == mask
      ).to_bitmask();
    }

    simdjson_really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] <= mask,
        this->chunks[1] <= mask
      ).to_bitmask();
    }
  }; // struct simd8x64<T>

} // namespace simd

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#endif // SIMDJSON_HASWELL_SIMD_H
/* end file src/haswell/simd.h */

/* end file src/haswell/simd.h */
/* begin file src/haswell/dom_parser_implementation.h */
#ifndef SIMDJSON_HASWELL_DOM_PARSER_IMPLEMENTATION_H
#define SIMDJSON_HASWELL_DOM_PARSER_IMPLEMENTATION_H

/* begin file src/generic/dom_parser_implementation.h */
/* isadetection.h already included: #include "isadetection.h" */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  simdjson_really_inline dom_parser_implementation();
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation & operator=(const dom_parser_implementation &) = delete;

  SIMDJSON_WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  SIMDJSON_WARN_UNUSED error_code check_for_unclosed_array() noexcept;
  SIMDJSON_WARN_UNUSED error_code stage2(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage2_next(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_capacity(size_t capacity) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_max_depth(size_t max_depth) noexcept final;
};

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/generic/stage1/allocate.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {
namespace allocate {

//
// Allocates stage 1 internal state and outputs in the parser
//
simdjson_really_inline error_code set_capacity(internal::dom_parser_implementation &parser, size_t capacity) {
  size_t max_structures = SIMDJSON_ROUNDUP_N(capacity, 64) + 2 + 7;
  parser.structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] );
  if (!parser.structural_indexes) { return MEMALLOC; }
  parser.structural_indexes[0] = 0;
  parser.n_structural_indexes = 0;
  return SUCCESS;
}

} // namespace allocate
} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/allocate.h */
/* begin file src/generic/stage2/allocate.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace allocate {

//
// Allocates stage 2 internal state and outputs in the parser
//
simdjson_really_inline error_code set_max_depth(dom_parser_implementation &parser, size_t max_depth) {
  parser.open_containers.reset(new (std::nothrow) open_container[max_depth]);
  parser.is_array.reset(new (std::nothrow) bool[max_depth]);

  if (!parser.is_array || !parser.open_containers) {
    return MEMALLOC;
  }
  return SUCCESS;
}

} // namespace allocate
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/allocate.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

simdjson_really_inline dom_parser_implementation::dom_parser_implementation() {}

// Leaving these here so they can be inlined if so desired
SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  error_code err = stage1::allocate::set_capacity(*this, capacity);
  if (err) { _capacity = 0; return err; }
  _capacity = capacity;
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  error_code err = stage2::allocate::set_max_depth(*this, max_depth);
  if (err) { _max_depth = 0; return err; }
  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/allocate.h */

#endif // SIMDJSON_HASWELL_DOM_PARSER_IMPLEMENTATION_H
/* end file src/generic/stage2/allocate.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

SIMDJSON_WARN_UNUSED error_code implementation::create_dom_parser_implementation(
  size_t capacity,
  size_t max_depth,
  std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  dst.reset( new (std::nothrow) dom_parser_implementation() );
  if (!dst) { return MEMALLOC; }
  dst->set_capacity(capacity);
  dst->set_max_depth(max_depth);
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/haswell/end_implementation.h */
#undef SIMDJSON_IMPLEMENTATION
SIMDJSON_UNTARGET_REGION
/* end file src/haswell/end_implementation.h */

/* end file src/haswell/end_implementation.h */
/* begin file src/haswell/dom_parser_implementation.cpp */
/* begin file src/haswell/begin_implementation.h */
#define SIMDJSON_IMPLEMENTATION haswell
#define SIMDJSON_TARGET_HASWELL SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt")

/* haswell/implementation.h already included: #include "haswell/implementation.h" */
/* haswell/intrinsics.h already included: #include "haswell/intrinsics.h" // Generally need to be included outside SIMDJSON_TARGET_REGION */

SIMDJSON_TARGET_HASWELL

/* haswell/bitmanipulation.h already included: #include "haswell/bitmanipulation.h" */
/* haswell/bitmask.h already included: #include "haswell/bitmask.h" */
/* haswell/simd.h already included: #include "haswell/simd.h" */

/* end file src/haswell/begin_implementation.h */
/* haswell/dom_parser_implementation.h already included: #include "haswell/dom_parser_implementation.h" */
/* begin file src/generic/stage2/jsoncharutils.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_really_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return structural_or_whitespace_negated[c];
}

simdjson_really_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = digit_to_val32[630 + src[0]];
  uint32_t v2 = digit_to_val32[420 + src[1]];
  uint32_t v3 = digit_to_val32[210 + src[2]];
  uint32_t v4 = digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_really_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#ifdef SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_really_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_really_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

simdjson_really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
#ifdef _M_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // _M_ARM64
#else // defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
  __uint128_t r = ((__uint128_t)value1) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/jsoncharutils.h */

//
// Stage 1
//

namespace {
namespace SIMDJSON_IMPLEMENTATION {

using namespace simd;

struct json_character_block {
  static simdjson_really_inline json_character_block classify(const simd::simd8x64<uint8_t>& in);
  //  ASCII white-space ('\r','\n','\t',' ')
  simdjson_really_inline uint64_t whitespace() const { return _whitespace; }
  // non-quote structural characters (comma, colon, braces, brackets)
  simdjson_really_inline uint64_t op() const { return _op; }
  // neither a structural character nor a white-space, so letters, numbers and quotes
  simdjson_really_inline uint64_t scalar() { return ~(op() | whitespace()); }

  uint64_t _whitespace; // ASCII white-space ('\r','\n','\t',' ')
  uint64_t _op; // structural characters (comma, colon, braces, brackets but not quotes)
};

// This identifies structural characters (comma, colon, braces, brackets),
// and ASCII white-space ('\r','\n','\t',' ').
simdjson_really_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t>& in) {
  // These lookups rely on the fact that anything < 127 will match the lower 4 bits, which is why
  // we can't use the generic lookup_16.
  auto whitespace_table = simd8<uint8_t>::repeat_16(' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100);
  auto op_table = simd8<uint8_t>::repeat_16(',', '}', 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{');

  // We compute whitespace and op separately. If the code later only use one or the
  // other, given the fact that all functions are aggressively inlined, we can
  // hope that useless computations will be omitted. This is namely case when
  // minifying (we only need whitespace).

  uint64_t whitespace = simd8x64<bool>(
        in.chunks[0] == simd8<uint8_t>(_mm256_shuffle_epi8(whitespace_table, in.chunks[0])),
        in.chunks[1] == simd8<uint8_t>(_mm256_shuffle_epi8(whitespace_table, in.chunks[1]))
  ).to_bitmask();
  
  uint64_t op = simd8x64<bool>(
        (in.chunks[0] | 32) == simd8<uint8_t>(_mm256_shuffle_epi8(op_table, in.chunks[0]-',')),
        (in.chunks[1] | 32) == simd8<uint8_t>(_mm256_shuffle_epi8(op_table, in.chunks[1]-','))
  ).to_bitmask();
  return { whitespace, op };
}

simdjson_really_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  return input.reduce_or().is_ascii();
}

SIMDJSON_UNUSED simdjson_really_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0b11000000u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

simdjson_really_inline simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_third_byte | is_fourth_byte) > int8_t(0);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/generic/stage1/utf8_lookup4_algorithm.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace utf8_validation {

using namespace simd;

  simdjson_really_inline simd8<uint8_t> check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1) {
// Bit 0 = Too Short (lead byte/ASCII followed by lead byte/ASCII)
// Bit 1 = Too Long (ASCII followed by continuation)
// Bit 2 = Overlong 3-byte
// Bit 4 = Surrogate
// Bit 5 = Overlong 2-byte
// Bit 7 = Two Continuations
    constexpr const uint8_t TOO_SHORT   = 1<<0; // 11______ 0_______
                                                // 11______ 11______
    constexpr const uint8_t TOO_LONG    = 1<<1; // 0_______ 10______
    constexpr const uint8_t OVERLONG_3  = 1<<2; // 11100000 100_____
    constexpr const uint8_t SURROGATE   = 1<<4; // 11101101 101_____
    constexpr const uint8_t OVERLONG_2  = 1<<5; // 1100000_ 10______
    constexpr const uint8_t TWO_CONTS   = 1<<7; // 10______ 10______
    constexpr const uint8_t TOO_LARGE   = 1<<3; // 11110100 1001____
                                                // 11110100 101_____
                                                // 11110101 1001____
                                                // 11110101 101_____
                                                // 1111011_ 1001____
                                                // 1111011_ 101_____
                                                // 11111___ 1001____
                                                // 11111___ 101_____
    constexpr const uint8_t TOO_LARGE_1000 = 1<<6;
                                                // 11110101 1000____
                                                // 1111011_ 1000____
                                                // 11111___ 1000____
    constexpr const uint8_t OVERLONG_4  = 1<<6; // 11110000 1000____

    const simd8<uint8_t> byte_1_high = prev1.shr<4>().lookup_16<uint8_t>(
      // 0_______ ________ <ASCII in byte 1>
      TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
      TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
      // 10______ ________ <continuation in byte 1>
      TWO_CONTS, TWO_CONTS, TWO_CONTS, TWO_CONTS,
      // 1100____ ________ <two byte lead in byte 1>
      TOO_SHORT | OVERLONG_2,
      // 1101____ ________ <two byte lead in byte 1>
      TOO_SHORT,
      // 1110____ ________ <three byte lead in byte 1>
      TOO_SHORT | OVERLONG_3 | SURROGATE,
      // 1111____ ________ <four+ byte lead in byte 1>
      TOO_SHORT | TOO_LARGE | TOO_LARGE_1000 | OVERLONG_4
    );
    constexpr const uint8_t CARRY = TOO_SHORT | TOO_LONG | TWO_CONTS; // These all have ____ in byte 1 .
    const simd8<uint8_t> byte_1_low = (prev1 & 0x0F).lookup_16<uint8_t>(
      // ____0000 ________
      CARRY | OVERLONG_3 | OVERLONG_2 | OVERLONG_4,
      // ____0001 ________
      CARRY | OVERLONG_2,
      // ____001_ ________
      CARRY,
      CARRY,

      // ____0100 ________
      CARRY | TOO_LARGE,
      // ____0101 ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      // ____011_ ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,

      // ____1___ ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      // ____1101 ________
      CARRY | TOO_LARGE | TOO_LARGE_1000 | SURROGATE,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000
    );
    const simd8<uint8_t> byte_2_high = input.shr<4>().lookup_16<uint8_t>(
      // ________ 0_______ <ASCII in byte 2>
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,

      // ________ 1000____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE_1000 | OVERLONG_4,
      // ________ 1001____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE,
      // ________ 101_____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,

      // ________ 11______
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT
    );
    return (byte_1_high & byte_1_low & byte_2_high);
  }
  simdjson_really_inline simd8<uint8_t> check_multibyte_lengths(const simd8<uint8_t> input,
      const simd8<uint8_t> prev_input, const simd8<uint8_t> sc) {
    simd8<uint8_t> prev2 = input.prev<2>(prev_input);
    simd8<uint8_t> prev3 = input.prev<3>(prev_input);
    simd8<uint8_t> must23 = simd8<uint8_t>(must_be_2_3_continuation(prev2, prev3));
    simd8<uint8_t> must23_80 = must23 & uint8_t(0x80);
    return must23_80 ^ sc;
  }

  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  simdjson_really_inline simd8<uint8_t> is_incomplete(const simd8<uint8_t> input) {
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
    simdjson_really_inline void check_utf8_bytes(const simd8<uint8_t> input, const simd8<uint8_t> prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      simd8<uint8_t> prev1 = input.prev<1>(prev_input);
      simd8<uint8_t> sc = check_special_cases(input, prev1);
      this->error |= check_multibyte_lengths(input, prev_input, sc);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short.
    simdjson_really_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error |= this->prev_incomplete;
    }

    simdjson_really_inline void check_next_input(const simd8x64<uint8_t>& input) {
      if(simdjson_likely(is_ascii(input))) {
        this->error |= this->prev_incomplete;
      } else {
        // you might think that a for-loop would work, but under Visual Studio, it is not good enough.
        static_assert((simd8x64<uint8_t>::NUM_CHUNKS == 2) || (simd8x64<uint8_t>::NUM_CHUNKS == 4),
            "We support either two or four chunks per 64-byte block.");
        if(simd8x64<uint8_t>::NUM_CHUNKS == 2) {
          this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
          this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
        } else if(simd8x64<uint8_t>::NUM_CHUNKS == 4) {
          this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
          this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
          this->check_utf8_bytes(input.chunks[2], input.chunks[1]);
          this->check_utf8_bytes(input.chunks[3], input.chunks[2]);
        }
        this->prev_incomplete = is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1]);
        this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1];

      }
    }

    simdjson_really_inline error_code errors() {
      return this->error.any_bits_set_anywhere() ? error_code::UTF8_ERROR : error_code::SUCCESS;
    }

  }; // struct utf8_checker
} // namespace utf8_validation

using utf8_validation::utf8_checker;

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/utf8_lookup4_algorithm.h */
/* begin file src/generic/stage1/json_structural_indexer.h */
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

/* begin file src/generic/stage1/buf_block_reader.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {

// Walks through a buffer in block-sized increments, loading the last part with spaces
template<size_t STEP_SIZE>
struct buf_block_reader {
public:
  simdjson_really_inline buf_block_reader(const uint8_t *_buf, size_t _len);
  simdjson_really_inline size_t block_index();
  simdjson_really_inline bool has_full_block() const;
  simdjson_really_inline const uint8_t *full_block() const;
  /**
   * Get the last block, padded with spaces.
   *
   * There will always be a last block, with at least 1 byte, unless len == 0 (in which case this
   * function fills the buffer with spaces and returns 0. In particular, if len == STEP_SIZE there
   * will be 0 full_blocks and 1 remainder block with STEP_SIZE bytes and no spaces for padding.
   *
   * @return the number of effective characters in the last block.
   */
  simdjson_really_inline size_t get_remainder(uint8_t *dst) const;
  simdjson_really_inline void advance();
private:
  const uint8_t *buf;
  const size_t len;
  const size_t lenminusstep;
  size_t idx;
};

// Routines to print masks and text for debugging bitmask operations
SIMDJSON_UNUSED static char * format_input_text_64(const uint8_t *text) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    buf[i] = int8_t(text[i]) < ' ' ? '_' : int8_t(text[i]);
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

// Routines to print masks and text for debugging bitmask operations
SIMDJSON_UNUSED static char * format_input_text(const simd8x64<uint8_t>& in) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  in.store((uint8_t*)buf);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    if (buf[i] < ' ') { buf[i] = '_'; }
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

SIMDJSON_UNUSED static char * format_mask(uint64_t mask) {
  static char *buf = (char*)malloc(64 + 1);
  for (size_t i=0; i<64; i++) {
    buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
  }
  buf[64] = '\0';
  return buf;
}

template<size_t STEP_SIZE>
simdjson_really_inline buf_block_reader<STEP_SIZE>::buf_block_reader(const uint8_t *_buf, size_t _len) : buf{_buf}, len{_len}, lenminusstep{len < STEP_SIZE ? 0 : len - STEP_SIZE}, idx{0} {}

template<size_t STEP_SIZE>
simdjson_really_inline size_t buf_block_reader<STEP_SIZE>::block_index() { return idx; }

template<size_t STEP_SIZE>
simdjson_really_inline bool buf_block_reader<STEP_SIZE>::has_full_block() const {
  return idx < lenminusstep;
}

template<size_t STEP_SIZE>
simdjson_really_inline const uint8_t *buf_block_reader<STEP_SIZE>::full_block() const {
  return &buf[idx];
}

template<size_t STEP_SIZE>
simdjson_really_inline size_t buf_block_reader<STEP_SIZE>::get_remainder(uint8_t *dst) const {
  memset(dst, 0x20, STEP_SIZE); // memset STEP_SIZE because it's more efficient to write out 8 or 16 bytes at once.
  memcpy(dst, buf + idx, len - idx);
  return len - idx;
}

template<size_t STEP_SIZE>
simdjson_really_inline void buf_block_reader<STEP_SIZE>::advance() {
  idx += STEP_SIZE;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/buf_block_reader.h */
/* begin file src/generic/stage1/json_string_scanner.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

struct json_string_block {
  // Escaped characters (characters following an escape() character)
  simdjson_really_inline uint64_t escaped() const { return _escaped; }
  // Escape characters (backslashes that are not escaped--i.e. in \\, includes only the first \)
  simdjson_really_inline uint64_t escape() const { return _backslash & ~_escaped; }
  // Real (non-backslashed) quotes
  simdjson_really_inline uint64_t quote() const { return _quote; }
  // Start quotes of strings
  simdjson_really_inline uint64_t string_start() const { return _quote & _in_string; }
  // End quotes of strings
  simdjson_really_inline uint64_t string_end() const { return _quote & ~_in_string; }
  // Only characters inside the string (not including the quotes)
  simdjson_really_inline uint64_t string_content() const { return _in_string & ~_quote; }
  // Return a mask of whether the given characters are inside a string (only works on non-quotes)
  simdjson_really_inline uint64_t non_quote_inside_string(uint64_t mask) const { return mask & _in_string; }
  // Return a mask of whether the given characters are inside a string (only works on non-quotes)
  simdjson_really_inline uint64_t non_quote_outside_string(uint64_t mask) const { return mask & ~_in_string; }
  // Tail of string (everything except the start quote)
  simdjson_really_inline uint64_t string_tail() const { return _in_string ^ _quote; }

  // backslash characters
  uint64_t _backslash;
  // escaped characters (backslashed--does not include the hex characters after \u)
  uint64_t _escaped;
  // real quotes (non-backslashed ones)
  uint64_t _quote;
  // string characters (includes start quote but not end quote)
  uint64_t _in_string;
};

// Scans blocks for string characters, storing the state necessary to do so
class json_string_scanner {
public:
  simdjson_really_inline json_string_block next(const simd::simd8x64<uint8_t>& in);
  simdjson_really_inline error_code finish(bool streaming);

private:
  // Intended to be defined by the implementation
  simdjson_really_inline uint64_t find_escaped(uint64_t escape);
  simdjson_really_inline uint64_t find_escaped_branchless(uint64_t escape);

  // Whether the last iteration was still inside a string (all 1's = true, all 0's = false).
  uint64_t prev_in_string = 0ULL;
  // Whether the first character of the next iteration is escaped.
  uint64_t prev_escaped = 0ULL;
};

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
simdjson_really_inline uint64_t json_string_scanner::find_escaped_branchless(uint64_t backslash) {
  // If there was overflow, pretend the first character isn't a backslash
  backslash &= ~prev_escaped;
  uint64_t follows_escape = backslash << 1 | prev_escaped;

  // Get sequences starting on even bits by clearing out the odd series using +
  const uint64_t even_bits = 0x5555555555555555ULL;
  uint64_t odd_sequence_starts = backslash & ~even_bits & ~follows_escape;
  uint64_t sequences_starting_on_even_bits;
  prev_escaped = add_overflow(odd_sequence_starts, backslash, &sequences_starting_on_even_bits);
  uint64_t invert_mask = sequences_starting_on_even_bits << 1; // The mask we want to return is the *escaped* bits, not escapes.

  // Mask every other backslashed character as an escaped character
  // Flip the mask for sequences that start on even bits, to correct them
  return (even_bits ^ invert_mask) & follows_escape;
}

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
simdjson_really_inline json_string_block json_string_scanner::next(const simd::simd8x64<uint8_t>& in) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = find_escaped(backslash);
  const uint64_t quote = in.eq('"') & ~escaped;

  //
  // prefix_xor flips on bits inside the string (and flips off the end quote).
  //
  // Then we xor with prev_in_string: if we were in a string already, its effect is flipped
  // (characters inside strings are outside, and characters outside strings are inside).
  //
  const uint64_t in_string = prefix_xor(quote) ^ prev_in_string;

  //
  // Check if we're still in a string at the end of the box so the next block will know
  //
  // right shift of a signed value expected to be well-defined and standard
  // compliant as of C++20, John Regher from Utah U. says this is fine code
  //
  prev_in_string = uint64_t(static_cast<int64_t>(in_string) >> 63);

  // Use ^ to turn the beginning quote off, and the end quote on.
  return {
    backslash,
    escaped,
    quote,
    in_string
  };
}

simdjson_really_inline error_code json_string_scanner::finish(bool streaming) {
  if (prev_in_string and (not streaming)) {
    return UNCLOSED_STRING;
  }
  return SUCCESS;
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/json_string_scanner.h */
/* begin file src/generic/stage1/json_scanner.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

/**
 * A block of scanned json, with information on operators and scalars.
 *
 * We seek to identify pseudo-structural characters. Anything that is inside
 * a string must be omitted (hence  & ~_string.string_tail()).
 * Otherwise, pseudo-structural characters come in two forms.
 * 1. We have the structural characters ([,],{,},:, comma). The 
 *    term 'structural character' is from the JSON RFC.
 * 2. We have the 'scalar pseudo-structural characters'.
 *    Scalars are quotes, and any character except structural characters and white space.
 *
 * To identify the scalar pseudo-structural characters, we must look at what comes
 * before them: it must be a space, a quote or a structural characters.
 * Starting with simdjson v0.3, we identify them by
 * negation: we identify everything that is followed by a non-quote scalar,
 * and we negate that. Whatever remains must be a 'scalar pseudo-structural character'.
 */
struct json_block {
public:
  /**
   * The start of structurals.
   * In simdjson prior to v0.3, these were called the pseudo-structural characters.
   **/
  simdjson_really_inline uint64_t structural_start() { return potential_structural_start() & ~_string.string_tail(); }
  /** All JSON whitespace (i.e. not in a string) */
  simdjson_really_inline uint64_t whitespace() { return non_quote_outside_string(_characters.whitespace()); }

  // Helpers

  /** Whether the given characters are inside a string (only works on non-quotes) */
  simdjson_really_inline uint64_t non_quote_inside_string(uint64_t mask) { return _string.non_quote_inside_string(mask); }
  /** Whether the given characters are outside a string (only works on non-quotes) */
  simdjson_really_inline uint64_t non_quote_outside_string(uint64_t mask) { return _string.non_quote_outside_string(mask); }

  // string and escape characters
  json_string_block _string;
  // whitespace, structural characters ('operators'), scalars
  json_character_block _characters;
  // whether the previous character was a scalar
  uint64_t _follows_potential_nonquote_scalar;
private:
  // Potential structurals (i.e. disregarding strings)

  /**
   * structural elements ([,],{,},:, comma) plus scalar starts like 123, true and "abc".
   * They may reside inside a string.
   **/
  simdjson_really_inline uint64_t potential_structural_start() { return _characters.op() | potential_scalar_start(); }
  /**
   * The start of non-operator runs, like 123, true and "abc".
   * It main reside inside a string.
   **/
  simdjson_really_inline uint64_t potential_scalar_start() {
    // The term "scalar" refers to anything except structural characters and white space
    // (so letters, numbers, quotes).
    // Whenever it is preceded by something that is not a structural element ({,},[,],:, ") nor a white-space
    // then we know that it is irrelevant structurally.
    return _characters.scalar() & ~follows_potential_scalar();
  }
  /**
   * Whether the given character is immediately after a non-operator like 123, true.
   * The characters following a quote are not included.
   */
  simdjson_really_inline uint64_t follows_potential_scalar() {
    // _follows_potential_nonquote_scalar: is defined as marking any character that follows a character
    // that is not a structural element ({,},[,],:, comma) nor a quote (") and that is not a
    // white space.
    // It is understood that within quoted region, anything at all could be marked (irrelevant).
    return _follows_potential_nonquote_scalar;
  }
};

/**
 * Scans JSON for important bits: structural characters or 'operators', strings, and scalars.
 *
 * The scanner starts by calculating two distinct things:
 * - string characters (taking \" into account)
 * - structural characters or 'operators' ([]{},:, comma)
 *   and scalars (runs of non-operators like 123, true and "abc")
 *
 * To minimize data dependency (a key component of the scanner's speed), it finds these in parallel:
 * in particular, the operator/scalar bit will find plenty of things that are actually part of
 * strings. When we're done, json_block will fuse the two together by masking out tokens that are
 * part of a string.
 */
class json_scanner {
public:
  json_scanner() {}
  simdjson_really_inline json_block next(const simd::simd8x64<uint8_t>& in);
  simdjson_really_inline error_code finish(bool streaming);

private:
  // Whether the last character of the previous iteration is part of a scalar token
  // (anything except whitespace or a structural character/'operator').
  uint64_t prev_scalar = 0ULL;
  json_string_scanner string_scanner{};
};


//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
simdjson_really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

simdjson_really_inline json_block json_scanner::next(const simd::simd8x64<uint8_t>& in) {
  json_string_block strings = string_scanner.next(in);
  // identifies the white-space and the structurat characters
  json_character_block characters = json_character_block::classify(in);
  // The term "scalar" refers to anything except structural characters and white space
  // (so letters, numbers, quotes).
  // We want  follows_scalar to mark anything that follows a non-quote scalar (so letters and numbers).
  //
  // A terminal quote should either be followed by a structural character (comma, brace, bracket, colon)
  // or nothing. However, we still want ' "a string"true ' to mark the 't' of 'true' as a potential
  // pseudo-structural character just like we would if we had  ' "a string" true '; otherwise we
  // may need to add an extra check when parsing strings.
  //
  // Performance: there are many ways to skin this cat.
  const uint64_t nonquote_scalar = characters.scalar() & ~strings.quote();
  uint64_t follows_nonquote_scalar = follows(nonquote_scalar, prev_scalar);
  return {
    strings,
    characters,
    follows_nonquote_scalar
  };
}

simdjson_really_inline error_code json_scanner::finish(bool streaming) {
  return string_scanner.finish(streaming);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/json_scanner.h */
/* begin file src/generic/stage1/json_minifier.h */
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class json_minifier {
public:
  template<size_t STEP_SIZE>
  static error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept;

private:
  simdjson_really_inline json_minifier(uint8_t *_dst)
  : dst{_dst}
  {}
  template<size_t STEP_SIZE>
  simdjson_really_inline void step(const uint8_t *block_buf, buf_block_reader<STEP_SIZE> &reader) noexcept;
  simdjson_really_inline void next(const simd::simd8x64<uint8_t>& in, json_block block);
  simdjson_really_inline error_code finish(uint8_t *dst_start, size_t &dst_len);
  json_scanner scanner{};
  uint8_t *dst;
};

simdjson_really_inline void json_minifier::next(const simd::simd8x64<uint8_t>& in, json_block block) {
  uint64_t mask = block.whitespace();
  in.compress(mask, dst);
  dst += 64 - count_ones(mask);
}

simdjson_really_inline error_code json_minifier::finish(uint8_t *dst_start, size_t &dst_len) {
  *dst = '\0';
  error_code error = scanner.finish(false);
  if (error) { dst_len = 0; return error; }
  dst_len = dst - dst_start;
  return SUCCESS;
}

template<>
simdjson_really_inline void json_minifier::step<128>(const uint8_t *block_buf, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  simd::simd8x64<uint8_t> in_2(block_buf+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1);
  this->next(in_2, block_2);
  reader.advance();
}

template<>
simdjson_really_inline void json_minifier::step<64>(const uint8_t *block_buf, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  json_block block_1 = scanner.next(in_1);
  this->next(block_buf, block_1);
  reader.advance();
}

template<size_t STEP_SIZE>
error_code json_minifier::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept {
  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_minifier minifier(dst);

  // Index the first n-1 blocks
  while (reader.has_full_block()) {
    minifier.step<STEP_SIZE>(reader.full_block(), reader);
  }

  // Index the last (remainder) block, padded with spaces
  uint8_t block[STEP_SIZE];
  if (simdjson_likely(reader.get_remainder(block)) > 0) {
    minifier.step<STEP_SIZE>(block, reader);
  }

  return minifier.finish(dst, dst_len);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/json_minifier.h */
/* begin file src/generic/stage1/find_next_document_index.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {

/**
  * This algorithm is used to quickly identify the last structural position that
  * makes up a complete document.
  *
  * It does this by going backwards and finding the last *document boundary* (a
  * place where one value follows another without a comma between them). If the
  * last document (the characters after the boundary) has an equal number of
  * start and end brackets, it is considered complete.
  *
  * Simply put, we iterate over the structural characters, starting from
  * the end. We consider that we found the end of a JSON document when the
  * first element of the pair is NOT one of these characters: '{' '[' ';' ','
  * and when the second element is NOT one of these characters: '}' '}' ';' ','.
  *
  * This simple comparison works most of the time, but it does not cover cases
  * where the batch's structural indexes contain a perfect amount of documents.
  * In such a case, we do not have access to the structural index which follows
  * the last document, therefore, we do not have access to the second element in
  * the pair, and that means we cannot identify the last document. To fix this
  * issue, we keep a count of the open and closed curly/square braces we found
  * while searching for the pair. When we find a pair AND the count of open and
  * closed curly/square braces is the same, we know that we just passed a
  * complete document, therefore the last json buffer location is the end of the
  * batch.
  */
simdjson_really_inline uint32_t find_next_document_index(dom_parser_implementation &parser) {
  // TODO don't count separately, just figure out depth
  auto arr_cnt = 0;
  auto obj_cnt = 0;
  for (auto i = parser.n_structural_indexes - 1; i > 0; i--) {
    auto idxb = parser.structural_indexes[i];
    switch (parser.buf[idxb]) {
    case ':':
    case ',':
      continue;
    case '}':
      obj_cnt--;
      continue;
    case ']':
      arr_cnt--;
      continue;
    case '{':
      obj_cnt++;
      break;
    case '[':
      arr_cnt++;
      break;
    }
    auto idxa = parser.structural_indexes[i - 1];
    switch (parser.buf[idxa]) {
    case '{':
    case '[':
    case ':':
    case ',':
      continue;
    }
    // Last document is complete, so the next document will appear after!
    if (!arr_cnt && !obj_cnt) {
      return parser.n_structural_indexes;
    }
    // Last document is incomplete; mark the document at i + 1 as the next one
    return i;
  }
  return 0;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/find_next_document_index.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class bit_indexer {
public:
  uint32_t *tail;

  simdjson_really_inline bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // base_ptr[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so base_ptr
  // needs to be large enough to handle this
  simdjson_really_inline void write(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
        return;
    int cnt = static_cast<int>(count_ones(bits));

    // Do the first 8 all together
    for (int i=0; i<8; i++) {
      this->tail[i] = idx + trailing_zeroes(bits);
      bits = clear_lowest_bit(bits);
    }

    // Do the next 8 all together (we hope in most cases it won't happen at all
    // and the branch is easily predicted).
    if (simdjson_unlikely(cnt > 8)) {
      for (int i=8; i<16; i++) {
        this->tail[i] = idx + trailing_zeroes(bits);
        bits = clear_lowest_bit(bits);
      }

      // Most files don't have 16+ structurals per block, so we take several basically guaranteed
      // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
      // or the start of a value ("abc" true 123) every four characters.
      if (simdjson_unlikely(cnt > 16)) {
        int i = 16;
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

class json_structural_indexer {
public:
  /**
   * Find the important bits of JSON in a 128-byte chunk, and add them to structural_indexes.
   *
   * @param partial Setting the partial parameter to true allows the find_structural_bits to
   *   tolerate unclosed strings. The caller should still ensure that the input is valid UTF-8. If
   *   you are processing substrings, you may want to call on a function like trimmed_length_safe_utf8.
   */
  template<size_t STEP_SIZE>
  static error_code index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, bool partial) noexcept;

private:
  simdjson_really_inline json_structural_indexer(uint32_t *structural_indexes);
  template<size_t STEP_SIZE>
  simdjson_really_inline void step(const uint8_t *block, buf_block_reader<STEP_SIZE> &reader) noexcept;
  simdjson_really_inline void next(const simd::simd8x64<uint8_t>& in, json_block block, size_t idx);
  simdjson_really_inline error_code finish(dom_parser_implementation &parser, size_t idx, size_t len, bool partial);

  json_scanner scanner{};
  utf8_checker checker{};
  bit_indexer indexer;
  uint64_t prev_structurals = 0;
  uint64_t unescaped_chars_error = 0;
};

simdjson_really_inline json_structural_indexer::json_structural_indexer(uint32_t *structural_indexes) : indexer{structural_indexes} {}

// Skip the last character if it is partial
simdjson_really_inline size_t trim_partial_utf8(const uint8_t *buf, size_t len) {
  if (simdjson_unlikely(len < 3)) {
    switch (len) {
      case 2:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 2 bytes left
        return len;
      case 1:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        return len;
      case 0:
        return len;
    }
  }
  if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
  if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 1 byte left
  if (buf[len-3] >= 0b11110000) { return len-3; } // 4-byte characters with only 3 bytes left
  return len;
}

//
// PERF NOTES:
// We pipe 2 inputs through these stages:
// 1. Load JSON into registers. This takes a long time and is highly parallelizable, so we load
//    2 inputs' worth at once so that by the time step 2 is looking for them input, it's available.
// 2. Scan the JSON for critical data: strings, scalars and operators. This is the critical path.
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
template<size_t STEP_SIZE>
error_code json_structural_indexer::index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, bool partial) noexcept {
  if (simdjson_unlikely(len > parser.capacity())) { return CAPACITY; }
  if (partial) { len = trim_partial_utf8(buf, len); }

  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_structural_indexer indexer(parser.structural_indexes.get());

  // Read all but the last block
  while (reader.has_full_block()) {
    indexer.step<STEP_SIZE>(reader.full_block(), reader);
  }

  // Take care of the last block (will always be there unless file is empty)
  uint8_t block[STEP_SIZE];
  if (simdjson_unlikely(reader.get_remainder(block) == 0)) { return EMPTY; }
  indexer.step<STEP_SIZE>(block, reader);

  return indexer.finish(parser, reader.block_index(), len, partial);
}

template<>
simdjson_really_inline void json_structural_indexer::step<128>(const uint8_t *block, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  simd::simd8x64<uint8_t> in_2(block+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1, reader.block_index());
  this->next(in_2, block_2, reader.block_index()+64);
  reader.advance();
}

template<>
simdjson_really_inline void json_structural_indexer::step<64>(const uint8_t *block, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  json_block block_1 = scanner.next(in_1);
  this->next(in_1, block_1, reader.block_index());
  reader.advance();
}

simdjson_really_inline void json_structural_indexer::next(const simd::simd8x64<uint8_t>& in, json_block block, size_t idx) {
  uint64_t unescaped = in.lteq(0x1F);
  checker.check_next_input(in);
  indexer.write(uint32_t(idx-64), prev_structurals); // Output *last* iteration's structurals to the parser
  prev_structurals = block.structural_start();
  unescaped_chars_error |= block.non_quote_inside_string(unescaped);
}

simdjson_really_inline error_code json_structural_indexer::finish(dom_parser_implementation &parser, size_t idx, size_t len, bool partial) {
  // Write out the final iteration's structurals
  indexer.write(uint32_t(idx-64), prev_structurals);

  error_code error = scanner.finish(partial);
  if (simdjson_unlikely(error != SUCCESS)) { return error; }

  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }

  parser.n_structural_indexes = uint32_t(indexer.tail - parser.structural_indexes.get());
  /***
   * This is related to https://github.com/simdjson/simdjson/issues/906
   * Basically, we want to make sure that if the parsing continues beyond the last (valid)
   * structural character, it quickly stops.
   * Only three structural characters can be repeated without triggering an error in JSON:  [,] and }.
   * We repeat the padding character (at 'len'). We don't know what it is, but if the parsing
   * continues, then it must be [,] or }.
   * Suppose it is ] or }. We backtrack to the first character, what could it be that would
   * not trigger an error? It could be ] or } but no, because you can't start a document that way.
   * It can't be a comma, a colon or any simple value. So the only way we could continue is
   * if the repeated character is [. But if so, the document must start with [. But if the document
   * starts with [, it should end with ]. If we enforce that rule, then we would get
   * ][[ which is invalid.
   **/
  parser.structural_indexes[parser.n_structural_indexes] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 1] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 2] = 0;
  parser.next_structural_index = 0;
  // a valid JSON file cannot have zero structural indexes - we should have found something
  if (simdjson_unlikely(parser.n_structural_indexes == 0u)) {
    return EMPTY;
  }
  if (simdjson_unlikely(parser.structural_indexes[parser.n_structural_indexes - 1] > len)) {
    return UNEXPECTED_ERROR;
  }
  if (partial) {
    auto new_structural_indexes = find_next_document_index(parser);
    if (new_structural_indexes == 0 && parser.n_structural_indexes > 0) {
      return CAPACITY; // If the buffer is partial but the document is incomplete, it's too big to parse.
    }
    parser.n_structural_indexes = new_structural_indexes;
  }
  return checker.errors();
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/find_next_document_index.h */
/* begin file src/generic/stage1/utf8_validator.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

/**
 * Validates that the string is actual UTF-8.
 */
template<class checker>
bool generic_validate_utf8(const uint8_t * input, size_t length) {
    checker c{};
    buf_block_reader<64> reader(input, length);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      c.check_next_input(in);
      reader.advance();
    }
    uint8_t block[64]{};
    reader.get_remainder(block);
    simd::simd8x64<uint8_t> in(block);
    c.check_next_input(in);
    reader.advance();
    return c.errors() == error_code::SUCCESS;
}

bool generic_validate_utf8(const char * input, size_t length) {
    return generic_validate_utf8<utf8_checker>((const uint8_t *)input,length);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/utf8_validator.h */

//
// Stage 2
//
/* begin file src/haswell/stringparsing.h */
#ifndef SIMDJSON_HASWELL_STRINGPARSING_H
#define SIMDJSON_HASWELL_STRINGPARSING_H

/* haswell/simd.h already included: #include "haswell/simd.h" */
/* haswell/bitmanipulation.h already included: #include "haswell/bitmanipulation.h" */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_really_inline bool has_backslash() { return ((quote_bits - 1) & bs_bits) != 0; }
  simdjson_really_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_really_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 15 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v(src);
  // store to dest unconditionally - we can overwrite the bits we don't like later
  v.store(dst);
  return {
      (uint32_t)(v == '\\').to_bitmask(),     // bs_bits
      (uint32_t)(v == '"').to_bitmask(), // quote_bits
  };
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/generic/stage2/stringparsing.h */
// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace stringparsing {

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
SIMDJSON_WARN_UNUSED
simdjson_really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
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

SIMDJSON_WARN_UNUSED simdjson_really_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst) {
  src++;
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return nullptr;
}

SIMDJSON_UNUSED SIMDJSON_WARN_UNUSED simdjson_really_inline error_code parse_string_to_buffer(const uint8_t *src, uint8_t *&current_string_buf_loc, std::string_view &s) {
  if (src[0] != '"') { return STRING_ERROR; }
  auto end = stringparsing::parse_string(src, current_string_buf_loc);
  if (!end) { return STRING_ERROR; }
  s = std::string_view((const char *)current_string_buf_loc, end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}

} // namespace stringparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/stringparsing.h */

#endif // SIMDJSON_HASWELL_STRINGPARSING_H
/* end file src/generic/stage2/stringparsing.h */
/* begin file src/haswell/numberparsing.h */
#ifndef SIMDJSON_HASWELL_NUMBERPARSING_H
#define SIMDJSON_HASWELL_NUMBERPARSING_H

namespace {
namespace SIMDJSON_IMPLEMENTATION {

static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
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

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#define SWAR_NUMBER_PARSING

/* begin file src/generic/stage2/numberparsing.h */
#include <cmath>
#include <limits>

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace numberparsing {

#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [FASTFLOAT_SMALLEST_POWER,
// FASTFLOAT_LARGEST_POWER] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / power_of_ten[-power];
    } else {
      d = d * power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It wa
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }

  // We are going to need to do some 64-bit arithmetic to get a more precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= FASTFLOAT_SMALLEST_POWER
  // and power <= FASTFLOAT_LARGEST_POWER
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  uint64_t factor_mantissa = mantissa_64[power - FASTFLOAT_SMALLEST_POWER];
  
  // The exponent is 1024 + 63 + power 
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power
  //
  // The 65536 is (1<<16) and corresponds to 
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to 
  // floor(log(5**power)/log(2)) 
  //
  // Note that this is not magic: 152170/(1<<16) is 
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a 
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;
  

  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  value128 product = full_multiplication(i, factor_mantissa);
  uint64_t lower = product.low;
  uint64_t upper = product.high;

  // We know that upper has at most one leading zero because
  // both i and  factor_mantissa have a leading one. This means
  // that the result is at least as large as ((1<<63)*(1<<63))/(1<<64).

  // As long as the first 9 bits of "upper" are not "1", then we
  // know that we have an exact computed value for the leading
  // 55 bits because any imprecision would play out as a +1, in
  // the worst case.
  if (simdjson_unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
    uint64_t factor_mantissa_low =
        mantissa_128[power - FASTFLOAT_SMALLEST_POWER];
    // next, we compute the 64-bit x 128-bit multiplication, getting a 192-bit
    // result (three 64-bit values)
    product = full_multiplication(i, factor_mantissa_low);
    uint64_t product_low = product.low;
    uint64_t product_middle2 = product.high;
    uint64_t product_middle1 = lower;
    uint64_t product_high = upper;
    uint64_t product_middle = product_middle1 + product_middle2;
    if (product_middle < product_middle1) {
      product_high++; // overflow carry
    }
    // We want to check whether mantissa *i + i would affect our result.
    // This does happen, e.g. with 7.3177701707893310e+15.
    if (((product_middle + 1 == 0) && ((product_high & 0x1FF) == 0x1FF) &&
         (product_low + i < product_low))) { // let us be prudent and bail out.
      return false;
    }
    upper = product_high;
    lower = product_middle;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).

  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  if (simdjson_unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
               ((mantissa & 3) == 1))) {
    // if mantissa & 1 == 1 we might need to round up.
    //
    // Scenarios:
    // 1. We are not in the middle. Then we should round up.
    //
    // 2. We are right in the middle. Whether we round up depends
    // on the last significant bit: if it is "one" then we round
    // up (round to even) otherwise, we do not.
    //
    // So if the last significant bit is 1, we can safely round up.
    // Hence we only need to bail out if (mantissa & 3) == 1.
    // Otherwise we may need more accuracy or analysis to determine whether
    // we are exactly between two floating-point numbers.
    // It can be triggered with 1e23.
    // Note: because the factor_mantissa and factor_mantissa_low are
    // almost always rounded down (except for small positive powers),
    // almost always should round up.
    return false;
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    lz--; // undo previous addition
  }
  mantissa &= ~(1ULL << 52);
  uint64_t real_exponent = exponent - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    return false;
  }
  mantissa |= real_exponent << 52;
  mantissa |= (((uint64_t)negative) << 63);
  memcpy(&d, &mantissa, sizeof(d));
  return true;
}

static bool parse_float_strtod(const uint8_t *ptr, double *outDouble) {
  char *endptr;
  *outDouble = strtod((const char *)ptr, &endptr);
  // Some libraries will set errno = ERANGE when the value is subnormal,
  // yet we may want to be able to parse subnormal values.
  // However, we do not want to tolerate NAN or infinite values.
  //
  // Values like infinity or NaN are not allowed in the JSON specification.
  // If you consume a large value and you map it to "infinity", you will no
  // longer be able to serialize back a standard-compliant JSON. And there is
  // no realistic application where you might need values so large than they
  // can't fit in binary64. The maximal value is about  1.7976931348623157 x
  // 10^308 It is an unimaginable large number. There will never be any piece of
  // engineering involving as many as 10^308 parts. It is estimated that there
  // are about 10^80 atoms in the universe.  The estimate for the total number
  // of electrons is similar. Using a double-precision floating-point value, we
  // can represent easily the number of atoms in the universe. We could  also
  // represent the number of ways you can pick any three individual atoms at
  // random in the universe. If you ever encounter a number much larger than
  // 10^308, you know that you have a bug. RapidJSON will reject a document with
  // a float that does not fit in binary64. JSON for Modern C++ (nlohmann/json)
  // will flat out throw an exception.
  //
  if ((endptr == (const char *)ptr) || (!std::isfinite(*outDouble))) {
    return false;
  }
  return true;
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(SIMDJSON_UNUSED const uint8_t * src, W writer) {
  double d;
  if (parse_float_strtod(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow. 
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off 
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do 
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time. 
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in 
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline int significant_digits(const uint8_t * start_digits, int digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - int(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, int digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  // digit count is off by 1 because of the decimal (assuming there was one).
  if (simdjson_unlikely(digit_count-1 >= 19 && significant_digits(start_digits, digit_count) >= 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < FASTFLOAT_SMALLEST_POWER) || (exponent > FASTFLOAT_LARGEST_POWER)) {
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_strtod(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool clean_end = is_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (!clean_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// SAX functions
namespace {
// Parse any number from 0 to 18,446,744,073,709,551,615
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  if (digit_count > 20) { return NUMBER_ERROR; }
  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return i;
}

// Parse any number from 0 to 18,446,744,073,709,551,615
// Call this version of the method if you regularly expect 8- or 16-digit numbers.
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_large_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
      if (parse_digit(*p, i)) { // digit 17
        p++;
        if (parse_digit(*p, i)) { // digit 18
          p++;
          if (parse_digit(*p, i)) { // digit 19
            p++;
            if (parse_digit(*p, i)) { // digit 20
              p++;
              if (parse_digit(*p, i)) { return NUMBER_ERROR; } // 21 digits is an error
              // Positive overflow check:
              // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
              //   biggest uint64_t.
              // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
              //   If we got here, it's a 20 digit number starting with the digit "1".
              // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
              //   than 1,553,255,926,290,448,384.
              // - That is smaller than the smallest possible 20-digit number the user could write:
              //   10,000,000,000,000,000,000.
              // - Therefore, if the number is positive and lower than that, it's overflow.
              // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
              //
              if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
            }
          }
        }
      }
    } // 16 digits
  } else { // 8 digits
    // Less than 8 digits can't overflow, simpler logic here.
    if (parse_digit(*p, i)) { p++; } else { return NUMBER_ERROR; }
    while (parse_digit(*p, i)) { p++; }
  }

  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }
  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - src);
  if (digit_count == 0 || ('0' == *src && digit_count > 1)) { return NUMBER_ERROR; }
  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return NUMBER_ERROR; }
  if (digit_count == longest_digit_count) {
    if(negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return NUMBER_ERROR; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return negative ? (~i+1) : i;
}

SIMDJSON_UNUSED simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src || (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 >= 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src >= 19;
    }
  } else {
    overflow = p-src >= 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
    overflow = overflow || exponent < FASTFLOAT_SMALLEST_POWER || exponent > FASTFLOAT_LARGEST_POWER;
  }

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_strtod(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/numberparsing.h */

#endif // SIMDJSON_HASWELL_NUMBERPARSING_H
/* end file src/generic/stage2/numberparsing.h */
/* begin file src/generic/stage2/tape_builder.h */
/* begin file src/generic/stage2/json_iterator.h */
/* begin file src/generic/stage2/logger.h */
// This is for an internal-only stage 2 specific logger.
// Set LOG_ENABLED = true to log what stage 2 is doing!
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace logger {

  static constexpr const char * DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";

  static constexpr const bool LOG_ENABLED = false;
  static constexpr const int LOG_EVENT_LEN = 20;
  static constexpr const int LOG_BUFFER_LEN = 30;
  static constexpr const int LOG_SMALL_BUFFER_LEN = 10;
  static constexpr const int LOG_INDEX_LEN = 5;

  static int log_depth; // Not threadsafe. Log only.

  // Helper to turn unprintable or newline characters into spaces
  static simdjson_really_inline char printable_char(char c) {
    if (c >= 0x20) {
      return c;
    } else {
      return ' ';
    }
  }

  // Print the header and set up log_start
  static simdjson_really_inline void log_start() {
    if (LOG_ENABLED) {
      log_depth = 0;
      printf("\n");
      printf("| %-*s | %-*s | %-*s | %-*s | Detail |\n", LOG_EVENT_LEN, "Event", LOG_BUFFER_LEN, "Buffer", LOG_SMALL_BUFFER_LEN, "Next", 5, "Next#");
      printf("|%.*s|%.*s|%.*s|%.*s|--------|\n", LOG_EVENT_LEN+2, DASHES, LOG_BUFFER_LEN+2, DASHES, LOG_SMALL_BUFFER_LEN+2, DASHES, 5+2, DASHES);
    }
  }

  // Logs a single line of 
  template<typename S>
  static simdjson_really_inline void log_line(S &structurals, const char *title_prefix, const char *title, const char *detail) {
    if (LOG_ENABLED) {
      printf("| %*s%s%-*s ", log_depth*2, "", title_prefix, LOG_EVENT_LEN - log_depth*2 - int(strlen(title_prefix)), title);
      auto current_index = structurals.at_beginning() ? nullptr : structurals.next_structural-1;
      auto next_index = structurals.next_structural;
      auto current = current_index ? &structurals.buf[*current_index] : (const uint8_t*)"                                                       ";
      auto next = &structurals.buf[*next_index];
      {
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_BUFFER_LEN;i++) {
          printf("%c", printable_char(current[i]));
        }
        printf(" ");
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_SMALL_BUFFER_LEN;i++) {
          printf("%c", printable_char(next[i]));
        }
        printf(" ");
      }
      if (current_index) {
        printf("| %*u ", LOG_INDEX_LEN, *current_index);
      } else {
        printf("| %-*s ", LOG_INDEX_LEN, "");
      }
      // printf("| %*u ", LOG_INDEX_LEN, structurals.next_tape_index());
      printf("| %-s ", detail);
      printf("|\n");
    }
  }

} // namespace logger
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/logger.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

class json_iterator {
public:
  const uint8_t* const buf;
  uint32_t *next_structural;
  dom_parser_implementation &dom_parser;
  uint32_t depth{0};

  /**
   * Walk the JSON document.
   *
   * The visitor receives callbacks when values are encountered. All callbacks pass the iterator as
   * the first parameter; some callbacks have other parameters as well:
   *
   * - visit_document_start() - at the beginning.
   * - visit_document_end() - at the end (if things were successful).
   *
   * - visit_array_start() - at the start `[` of a non-empty array.
   * - visit_array_end() - at the end `]` of a non-empty array.
   * - visit_empty_array() - when an empty array is encountered.
   *
   * - visit_object_end() - at the start `]` of a non-empty object.
   * - visit_object_start() - at the end `]` of a non-empty object.
   * - visit_empty_object() - when an empty object is encountered.
   * - visit_key(const uint8_t *key) - when a key in an object field is encountered. key is
   *                                   guaranteed to point at the first quote of the string (`"key"`).
   * - visit_primitive(const uint8_t *value) - when a value is a string, number, boolean or null.
   * - visit_root_primitive(iter, uint8_t *value) - when the top-level value is a string, number, boolean or null.
   *
   * - increment_count(iter) - each time a value is found in an array or object.
   */
  template<bool STREAMING, typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code walk_document(V &visitor) noexcept;

  /**
   * Create an iterator capable of walking a JSON document.
   *
   * The document must have already passed through stage 1.
   */
  simdjson_really_inline json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index);

  /**
   * Look at the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *peek() const noexcept;
  /**
   * Advance to the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *advance() noexcept;
  /**
   * Get the remaining length of the document, from the start of the current token.
   */
  simdjson_really_inline size_t remaining_len() const noexcept;
  /**
   * Check if we are at the end of the document.
   *
   * If this is true, there are no more tokens.
   */
  simdjson_really_inline bool at_eof() const noexcept;
  /**
   * Check if we are at the beginning of the document.
   */
  simdjson_really_inline bool at_beginning() const noexcept;
  simdjson_really_inline uint8_t last_structural() const noexcept;

  /**
   * Log that a value has been found.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_value(const char *type) const noexcept;
  /**
   * Log the start of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_start_value(const char *type) const noexcept;
  /**
   * Log the end of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_end_value(const char *type) const noexcept;
  /**
   * Log an error.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_error(const char *error) const noexcept;

  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_primitive(V &visitor, const uint8_t *value) noexcept;
  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_primitive(V &visitor, const uint8_t *value) noexcept;
};

template<bool STREAMING, typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::walk_document(V &visitor) noexcept {
  logger::log_start();

  //
  // Start the document
  //
  if (at_eof()) { return EMPTY; }
  log_start_value("document");
  SIMDJSON_TRY( visitor.visit_document_start(*this) );

  //
  // Read first value
  //
  {
    auto value = advance();

    // Make sure the outer hash or array is closed before continuing; otherwise, there are ways we
    // could get into memory corruption. See https://github.com/simdjson/simdjson/issues/906
    if (!STREAMING) {
      switch (*value) {
        case '{': if (last_structural() != '}') { return TAPE_ERROR; }; break;
        case '[': if (last_structural() != ']') { return TAPE_ERROR; }; break;
      }
    }

    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_root_primitive(*this, value) ); break;
    }
  }
  goto document_end;

//
// Object parser states
//
object_begin:
  log_start_value("object");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = false;
  SIMDJSON_TRY( visitor.visit_object_start(*this) );

  {
    auto key = advance();
    if (*key != '"') { log_error("Object does not start with a key"); return TAPE_ERROR; }
    SIMDJSON_TRY( visitor.increment_count(*this) );
    SIMDJSON_TRY( visitor.visit_key(*this, key) );
  }

object_field:
  if (simdjson_unlikely( *advance() != ':' )) { log_error("Missing colon after key in object"); return TAPE_ERROR; }
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

object_continue:
  switch (*advance()) {
    case ',':
      SIMDJSON_TRY( visitor.increment_count(*this) );
      {
        auto key = advance();
        if (simdjson_unlikely( *key != '"' )) { log_error("Key string missing at beginning of field in object"); return TAPE_ERROR; }
        SIMDJSON_TRY( visitor.visit_key(*this, key) );
      }
      goto object_field;
    case '}': log_end_value("object"); SIMDJSON_TRY( visitor.visit_object_end(*this) ); goto scope_end;
    default: log_error("No comma between object fields"); return TAPE_ERROR;
  }

scope_end:
  depth--;
  if (depth == 0) { goto document_end; }
  if (dom_parser.is_array[depth]) { goto array_continue; }
  goto object_continue;

//
// Array parser states
//
array_begin:
  log_start_value("array");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = true;
  SIMDJSON_TRY( visitor.visit_array_start(*this) );
  SIMDJSON_TRY( visitor.increment_count(*this) );

array_value:
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

array_continue:
  switch (*advance()) {
    case ',': SIMDJSON_TRY( visitor.increment_count(*this) ); goto array_value;
    case ']': log_end_value("array"); SIMDJSON_TRY( visitor.visit_array_end(*this) ); goto scope_end;
    default: log_error("Missing comma between array values"); return TAPE_ERROR;
  }

document_end:
  log_end_value("document");
  SIMDJSON_TRY( visitor.visit_document_end(*this) );

  dom_parser.next_structural_index = uint32_t(next_structural - &dom_parser.structural_indexes[0]);

  // If we didn't make it to the end, it's an error
  if ( !STREAMING && dom_parser.next_structural_index != dom_parser.n_structural_indexes ) {
    log_error("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
    return TAPE_ERROR;
  }

  return SUCCESS;

} // walk_document()

simdjson_really_inline json_iterator::json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index)
  : buf{_dom_parser.buf},
    next_structural{&_dom_parser.structural_indexes[start_structural_index]},
    dom_parser{_dom_parser} {
}

simdjson_really_inline const uint8_t *json_iterator::peek() const noexcept {
  return &buf[*(next_structural)];
}
simdjson_really_inline const uint8_t *json_iterator::advance() noexcept {
  return &buf[*(next_structural++)];
}
simdjson_really_inline size_t json_iterator::remaining_len() const noexcept {
  return dom_parser.len - *(next_structural-1);
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return next_structural == &dom_parser.structural_indexes[dom_parser.n_structural_indexes];
}
simdjson_really_inline bool json_iterator::at_beginning() const noexcept {
  return next_structural == dom_parser.structural_indexes.get();
}
simdjson_really_inline uint8_t json_iterator::last_structural() const noexcept {
  return buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]];
}

simdjson_really_inline void json_iterator::log_value(const char *type) const noexcept {
  logger::log_line(*this, "", type, "");
}

simdjson_really_inline void json_iterator::log_start_value(const char *type) const noexcept {
  logger::log_line(*this, "+", type, "");
  if (logger::LOG_ENABLED) { logger::log_depth++; }
}

simdjson_really_inline void json_iterator::log_end_value(const char *type) const noexcept {
  if (logger::LOG_ENABLED) { logger::log_depth--; }
  logger::log_line(*this, "-", type, "");
}

simdjson_really_inline void json_iterator::log_error(const char *error) const noexcept {
  logger::log_line(*this, "", "ERROR", error);
}

template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_root_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_root_string(*this, value);
    case 't': return visitor.visit_root_true_atom(*this, value);
    case 'f': return visitor.visit_root_false_atom(*this, value);
    case 'n': return visitor.visit_root_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_root_number(*this, value);
    default:
      log_error("Document starts with a non-value character");
      return TAPE_ERROR;
  }
}
template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_string(*this, value);
    case 't': return visitor.visit_true_atom(*this, value);
    case 'f': return visitor.visit_false_atom(*this, value);
    case 'n': return visitor.visit_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_number(*this, value);
    default:
      log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/logger.h */
/* begin file src/generic/stage2/tape_writer.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_writer {
  /** The next place to write to tape */
  uint64_t *next_tape_loc;
  
  /** Write a signed 64-bit value to tape. */
  simdjson_really_inline void append_s64(int64_t value) noexcept;

  /** Write an unsigned 64-bit value to tape. */
  simdjson_really_inline void append_u64(uint64_t value) noexcept;

  /** Write a double value to tape. */
  simdjson_really_inline void append_double(double value) noexcept;

  /**
   * Append a tape entry (an 8-bit type,and 56 bits worth of value).
   */
  simdjson_really_inline void append(uint64_t val, internal::tape_type t) noexcept;

  /**
   * Skip the current tape entry without writing.
   *
   * Used to skip the start of the container, since we'll come back later to fill it in when the
   * container ends.
   */
  simdjson_really_inline void skip() noexcept;

  /**
   * Skip the number of tape entries necessary to write a large u64 or i64.
   */
  simdjson_really_inline void skip_large_integer() noexcept;

  /**
   * Skip the number of tape entries necessary to write a double.
   */
  simdjson_really_inline void skip_double() noexcept;

  /**
   * Write a value to a known location on tape.
   *
   * Used to go back and write out the start of a container after the container ends.
   */
  simdjson_really_inline static void write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept;

private:
  /**
   * Append both the tape entry, and a supplementary value following it. Used for types that need
   * all 64 bits, such as double and uint64_t.
   */
  template<typename T>
  simdjson_really_inline void append2(uint64_t val, T val2, internal::tape_type t) noexcept;
}; // struct number_writer

simdjson_really_inline void tape_writer::append_s64(int64_t value) noexcept {
  append2(0, value, internal::tape_type::INT64);
}

simdjson_really_inline void tape_writer::append_u64(uint64_t value) noexcept {
  append(0, internal::tape_type::UINT64);
  *next_tape_loc = value;
  next_tape_loc++;
}

/** Write a double value to tape. */
simdjson_really_inline void tape_writer::append_double(double value) noexcept {
  append2(0, value, internal::tape_type::DOUBLE);
}

simdjson_really_inline void tape_writer::skip() noexcept {
  next_tape_loc++;
}

simdjson_really_inline void tape_writer::skip_large_integer() noexcept {
  next_tape_loc += 2;
}

simdjson_really_inline void tape_writer::skip_double() noexcept {
  next_tape_loc += 2;
}

simdjson_really_inline void tape_writer::append(uint64_t val, internal::tape_type t) noexcept {
  *next_tape_loc = val | ((uint64_t(char(t))) << 56);
  next_tape_loc++;
}

template<typename T>
simdjson_really_inline void tape_writer::append2(uint64_t val, T val2, internal::tape_type t) noexcept {
  append(val, t);
  static_assert(sizeof(val2) == sizeof(*next_tape_loc), "Type is not 64 bits!");
  memcpy(next_tape_loc, &val2, sizeof(val2));
  next_tape_loc++;
}

simdjson_really_inline void tape_writer::write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept {
  tape_loc = val | ((uint64_t(char(t))) << 56);
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/tape_writer.h */
/* begin file src/generic/stage2/atomparsing.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_really_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
SIMDJSON_WARN_UNUSED
simdjson_really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | is_not_structural_or_whitespace(src[4])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | is_not_structural_or_whitespace(src[5])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | is_not_structural_or_whitespace(src[4])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/atomparsing.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_builder {
  template<bool STREAMING>
  SIMDJSON_WARN_UNUSED static simdjson_really_inline error_code parse_document(
    dom_parser_implementation &dom_parser,
    dom::document &doc) noexcept;

  /** Called when a non-empty document starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_document_start(json_iterator &iter) noexcept;
  /** Called when a non-empty document ends without error. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_document_end(json_iterator &iter) noexcept;

  /** Called when a non-empty array starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_array_start(json_iterator &iter) noexcept;
  /** Called when a non-empty array ends. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_array_end(json_iterator &iter) noexcept;
  /** Called when an empty array is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_empty_array(json_iterator &iter) noexcept;

  /** Called when a non-empty object starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_object_start(json_iterator &iter) noexcept;
  /**
   * Called when a key in a field is encountered.
   *
   * primitive, visit_object_start, visit_empty_object, visit_array_start, or visit_empty_array
   * will be called after this with the field value.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_key(json_iterator &iter, const uint8_t *key) noexcept;
  /** Called when a non-empty object ends. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_object_end(json_iterator &iter) noexcept;
  /** Called when an empty object is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_empty_object(json_iterator &iter) noexcept;

  /**
   * Called when a string, number, boolean or null is found.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_primitive(json_iterator &iter, const uint8_t *value) noexcept;
  /**
   * Called when a string, number, boolean or null is found at the top level of a document (i.e.
   * when there is no array or object and the entire document is a single string, number, boolean or
   * null.
   *
   * This is separate from primitive() because simdjson's normal primitive parsing routines assume
   * there is at least one more token after the value, which is only true in an array or object.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_string(json_iterator &iter, const uint8_t *value, bool key = false) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_number(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_string(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_number(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

  /** Called each time a new field or element in an array or object is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code increment_count(json_iterator &iter) noexcept;

  /** Next location to write to tape */
  tape_writer tape;
private:
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc;

  simdjson_really_inline tape_builder(dom::document &doc) noexcept;

  simdjson_really_inline uint32_t next_tape_index(json_iterator &iter) const noexcept;
  simdjson_really_inline void start_container(json_iterator &iter) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  simdjson_really_inline uint8_t *on_start_string(json_iterator &iter) noexcept;
  simdjson_really_inline void on_end_string(uint8_t *dst) noexcept;
}; // class tape_builder

template<bool STREAMING>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::parse_document(
    dom_parser_implementation &dom_parser,
    dom::document &doc) noexcept {
  dom_parser.doc = &doc;
  json_iterator iter(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
  tape_builder builder(doc);
  return iter.walk_document<STREAMING>(builder);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_root_primitive(*this, value);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_primitive(*this, value);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_empty_object(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_empty_array(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_document_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_object_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_array_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_object_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_array_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_document_end(json_iterator &iter) noexcept {
  constexpr uint32_t start_tape_index = 0;
  tape.append(start_tape_index, internal::tape_type::ROOT);
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter), internal::tape_type::ROOT);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_key(json_iterator &iter, const uint8_t *key) noexcept {
  return visit_string(iter, key, true);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::increment_count(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].count++; // we have a key value pair in the object at parser.dom_parser.depth - 1
  return SUCCESS;
}

simdjson_really_inline tape_builder::tape_builder(dom::document &doc) noexcept : tape{doc.tape.get()}, current_string_buf_loc{doc.string_buf.get()} {}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_string(json_iterator &iter, const uint8_t *value, bool key) noexcept {
  iter.log_value(key ? "key" : "string");
  uint8_t *dst = on_start_string(iter);
  dst = stringparsing::parse_string(value, dst);
  if (dst == nullptr) {
    iter.log_error("Invalid escape in string");
    return STRING_ERROR;
  }
  on_end_string(dst);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_string(json_iterator &iter, const uint8_t *value) noexcept {
  return visit_string(iter, value);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_number(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("number");
  return numberparsing::parse_number(value, tape);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_number(json_iterator &iter, const uint8_t *value) noexcept {
  //
  // We need to make a copy to make sure that the string is space terminated.
  // This is not about padding the input, which should already padded up
  // to len + SIMDJSON_PADDING. However, we have no control at this stage
  // on how the padding was done. What if the input string was padded with nulls?
  // It is quite common for an input string to have an extra null character (C string).
  // We do not want to allow 9\0 (where \0 is the null character) inside a JSON
  // document, but the string "9\0" by itself is fine. So we make a copy and
  // pad the input with spaces when we know that there is just one input element.
  // This copy is relatively expensive, but it will almost never be called in
  // practice unless you are in the strange scenario where you have many JSON
  // documents made of single atoms.
  //
  uint8_t *copy = static_cast<uint8_t *>(malloc(iter.remaining_len() + SIMDJSON_PADDING));
  if (copy == nullptr) { return MEMALLOC; }
  memcpy(copy, value, iter.remaining_len());
  memset(copy + iter.remaining_len(), ' ', SIMDJSON_PADDING);
  error_code error = visit_number(iter, copy);
  free(copy);
  return error;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  if (!atomparsing::is_valid_true_atom(value)) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  if (!atomparsing::is_valid_true_atom(value, iter.remaining_len())) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  if (!atomparsing::is_valid_false_atom(value)) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  if (!atomparsing::is_valid_false_atom(value, iter.remaining_len())) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  if (!atomparsing::is_valid_null_atom(value)) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  if (!atomparsing::is_valid_null_atom(value, iter.remaining_len())) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

// private:

simdjson_really_inline uint32_t tape_builder::next_tape_index(json_iterator &iter) const noexcept {
  return uint32_t(tape.next_tape_loc - iter.dom_parser.doc->tape.get());
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  auto start_index = next_tape_index(iter);
  tape.append(start_index+2, start);
  tape.append(start_index, end);
  return SUCCESS;
}

simdjson_really_inline void tape_builder::start_container(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].tape_index = next_tape_index(iter);
  iter.dom_parser.open_containers[iter.depth].count = 0;
  tape.skip(); // We don't actually *write* the start element until the end.
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  // Write the ending tape element, pointing at the start location
  const uint32_t start_tape_index = iter.dom_parser.open_containers[iter.depth].tape_index;
  tape.append(start_tape_index, end);
  // Write the start tape element, pointing at the end location (and including count)
  // count can overflow if it exceeds 24 bits... so we saturate
  // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
  const uint32_t count = iter.dom_parser.open_containers[iter.depth].count;
  const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter) | (uint64_t(cntsat) << 32), start);
  return SUCCESS;
}

simdjson_really_inline uint8_t *tape_builder::on_start_string(json_iterator &iter) noexcept {
  // we advance the point, accounting for the fact that we have a NULL termination
  tape.append(current_string_buf_loc - iter.dom_parser.doc->string_buf.get(), internal::tape_type::STRING);
  return current_string_buf_loc + sizeof(uint32_t);
}

simdjson_really_inline void tape_builder::on_end_string(uint8_t *dst) noexcept {
  uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
  // TODO check for overflow in case someone has a crazy string (>=4GB?)
  // But only add the overflow check when the document itself exceeds 4GB
  // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
  memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
  // NULL termination is still handy if you expect all your strings to
  // be NULL terminated? It comes at a small cost
  *dst = 0;
  current_string_buf_loc = dst + 1;
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/atomparsing.h */

//
// Implementation-specific overrides
//
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

simdjson_really_inline uint64_t json_string_scanner::find_escaped(uint64_t backslash) {
  if (!backslash) { uint64_t escaped = prev_escaped; prev_escaped = 0; return escaped; }
  return find_escaped_branchless(backslash);
}

} // namespace stage1

SIMDJSON_WARN_UNUSED error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return haswell::stage1::json_minifier::minify<128>(buf, len, dst, dst_len);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, bool streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return haswell::stage1::json_structural_indexer::index<128>(_buf, _len, *this, streaming);
}

SIMDJSON_WARN_UNUSED bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return haswell::stage1::generic_validate_utf8(buf,len);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, false);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/haswell/end_implementation.h */
#undef SIMDJSON_IMPLEMENTATION
SIMDJSON_UNTARGET_REGION
/* end file src/haswell/end_implementation.h */
/* end file src/haswell/end_implementation.h */
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
/* begin file src/westmere/implementation.cpp */
/* begin file src/westmere/begin_implementation.h */
#define SIMDJSON_IMPLEMENTATION westmere
#define SIMDJSON_TARGET_WESTMERE SIMDJSON_TARGET_REGION("sse4.2,pclmul")

/* begin file src/westmere/intrinsics.h */
#ifndef SIMDJSON_WESTMERE_INTRINSICS_H
#define SIMDJSON_WESTMERE_INTRINSICS_H

#ifdef SIMDJSON_VISUAL_STUDIO
// under clang within visual studio, this will include <x86intrin.h>
#include <intrin.h> // visual studio or clang
#else
#include <x86intrin.h> // elsewhere
#endif // SIMDJSON_VISUAL_STUDIO


#ifdef SIMDJSON_CLANG_VISUAL_STUDIO
/**
 * You are not supposed, normally, to include these
 * headers directly. Instead you should either include intrin.h
 * or x86intrin.h. However, when compiling with clang
 * under Windows (i.e., when _MSC_VER is set), these headers
 * only get included *if* the corresponding features are detected
 * from macros:
 */
#include <smmintrin.h>  // for _mm_alignr_epi8
#include <wmmintrin.h>  // for  _mm_clmulepi64_si128
#endif



#endif // SIMDJSON_WESTMERE_INTRINSICS_H
/* end file src/westmere/intrinsics.h */
/* westmere/implementation.h already included: #include "westmere/implementation.h" */

SIMDJSON_TARGET_WESTMERE

/* begin file src/westmere/bitmanipulation.h */
#ifndef SIMDJSON_WESTMERE_BITMANIPULATION_H
#define SIMDJSON_WESTMERE_BITMANIPULATION_H

namespace {
namespace SIMDJSON_IMPLEMENTATION {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
NO_SANITIZE_UNDEFINED
simdjson_really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long ret;
  // Search the mask data from least significant bit (LSB) 
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else // SIMDJSON_REGULAR_VISUAL_STUDIO
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
}

/* result might be undefined when input_num is zero */
simdjson_really_inline int leading_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB) 
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif// SIMDJSON_REGULAR_VISUAL_STUDIO
}

#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
simdjson_really_inline unsigned __int64 count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num);// Visual Studio wants two underscores
}
#else
simdjson_really_inline long long int count_ones(uint64_t input_num) {
  return _popcnt64(input_num);
}
#endif

simdjson_really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   (unsigned long long *)result);
#endif
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#endif // SIMDJSON_WESTMERE_BITMANIPULATION_H
/* end file src/westmere/bitmanipulation.h */
/* begin file src/westmere/bitmask.h */
#ifndef SIMDJSON_WESTMERE_BITMASK_H
#define SIMDJSON_WESTMERE_BITMASK_H

namespace {
namespace SIMDJSON_IMPLEMENTATION {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_really_inline uint64_t prefix_xor(const uint64_t bitmask) {
  // There should be no such thing with a processing supporting avx2
  // but not clmul.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return _mm_cvtsi128_si64(result);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#endif // SIMDJSON_WESTMERE_BITMASK_H
/* end file src/westmere/bitmask.h */
/* begin file src/westmere/simd.h */
#ifndef SIMDJSON_WESTMERE_SIMD_H
#define SIMDJSON_WESTMERE_SIMD_H

/* simdprune_tables.h already included: #include "simdprune_tables.h" */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace simd {

  template<typename Child>
  struct base {
    __m128i value;

    // Zero constructor
    simdjson_really_inline base() : value{__m128i()} {}

    // Conversion from SIMD register
    simdjson_really_inline base(const __m128i _value) : value(_value) {}

    // Conversion to SIMD register
    simdjson_really_inline operator const __m128i&() const { return this->value; }
    simdjson_really_inline operator __m128i&() { return this->value; }

    // Bit operations
    simdjson_really_inline Child operator|(const Child other) const { return _mm_or_si128(*this, other); }
    simdjson_really_inline Child operator&(const Child other) const { return _mm_and_si128(*this, other); }
    simdjson_really_inline Child operator^(const Child other) const { return _mm_xor_si128(*this, other); }
    simdjson_really_inline Child bit_andnot(const Child other) const { return _mm_andnot_si128(other, *this); }
    simdjson_really_inline Child& operator|=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast | other; return *this_cast; }
    simdjson_really_inline Child& operator&=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast & other; return *this_cast; }
    simdjson_really_inline Child& operator^=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    simdjson_really_inline base8() : base<simd8<T>>() {}
    simdjson_really_inline base8(const __m128i _value) : base<simd8<T>>(_value) {}

    simdjson_really_inline Mask operator==(const simd8<T> other) const { return _mm_cmpeq_epi8(*this, other); }

    static const int SIZE = sizeof(base<simd8<T>>::value);

    template<int N=1>
    simdjson_really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return _mm_alignr_epi8(*this, prev_chunk, 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static simdjson_really_inline simd8<bool> splat(bool _value) { return _mm_set1_epi8(uint8_t(-(!!_value))); }

    simdjson_really_inline simd8<bool>() : base8() {}
    simdjson_really_inline simd8<bool>(const __m128i _value) : base8<bool>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8<bool>(bool _value) : base8<bool>(splat(_value)) {}

    simdjson_really_inline int to_bitmask() const { return _mm_movemask_epi8(*this); }
    simdjson_really_inline bool any() const { return !_mm_testz_si128(*this, *this); }
    simdjson_really_inline simd8<bool> operator~() const { return *this ^ true; }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static simdjson_really_inline simd8<T> splat(T _value) { return _mm_set1_epi8(_value); }
    static simdjson_really_inline simd8<T> zero() { return _mm_setzero_si128(); }
    static simdjson_really_inline simd8<T> load(const T values[16]) {
      return _mm_loadu_si128(reinterpret_cast<const __m128i *>(values));
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static simdjson_really_inline simd8<T> repeat_16(
      T v0,  T v1,  T v2,  T v3,  T v4,  T v5,  T v6,  T v7,
      T v8,  T v9,  T v10, T v11, T v12, T v13, T v14, T v15
    ) {
      return simd8<T>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    simdjson_really_inline base8_numeric() : base8<T>() {}
    simdjson_really_inline base8_numeric(const __m128i _value) : base8<T>(_value) {}

    // Store to array
    simdjson_really_inline void store(T dst[16]) const { return _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), *this); }

    // Override to distinguish from bool version
    simdjson_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

    // Addition/subtraction are the same for signed and unsigned
    simdjson_really_inline simd8<T> operator+(const simd8<T> other) const { return _mm_add_epi8(*this, other); }
    simdjson_really_inline simd8<T> operator-(const simd8<T> other) const { return _mm_sub_epi8(*this, other); }
    simdjson_really_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *(simd8<T>*)this; }
    simdjson_really_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *(simd8<T>*)this; }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return _mm_shuffle_epi8(lookup_table, *this);
    }

    // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted as a bitset).
    // Passing a 0 value for mask would be equivalent to writing out every byte to output.
    // Only the first 16 - count_ones(mask) bytes of the result are significant but 16 bytes
    // get written.
    // Design consideration: it seems like a function with the
    // signature simd8<L> compress(uint32_t mask) would be
    // sensible, but the AVX ISA makes this kind of approach difficult.
    template<typename L>
    simdjson_really_inline void compress(uint16_t mask, L * output) const {
      // this particular implementation was inspired by work done by @animetosho
      // we do it in two steps, first 8 bytes and then second 8 bytes
      uint8_t mask1 = uint8_t(mask); // least significant 8 bits
      uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
      // next line just loads the 64-bit values thintable_epi8[mask1] and
      // thintable_epi8[mask2] into a 128-bit register, using only
      // two instructions on most compilers.
      __m128i shufmask =  _mm_set_epi64x(thintable_epi8[mask2], thintable_epi8[mask1]);
      // we increment by 0x08 the second half of the mask
      shufmask =
      _mm_add_epi8(shufmask, _mm_set_epi32(0x08080808, 0x08080808, 0, 0));
      // this is the version "nearly pruned"
      __m128i pruned = _mm_shuffle_epi8(*this, shufmask);
      // we still need to put the two halves together.
      // we compute the popcount of the first half:
      int pop1 = BitsSetTable256mul2[mask1];
      // then load the corresponding mask, what it does is to write
      // only the first pop1 bytes from the first 8 bytes, and then
      // it fills in with the bytes from the second 8 bytes + some filling
      // at the end.
      __m128i compactmask =
      _mm_loadu_si128((const __m128i *)(pshufb_combine_table + pop1 * 8));
      __m128i answer = _mm_shuffle_epi8(pruned, compactmask);
      _mm_storeu_si128(( __m128i *)(output), answer);
    }

    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(
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
    simdjson_really_inline simd8() : base8_numeric<int8_t>() {}
    simdjson_really_inline simd8(const __m128i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const int8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(_mm_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Order-sensitive comparisons
    simdjson_really_inline simd8<int8_t> max(const simd8<int8_t> other) const { return _mm_max_epi8(*this, other); }
    simdjson_really_inline simd8<int8_t> min(const simd8<int8_t> other) const { return _mm_min_epi8(*this, other); }
    simdjson_really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return _mm_cmpgt_epi8(*this, other); }
    simdjson_really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return _mm_cmpgt_epi8(other, *this); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    simdjson_really_inline simd8() : base8_numeric<uint8_t>() {}
    simdjson_really_inline simd8(const __m128i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const uint8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(_mm_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Saturated math
    simdjson_really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return _mm_adds_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return _mm_subs_epu8(*this, other); }

    // Order-specific operations
    simdjson_really_inline simd8<uint8_t> max(const simd8<uint8_t> other) const { return _mm_max_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> min(const simd8<uint8_t> other) const { return _mm_min_epu8(*this, other); }
    // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return this->saturating_sub(other); }
    // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return other.saturating_sub(*this); }
    simdjson_really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return other.max(*this) == other; }
    simdjson_really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return other.min(*this) == other; }
    simdjson_really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }
    simdjson_really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }

    // Bit-specific operations
    simdjson_really_inline simd8<bool> bits_not_set() const { return *this == uint8_t(0); }
    simdjson_really_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const { return (*this & bits).bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set() const { return ~this->bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return ~this->bits_not_set(bits); }
    simdjson_really_inline bool is_ascii() const { return _mm_movemask_epi8(*this) == 0; }
    simdjson_really_inline bool bits_not_set_anywhere() const { return _mm_testz_si128(*this, *this); }
    simdjson_really_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    simdjson_really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const { return _mm_testz_si128(*this, bits); }
    simdjson_really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(_mm_srli_epi16(*this, N)) & uint8_t(0xFFu >> N); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(_mm_slli_epi16(*this, N)) & uint8_t(0xFFu << N); }
    // Get one of the bits and make a bitmask out of it.
    // e.g. value.get_bit<7>() gets the high bit
    template<int N>
    simdjson_really_inline int get_bit() const { return _mm_movemask_epi8(_mm_slli_epi16(*this, 7-N)); }
  };

  template<typename T>
  struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 4, "Westmere kernel should use four registers per 64-byte block.");
    const simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T>& o) = delete; // no copy allowed
    simd8x64<T>& operator=(const simd8<T> other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed

    simdjson_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2, const simd8<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    simdjson_really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+16), simd8<T>::load(ptr+32), simd8<T>::load(ptr+48)} {}

    simdjson_really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store(ptr+sizeof(simd8<T>)*3);
    }

    simdjson_really_inline simd8<T> reduce_or() const {
      return (this->chunks[0] | this->chunks[1]) | (this->chunks[2] | this->chunks[3]);
    }

    simdjson_really_inline void compress(uint64_t mask, T * output) const {
      this->chunks[0].compress(uint16_t(mask), output);
      this->chunks[1].compress(uint16_t(mask >> 16), output + 16 - count_ones(mask & 0xFFFF));
      this->chunks[2].compress(uint16_t(mask >> 32), output + 32 - count_ones(mask & 0xFFFFFFFF));
      this->chunks[3].compress(uint16_t(mask >> 48), output + 48 - count_ones(mask & 0xFFFFFFFFFFFF));
    }

    simdjson_really_inline uint64_t to_bitmask() const {
      uint64_t r0 = uint32_t(this->chunks[0].to_bitmask());
      uint64_t r1 =                       this->chunks[1].to_bitmask();
      uint64_t r2 =                       this->chunks[2].to_bitmask();
      uint64_t r3 =                       this->chunks[3].to_bitmask();
      return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
    }

    simdjson_really_inline simd8x64<T> bit_or(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return simd8x64<T>(
        this->chunks[0] | mask,
        this->chunks[1] | mask,
        this->chunks[2] | mask,
        this->chunks[3] | mask
      );
    }

    simdjson_really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] == mask,
        this->chunks[1] == mask,
        this->chunks[2] == mask,
        this->chunks[3] == mask
      ).to_bitmask();
    }

    simdjson_really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] <= mask,
        this->chunks[1] <= mask,
        this->chunks[2] <= mask,
        this->chunks[3] <= mask
      ).to_bitmask();
    }
  }; // struct simd8x64<T>

} // namespace simd
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#endif // SIMDJSON_WESTMERE_SIMD_INPUT_H
/* end file src/westmere/simd.h */
/* end file src/westmere/simd.h */
/* begin file src/westmere/dom_parser_implementation.h */
#ifndef SIMDJSON_WESTMERE_DOM_PARSER_IMPLEMENTATION_H
#define SIMDJSON_WESTMERE_DOM_PARSER_IMPLEMENTATION_H

/* begin file src/generic/dom_parser_implementation.h */
/* isadetection.h already included: #include "isadetection.h" */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  simdjson_really_inline dom_parser_implementation();
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation & operator=(const dom_parser_implementation &) = delete;

  SIMDJSON_WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  SIMDJSON_WARN_UNUSED error_code check_for_unclosed_array() noexcept;
  SIMDJSON_WARN_UNUSED error_code stage2(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code stage2_next(dom::document &doc) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_capacity(size_t capacity) noexcept final;
  SIMDJSON_WARN_UNUSED error_code set_max_depth(size_t max_depth) noexcept final;
};

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/generic/stage1/allocate.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {
namespace allocate {

//
// Allocates stage 1 internal state and outputs in the parser
//
simdjson_really_inline error_code set_capacity(internal::dom_parser_implementation &parser, size_t capacity) {
  size_t max_structures = SIMDJSON_ROUNDUP_N(capacity, 64) + 2 + 7;
  parser.structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] );
  if (!parser.structural_indexes) { return MEMALLOC; }
  parser.structural_indexes[0] = 0;
  parser.n_structural_indexes = 0;
  return SUCCESS;
}

} // namespace allocate
} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/allocate.h */
/* begin file src/generic/stage2/allocate.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace allocate {

//
// Allocates stage 2 internal state and outputs in the parser
//
simdjson_really_inline error_code set_max_depth(dom_parser_implementation &parser, size_t max_depth) {
  parser.open_containers.reset(new (std::nothrow) open_container[max_depth]);
  parser.is_array.reset(new (std::nothrow) bool[max_depth]);

  if (!parser.is_array || !parser.open_containers) {
    return MEMALLOC;
  }
  return SUCCESS;
}

} // namespace allocate
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/allocate.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

simdjson_really_inline dom_parser_implementation::dom_parser_implementation() {}

// Leaving these here so they can be inlined if so desired
SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  error_code err = stage1::allocate::set_capacity(*this, capacity);
  if (err) { _capacity = 0; return err; }
  _capacity = capacity;
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  error_code err = stage2::allocate::set_max_depth(*this, max_depth);
  if (err) { _max_depth = 0; return err; }
  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/allocate.h */

#endif // SIMDJSON_WESTMERE_DOM_PARSER_IMPLEMENTATION_H
/* end file src/generic/stage2/allocate.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {

SIMDJSON_WARN_UNUSED error_code implementation::create_dom_parser_implementation(
  size_t capacity,
  size_t max_depth,
  std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  dst.reset( new (std::nothrow) dom_parser_implementation() );
  if (!dst) { return MEMALLOC; }
  dst->set_capacity(capacity);
  dst->set_max_depth(max_depth);
  return SUCCESS;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/westmere/end_implementation.h */
#undef SIMDJSON_IMPLEMENTATION
SIMDJSON_UNTARGET_REGION
/* end file src/westmere/end_implementation.h */
/* end file src/westmere/end_implementation.h */
/* begin file src/westmere/dom_parser_implementation.cpp */
/* begin file src/westmere/begin_implementation.h */
#define SIMDJSON_IMPLEMENTATION westmere
#define SIMDJSON_TARGET_WESTMERE SIMDJSON_TARGET_REGION("sse4.2,pclmul")

/* westmere/intrinsics.h already included: #include "westmere/intrinsics.h" // Generally need to be included outside SIMDJSON_TARGET_REGION */
/* westmere/implementation.h already included: #include "westmere/implementation.h" */

SIMDJSON_TARGET_WESTMERE

/* westmere/bitmanipulation.h already included: #include "westmere/bitmanipulation.h" */
/* westmere/bitmask.h already included: #include "westmere/bitmask.h" */
/* westmere/simd.h already included: #include "westmere/simd.h" */
/* end file src/westmere/begin_implementation.h */
/* westmere/dom_parser_implementation.h already included: #include "westmere/dom_parser_implementation.h" */
/* begin file src/generic/stage2/jsoncharutils.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_really_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return structural_or_whitespace_negated[c];
}

simdjson_really_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = digit_to_val32[630 + src[0]];
  uint32_t v2 = digit_to_val32[420 + src[1]];
  uint32_t v3 = digit_to_val32[210 + src[2]];
  uint32_t v4 = digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_really_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#ifdef SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_really_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_really_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

simdjson_really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
#ifdef _M_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // _M_ARM64
#else // defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
  __uint128_t r = ((__uint128_t)value1) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/jsoncharutils.h */

//
// Stage 1
//

namespace {
namespace SIMDJSON_IMPLEMENTATION {

using namespace simd;

struct json_character_block {
  static simdjson_really_inline json_character_block classify(const simd::simd8x64<uint8_t>& in);

  simdjson_really_inline uint64_t whitespace() const { return _whitespace; }
  simdjson_really_inline uint64_t op() const { return _op; }
  simdjson_really_inline uint64_t scalar() { return ~(op() | whitespace()); }

  uint64_t _whitespace;
  uint64_t _op;
};

simdjson_really_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t>& in) {
  // These lookups rely on the fact that anything < 127 will match the lower 4 bits, which is why
  // we can't use the generic lookup_16.
  auto whitespace_table = simd8<uint8_t>::repeat_16(' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100);
  auto op_table = simd8<uint8_t>::repeat_16(',', '}', 0, 0, 0xc0u, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{');

  // We compute whitespace and op separately. If the code later only use one or the
  // other, given the fact that all functions are aggressively inlined, we can
  // hope that useless computations will be omitted. This is namely case when
  // minifying (we only need whitespace).

  uint64_t whitespace = simd8x64<bool>(
        in.chunks[0] == simd8<uint8_t>(_mm_shuffle_epi8(whitespace_table, in.chunks[0])),
        in.chunks[1] == simd8<uint8_t>(_mm_shuffle_epi8(whitespace_table, in.chunks[1])),
        in.chunks[2] == simd8<uint8_t>(_mm_shuffle_epi8(whitespace_table, in.chunks[2])),
        in.chunks[3] == simd8<uint8_t>(_mm_shuffle_epi8(whitespace_table, in.chunks[3]))
  ).to_bitmask();

  // | 32 handles the fact that { } and [ ] are exactly 32 bytes apart
  uint64_t op = simd8x64<bool>(
        (in.chunks[0] | 32) == simd8<uint8_t>(_mm_shuffle_epi8(op_table, in.chunks[0]-',')),
        (in.chunks[1] | 32) == simd8<uint8_t>(_mm_shuffle_epi8(op_table, in.chunks[1]-',')),
        (in.chunks[2] | 32) == simd8<uint8_t>(_mm_shuffle_epi8(op_table, in.chunks[2]-',')),
        (in.chunks[3] | 32) == simd8<uint8_t>(_mm_shuffle_epi8(op_table, in.chunks[3]-','))
  ).to_bitmask();
  return { whitespace, op };
}

simdjson_really_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  return input.reduce_or().is_ascii();
}

SIMDJSON_UNUSED simdjson_really_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0b11000000u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

simdjson_really_inline simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_third_byte | is_fourth_byte) > int8_t(0);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/generic/stage1/utf8_lookup4_algorithm.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace utf8_validation {

using namespace simd;

  simdjson_really_inline simd8<uint8_t> check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1) {
// Bit 0 = Too Short (lead byte/ASCII followed by lead byte/ASCII)
// Bit 1 = Too Long (ASCII followed by continuation)
// Bit 2 = Overlong 3-byte
// Bit 4 = Surrogate
// Bit 5 = Overlong 2-byte
// Bit 7 = Two Continuations
    constexpr const uint8_t TOO_SHORT   = 1<<0; // 11______ 0_______
                                                // 11______ 11______
    constexpr const uint8_t TOO_LONG    = 1<<1; // 0_______ 10______
    constexpr const uint8_t OVERLONG_3  = 1<<2; // 11100000 100_____
    constexpr const uint8_t SURROGATE   = 1<<4; // 11101101 101_____
    constexpr const uint8_t OVERLONG_2  = 1<<5; // 1100000_ 10______
    constexpr const uint8_t TWO_CONTS   = 1<<7; // 10______ 10______
    constexpr const uint8_t TOO_LARGE   = 1<<3; // 11110100 1001____
                                                // 11110100 101_____
                                                // 11110101 1001____
                                                // 11110101 101_____
                                                // 1111011_ 1001____
                                                // 1111011_ 101_____
                                                // 11111___ 1001____
                                                // 11111___ 101_____
    constexpr const uint8_t TOO_LARGE_1000 = 1<<6;
                                                // 11110101 1000____
                                                // 1111011_ 1000____
                                                // 11111___ 1000____
    constexpr const uint8_t OVERLONG_4  = 1<<6; // 11110000 1000____

    const simd8<uint8_t> byte_1_high = prev1.shr<4>().lookup_16<uint8_t>(
      // 0_______ ________ <ASCII in byte 1>
      TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
      TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
      // 10______ ________ <continuation in byte 1>
      TWO_CONTS, TWO_CONTS, TWO_CONTS, TWO_CONTS,
      // 1100____ ________ <two byte lead in byte 1>
      TOO_SHORT | OVERLONG_2,
      // 1101____ ________ <two byte lead in byte 1>
      TOO_SHORT,
      // 1110____ ________ <three byte lead in byte 1>
      TOO_SHORT | OVERLONG_3 | SURROGATE,
      // 1111____ ________ <four+ byte lead in byte 1>
      TOO_SHORT | TOO_LARGE | TOO_LARGE_1000 | OVERLONG_4
    );
    constexpr const uint8_t CARRY = TOO_SHORT | TOO_LONG | TWO_CONTS; // These all have ____ in byte 1 .
    const simd8<uint8_t> byte_1_low = (prev1 & 0x0F).lookup_16<uint8_t>(
      // ____0000 ________
      CARRY | OVERLONG_3 | OVERLONG_2 | OVERLONG_4,
      // ____0001 ________
      CARRY | OVERLONG_2,
      // ____001_ ________
      CARRY,
      CARRY,

      // ____0100 ________
      CARRY | TOO_LARGE,
      // ____0101 ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      // ____011_ ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,

      // ____1___ ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      // ____1101 ________
      CARRY | TOO_LARGE | TOO_LARGE_1000 | SURROGATE,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000
    );
    const simd8<uint8_t> byte_2_high = input.shr<4>().lookup_16<uint8_t>(
      // ________ 0_______ <ASCII in byte 2>
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,

      // ________ 1000____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE_1000 | OVERLONG_4,
      // ________ 1001____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE,
      // ________ 101_____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,

      // ________ 11______
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT
    );
    return (byte_1_high & byte_1_low & byte_2_high);
  }
  simdjson_really_inline simd8<uint8_t> check_multibyte_lengths(const simd8<uint8_t> input,
      const simd8<uint8_t> prev_input, const simd8<uint8_t> sc) {
    simd8<uint8_t> prev2 = input.prev<2>(prev_input);
    simd8<uint8_t> prev3 = input.prev<3>(prev_input);
    simd8<uint8_t> must23 = simd8<uint8_t>(must_be_2_3_continuation(prev2, prev3));
    simd8<uint8_t> must23_80 = must23 & uint8_t(0x80);
    return must23_80 ^ sc;
  }

  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  simdjson_really_inline simd8<uint8_t> is_incomplete(const simd8<uint8_t> input) {
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
    simdjson_really_inline void check_utf8_bytes(const simd8<uint8_t> input, const simd8<uint8_t> prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      simd8<uint8_t> prev1 = input.prev<1>(prev_input);
      simd8<uint8_t> sc = check_special_cases(input, prev1);
      this->error |= check_multibyte_lengths(input, prev_input, sc);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short.
    simdjson_really_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error |= this->prev_incomplete;
    }

    simdjson_really_inline void check_next_input(const simd8x64<uint8_t>& input) {
      if(simdjson_likely(is_ascii(input))) {
        this->error |= this->prev_incomplete;
      } else {
        // you might think that a for-loop would work, but under Visual Studio, it is not good enough.
        static_assert((simd8x64<uint8_t>::NUM_CHUNKS == 2) || (simd8x64<uint8_t>::NUM_CHUNKS == 4),
            "We support either two or four chunks per 64-byte block.");
        if(simd8x64<uint8_t>::NUM_CHUNKS == 2) {
          this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
          this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
        } else if(simd8x64<uint8_t>::NUM_CHUNKS == 4) {
          this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
          this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
          this->check_utf8_bytes(input.chunks[2], input.chunks[1]);
          this->check_utf8_bytes(input.chunks[3], input.chunks[2]);
        }
        this->prev_incomplete = is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1]);
        this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS-1];

      }
    }

    simdjson_really_inline error_code errors() {
      return this->error.any_bits_set_anywhere() ? error_code::UTF8_ERROR : error_code::SUCCESS;
    }

  }; // struct utf8_checker
} // namespace utf8_validation

using utf8_validation::utf8_checker;

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/utf8_lookup4_algorithm.h */
/* begin file src/generic/stage1/json_structural_indexer.h */
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

/* begin file src/generic/stage1/buf_block_reader.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {

// Walks through a buffer in block-sized increments, loading the last part with spaces
template<size_t STEP_SIZE>
struct buf_block_reader {
public:
  simdjson_really_inline buf_block_reader(const uint8_t *_buf, size_t _len);
  simdjson_really_inline size_t block_index();
  simdjson_really_inline bool has_full_block() const;
  simdjson_really_inline const uint8_t *full_block() const;
  /**
   * Get the last block, padded with spaces.
   *
   * There will always be a last block, with at least 1 byte, unless len == 0 (in which case this
   * function fills the buffer with spaces and returns 0. In particular, if len == STEP_SIZE there
   * will be 0 full_blocks and 1 remainder block with STEP_SIZE bytes and no spaces for padding.
   *
   * @return the number of effective characters in the last block.
   */
  simdjson_really_inline size_t get_remainder(uint8_t *dst) const;
  simdjson_really_inline void advance();
private:
  const uint8_t *buf;
  const size_t len;
  const size_t lenminusstep;
  size_t idx;
};

// Routines to print masks and text for debugging bitmask operations
SIMDJSON_UNUSED static char * format_input_text_64(const uint8_t *text) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    buf[i] = int8_t(text[i]) < ' ' ? '_' : int8_t(text[i]);
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

// Routines to print masks and text for debugging bitmask operations
SIMDJSON_UNUSED static char * format_input_text(const simd8x64<uint8_t>& in) {
  static char *buf = (char*)malloc(sizeof(simd8x64<uint8_t>) + 1);
  in.store((uint8_t*)buf);
  for (size_t i=0; i<sizeof(simd8x64<uint8_t>); i++) {
    if (buf[i] < ' ') { buf[i] = '_'; }
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

SIMDJSON_UNUSED static char * format_mask(uint64_t mask) {
  static char *buf = (char*)malloc(64 + 1);
  for (size_t i=0; i<64; i++) {
    buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
  }
  buf[64] = '\0';
  return buf;
}

template<size_t STEP_SIZE>
simdjson_really_inline buf_block_reader<STEP_SIZE>::buf_block_reader(const uint8_t *_buf, size_t _len) : buf{_buf}, len{_len}, lenminusstep{len < STEP_SIZE ? 0 : len - STEP_SIZE}, idx{0} {}

template<size_t STEP_SIZE>
simdjson_really_inline size_t buf_block_reader<STEP_SIZE>::block_index() { return idx; }

template<size_t STEP_SIZE>
simdjson_really_inline bool buf_block_reader<STEP_SIZE>::has_full_block() const {
  return idx < lenminusstep;
}

template<size_t STEP_SIZE>
simdjson_really_inline const uint8_t *buf_block_reader<STEP_SIZE>::full_block() const {
  return &buf[idx];
}

template<size_t STEP_SIZE>
simdjson_really_inline size_t buf_block_reader<STEP_SIZE>::get_remainder(uint8_t *dst) const {
  memset(dst, 0x20, STEP_SIZE); // memset STEP_SIZE because it's more efficient to write out 8 or 16 bytes at once.
  memcpy(dst, buf + idx, len - idx);
  return len - idx;
}

template<size_t STEP_SIZE>
simdjson_really_inline void buf_block_reader<STEP_SIZE>::advance() {
  idx += STEP_SIZE;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/buf_block_reader.h */
/* begin file src/generic/stage1/json_string_scanner.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

struct json_string_block {
  // Escaped characters (characters following an escape() character)
  simdjson_really_inline uint64_t escaped() const { return _escaped; }
  // Escape characters (backslashes that are not escaped--i.e. in \\, includes only the first \)
  simdjson_really_inline uint64_t escape() const { return _backslash & ~_escaped; }
  // Real (non-backslashed) quotes
  simdjson_really_inline uint64_t quote() const { return _quote; }
  // Start quotes of strings
  simdjson_really_inline uint64_t string_start() const { return _quote & _in_string; }
  // End quotes of strings
  simdjson_really_inline uint64_t string_end() const { return _quote & ~_in_string; }
  // Only characters inside the string (not including the quotes)
  simdjson_really_inline uint64_t string_content() const { return _in_string & ~_quote; }
  // Return a mask of whether the given characters are inside a string (only works on non-quotes)
  simdjson_really_inline uint64_t non_quote_inside_string(uint64_t mask) const { return mask & _in_string; }
  // Return a mask of whether the given characters are inside a string (only works on non-quotes)
  simdjson_really_inline uint64_t non_quote_outside_string(uint64_t mask) const { return mask & ~_in_string; }
  // Tail of string (everything except the start quote)
  simdjson_really_inline uint64_t string_tail() const { return _in_string ^ _quote; }

  // backslash characters
  uint64_t _backslash;
  // escaped characters (backslashed--does not include the hex characters after \u)
  uint64_t _escaped;
  // real quotes (non-backslashed ones)
  uint64_t _quote;
  // string characters (includes start quote but not end quote)
  uint64_t _in_string;
};

// Scans blocks for string characters, storing the state necessary to do so
class json_string_scanner {
public:
  simdjson_really_inline json_string_block next(const simd::simd8x64<uint8_t>& in);
  simdjson_really_inline error_code finish(bool streaming);

private:
  // Intended to be defined by the implementation
  simdjson_really_inline uint64_t find_escaped(uint64_t escape);
  simdjson_really_inline uint64_t find_escaped_branchless(uint64_t escape);

  // Whether the last iteration was still inside a string (all 1's = true, all 0's = false).
  uint64_t prev_in_string = 0ULL;
  // Whether the first character of the next iteration is escaped.
  uint64_t prev_escaped = 0ULL;
};

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
simdjson_really_inline uint64_t json_string_scanner::find_escaped_branchless(uint64_t backslash) {
  // If there was overflow, pretend the first character isn't a backslash
  backslash &= ~prev_escaped;
  uint64_t follows_escape = backslash << 1 | prev_escaped;

  // Get sequences starting on even bits by clearing out the odd series using +
  const uint64_t even_bits = 0x5555555555555555ULL;
  uint64_t odd_sequence_starts = backslash & ~even_bits & ~follows_escape;
  uint64_t sequences_starting_on_even_bits;
  prev_escaped = add_overflow(odd_sequence_starts, backslash, &sequences_starting_on_even_bits);
  uint64_t invert_mask = sequences_starting_on_even_bits << 1; // The mask we want to return is the *escaped* bits, not escapes.

  // Mask every other backslashed character as an escaped character
  // Flip the mask for sequences that start on even bits, to correct them
  return (even_bits ^ invert_mask) & follows_escape;
}

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
simdjson_really_inline json_string_block json_string_scanner::next(const simd::simd8x64<uint8_t>& in) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = find_escaped(backslash);
  const uint64_t quote = in.eq('"') & ~escaped;

  //
  // prefix_xor flips on bits inside the string (and flips off the end quote).
  //
  // Then we xor with prev_in_string: if we were in a string already, its effect is flipped
  // (characters inside strings are outside, and characters outside strings are inside).
  //
  const uint64_t in_string = prefix_xor(quote) ^ prev_in_string;

  //
  // Check if we're still in a string at the end of the box so the next block will know
  //
  // right shift of a signed value expected to be well-defined and standard
  // compliant as of C++20, John Regher from Utah U. says this is fine code
  //
  prev_in_string = uint64_t(static_cast<int64_t>(in_string) >> 63);

  // Use ^ to turn the beginning quote off, and the end quote on.
  return {
    backslash,
    escaped,
    quote,
    in_string
  };
}

simdjson_really_inline error_code json_string_scanner::finish(bool streaming) {
  if (prev_in_string and (not streaming)) {
    return UNCLOSED_STRING;
  }
  return SUCCESS;
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/json_string_scanner.h */
/* begin file src/generic/stage1/json_scanner.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

/**
 * A block of scanned json, with information on operators and scalars.
 *
 * We seek to identify pseudo-structural characters. Anything that is inside
 * a string must be omitted (hence  & ~_string.string_tail()).
 * Otherwise, pseudo-structural characters come in two forms.
 * 1. We have the structural characters ([,],{,},:, comma). The 
 *    term 'structural character' is from the JSON RFC.
 * 2. We have the 'scalar pseudo-structural characters'.
 *    Scalars are quotes, and any character except structural characters and white space.
 *
 * To identify the scalar pseudo-structural characters, we must look at what comes
 * before them: it must be a space, a quote or a structural characters.
 * Starting with simdjson v0.3, we identify them by
 * negation: we identify everything that is followed by a non-quote scalar,
 * and we negate that. Whatever remains must be a 'scalar pseudo-structural character'.
 */
struct json_block {
public:
  /**
   * The start of structurals.
   * In simdjson prior to v0.3, these were called the pseudo-structural characters.
   **/
  simdjson_really_inline uint64_t structural_start() { return potential_structural_start() & ~_string.string_tail(); }
  /** All JSON whitespace (i.e. not in a string) */
  simdjson_really_inline uint64_t whitespace() { return non_quote_outside_string(_characters.whitespace()); }

  // Helpers

  /** Whether the given characters are inside a string (only works on non-quotes) */
  simdjson_really_inline uint64_t non_quote_inside_string(uint64_t mask) { return _string.non_quote_inside_string(mask); }
  /** Whether the given characters are outside a string (only works on non-quotes) */
  simdjson_really_inline uint64_t non_quote_outside_string(uint64_t mask) { return _string.non_quote_outside_string(mask); }

  // string and escape characters
  json_string_block _string;
  // whitespace, structural characters ('operators'), scalars
  json_character_block _characters;
  // whether the previous character was a scalar
  uint64_t _follows_potential_nonquote_scalar;
private:
  // Potential structurals (i.e. disregarding strings)

  /**
   * structural elements ([,],{,},:, comma) plus scalar starts like 123, true and "abc".
   * They may reside inside a string.
   **/
  simdjson_really_inline uint64_t potential_structural_start() { return _characters.op() | potential_scalar_start(); }
  /**
   * The start of non-operator runs, like 123, true and "abc".
   * It main reside inside a string.
   **/
  simdjson_really_inline uint64_t potential_scalar_start() {
    // The term "scalar" refers to anything except structural characters and white space
    // (so letters, numbers, quotes).
    // Whenever it is preceded by something that is not a structural element ({,},[,],:, ") nor a white-space
    // then we know that it is irrelevant structurally.
    return _characters.scalar() & ~follows_potential_scalar();
  }
  /**
   * Whether the given character is immediately after a non-operator like 123, true.
   * The characters following a quote are not included.
   */
  simdjson_really_inline uint64_t follows_potential_scalar() {
    // _follows_potential_nonquote_scalar: is defined as marking any character that follows a character
    // that is not a structural element ({,},[,],:, comma) nor a quote (") and that is not a
    // white space.
    // It is understood that within quoted region, anything at all could be marked (irrelevant).
    return _follows_potential_nonquote_scalar;
  }
};

/**
 * Scans JSON for important bits: structural characters or 'operators', strings, and scalars.
 *
 * The scanner starts by calculating two distinct things:
 * - string characters (taking \" into account)
 * - structural characters or 'operators' ([]{},:, comma)
 *   and scalars (runs of non-operators like 123, true and "abc")
 *
 * To minimize data dependency (a key component of the scanner's speed), it finds these in parallel:
 * in particular, the operator/scalar bit will find plenty of things that are actually part of
 * strings. When we're done, json_block will fuse the two together by masking out tokens that are
 * part of a string.
 */
class json_scanner {
public:
  json_scanner() {}
  simdjson_really_inline json_block next(const simd::simd8x64<uint8_t>& in);
  simdjson_really_inline error_code finish(bool streaming);

private:
  // Whether the last character of the previous iteration is part of a scalar token
  // (anything except whitespace or a structural character/'operator').
  uint64_t prev_scalar = 0ULL;
  json_string_scanner string_scanner{};
};


//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
simdjson_really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

simdjson_really_inline json_block json_scanner::next(const simd::simd8x64<uint8_t>& in) {
  json_string_block strings = string_scanner.next(in);
  // identifies the white-space and the structurat characters
  json_character_block characters = json_character_block::classify(in);
  // The term "scalar" refers to anything except structural characters and white space
  // (so letters, numbers, quotes).
  // We want  follows_scalar to mark anything that follows a non-quote scalar (so letters and numbers).
  //
  // A terminal quote should either be followed by a structural character (comma, brace, bracket, colon)
  // or nothing. However, we still want ' "a string"true ' to mark the 't' of 'true' as a potential
  // pseudo-structural character just like we would if we had  ' "a string" true '; otherwise we
  // may need to add an extra check when parsing strings.
  //
  // Performance: there are many ways to skin this cat.
  const uint64_t nonquote_scalar = characters.scalar() & ~strings.quote();
  uint64_t follows_nonquote_scalar = follows(nonquote_scalar, prev_scalar);
  return {
    strings,
    characters,
    follows_nonquote_scalar
  };
}

simdjson_really_inline error_code json_scanner::finish(bool streaming) {
  return string_scanner.finish(streaming);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/json_scanner.h */
/* begin file src/generic/stage1/json_minifier.h */
// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class json_minifier {
public:
  template<size_t STEP_SIZE>
  static error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept;

private:
  simdjson_really_inline json_minifier(uint8_t *_dst)
  : dst{_dst}
  {}
  template<size_t STEP_SIZE>
  simdjson_really_inline void step(const uint8_t *block_buf, buf_block_reader<STEP_SIZE> &reader) noexcept;
  simdjson_really_inline void next(const simd::simd8x64<uint8_t>& in, json_block block);
  simdjson_really_inline error_code finish(uint8_t *dst_start, size_t &dst_len);
  json_scanner scanner{};
  uint8_t *dst;
};

simdjson_really_inline void json_minifier::next(const simd::simd8x64<uint8_t>& in, json_block block) {
  uint64_t mask = block.whitespace();
  in.compress(mask, dst);
  dst += 64 - count_ones(mask);
}

simdjson_really_inline error_code json_minifier::finish(uint8_t *dst_start, size_t &dst_len) {
  *dst = '\0';
  error_code error = scanner.finish(false);
  if (error) { dst_len = 0; return error; }
  dst_len = dst - dst_start;
  return SUCCESS;
}

template<>
simdjson_really_inline void json_minifier::step<128>(const uint8_t *block_buf, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  simd::simd8x64<uint8_t> in_2(block_buf+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1);
  this->next(in_2, block_2);
  reader.advance();
}

template<>
simdjson_really_inline void json_minifier::step<64>(const uint8_t *block_buf, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  json_block block_1 = scanner.next(in_1);
  this->next(block_buf, block_1);
  reader.advance();
}

template<size_t STEP_SIZE>
error_code json_minifier::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept {
  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_minifier minifier(dst);

  // Index the first n-1 blocks
  while (reader.has_full_block()) {
    minifier.step<STEP_SIZE>(reader.full_block(), reader);
  }

  // Index the last (remainder) block, padded with spaces
  uint8_t block[STEP_SIZE];
  if (simdjson_likely(reader.get_remainder(block)) > 0) {
    minifier.step<STEP_SIZE>(block, reader);
  }

  return minifier.finish(dst, dst_len);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/json_minifier.h */
/* begin file src/generic/stage1/find_next_document_index.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {

/**
  * This algorithm is used to quickly identify the last structural position that
  * makes up a complete document.
  *
  * It does this by going backwards and finding the last *document boundary* (a
  * place where one value follows another without a comma between them). If the
  * last document (the characters after the boundary) has an equal number of
  * start and end brackets, it is considered complete.
  *
  * Simply put, we iterate over the structural characters, starting from
  * the end. We consider that we found the end of a JSON document when the
  * first element of the pair is NOT one of these characters: '{' '[' ';' ','
  * and when the second element is NOT one of these characters: '}' '}' ';' ','.
  *
  * This simple comparison works most of the time, but it does not cover cases
  * where the batch's structural indexes contain a perfect amount of documents.
  * In such a case, we do not have access to the structural index which follows
  * the last document, therefore, we do not have access to the second element in
  * the pair, and that means we cannot identify the last document. To fix this
  * issue, we keep a count of the open and closed curly/square braces we found
  * while searching for the pair. When we find a pair AND the count of open and
  * closed curly/square braces is the same, we know that we just passed a
  * complete document, therefore the last json buffer location is the end of the
  * batch.
  */
simdjson_really_inline uint32_t find_next_document_index(dom_parser_implementation &parser) {
  // TODO don't count separately, just figure out depth
  auto arr_cnt = 0;
  auto obj_cnt = 0;
  for (auto i = parser.n_structural_indexes - 1; i > 0; i--) {
    auto idxb = parser.structural_indexes[i];
    switch (parser.buf[idxb]) {
    case ':':
    case ',':
      continue;
    case '}':
      obj_cnt--;
      continue;
    case ']':
      arr_cnt--;
      continue;
    case '{':
      obj_cnt++;
      break;
    case '[':
      arr_cnt++;
      break;
    }
    auto idxa = parser.structural_indexes[i - 1];
    switch (parser.buf[idxa]) {
    case '{':
    case '[':
    case ':':
    case ',':
      continue;
    }
    // Last document is complete, so the next document will appear after!
    if (!arr_cnt && !obj_cnt) {
      return parser.n_structural_indexes;
    }
    // Last document is incomplete; mark the document at i + 1 as the next one
    return i;
  }
  return 0;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/find_next_document_index.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class bit_indexer {
public:
  uint32_t *tail;

  simdjson_really_inline bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // base_ptr[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so base_ptr
  // needs to be large enough to handle this
  simdjson_really_inline void write(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
        return;
    int cnt = static_cast<int>(count_ones(bits));

    // Do the first 8 all together
    for (int i=0; i<8; i++) {
      this->tail[i] = idx + trailing_zeroes(bits);
      bits = clear_lowest_bit(bits);
    }

    // Do the next 8 all together (we hope in most cases it won't happen at all
    // and the branch is easily predicted).
    if (simdjson_unlikely(cnt > 8)) {
      for (int i=8; i<16; i++) {
        this->tail[i] = idx + trailing_zeroes(bits);
        bits = clear_lowest_bit(bits);
      }

      // Most files don't have 16+ structurals per block, so we take several basically guaranteed
      // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
      // or the start of a value ("abc" true 123) every four characters.
      if (simdjson_unlikely(cnt > 16)) {
        int i = 16;
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

class json_structural_indexer {
public:
  /**
   * Find the important bits of JSON in a 128-byte chunk, and add them to structural_indexes.
   *
   * @param partial Setting the partial parameter to true allows the find_structural_bits to
   *   tolerate unclosed strings. The caller should still ensure that the input is valid UTF-8. If
   *   you are processing substrings, you may want to call on a function like trimmed_length_safe_utf8.
   */
  template<size_t STEP_SIZE>
  static error_code index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, bool partial) noexcept;

private:
  simdjson_really_inline json_structural_indexer(uint32_t *structural_indexes);
  template<size_t STEP_SIZE>
  simdjson_really_inline void step(const uint8_t *block, buf_block_reader<STEP_SIZE> &reader) noexcept;
  simdjson_really_inline void next(const simd::simd8x64<uint8_t>& in, json_block block, size_t idx);
  simdjson_really_inline error_code finish(dom_parser_implementation &parser, size_t idx, size_t len, bool partial);

  json_scanner scanner{};
  utf8_checker checker{};
  bit_indexer indexer;
  uint64_t prev_structurals = 0;
  uint64_t unescaped_chars_error = 0;
};

simdjson_really_inline json_structural_indexer::json_structural_indexer(uint32_t *structural_indexes) : indexer{structural_indexes} {}

// Skip the last character if it is partial
simdjson_really_inline size_t trim_partial_utf8(const uint8_t *buf, size_t len) {
  if (simdjson_unlikely(len < 3)) {
    switch (len) {
      case 2:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 2 bytes left
        return len;
      case 1:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        return len;
      case 0:
        return len;
    }
  }
  if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
  if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 1 byte left
  if (buf[len-3] >= 0b11110000) { return len-3; } // 4-byte characters with only 3 bytes left
  return len;
}

//
// PERF NOTES:
// We pipe 2 inputs through these stages:
// 1. Load JSON into registers. This takes a long time and is highly parallelizable, so we load
//    2 inputs' worth at once so that by the time step 2 is looking for them input, it's available.
// 2. Scan the JSON for critical data: strings, scalars and operators. This is the critical path.
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
template<size_t STEP_SIZE>
error_code json_structural_indexer::index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, bool partial) noexcept {
  if (simdjson_unlikely(len > parser.capacity())) { return CAPACITY; }
  if (partial) { len = trim_partial_utf8(buf, len); }

  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_structural_indexer indexer(parser.structural_indexes.get());

  // Read all but the last block
  while (reader.has_full_block()) {
    indexer.step<STEP_SIZE>(reader.full_block(), reader);
  }

  // Take care of the last block (will always be there unless file is empty)
  uint8_t block[STEP_SIZE];
  if (simdjson_unlikely(reader.get_remainder(block) == 0)) { return EMPTY; }
  indexer.step<STEP_SIZE>(block, reader);

  return indexer.finish(parser, reader.block_index(), len, partial);
}

template<>
simdjson_really_inline void json_structural_indexer::step<128>(const uint8_t *block, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  simd::simd8x64<uint8_t> in_2(block+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1, reader.block_index());
  this->next(in_2, block_2, reader.block_index()+64);
  reader.advance();
}

template<>
simdjson_really_inline void json_structural_indexer::step<64>(const uint8_t *block, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  json_block block_1 = scanner.next(in_1);
  this->next(in_1, block_1, reader.block_index());
  reader.advance();
}

simdjson_really_inline void json_structural_indexer::next(const simd::simd8x64<uint8_t>& in, json_block block, size_t idx) {
  uint64_t unescaped = in.lteq(0x1F);
  checker.check_next_input(in);
  indexer.write(uint32_t(idx-64), prev_structurals); // Output *last* iteration's structurals to the parser
  prev_structurals = block.structural_start();
  unescaped_chars_error |= block.non_quote_inside_string(unescaped);
}

simdjson_really_inline error_code json_structural_indexer::finish(dom_parser_implementation &parser, size_t idx, size_t len, bool partial) {
  // Write out the final iteration's structurals
  indexer.write(uint32_t(idx-64), prev_structurals);

  error_code error = scanner.finish(partial);
  if (simdjson_unlikely(error != SUCCESS)) { return error; }

  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }

  parser.n_structural_indexes = uint32_t(indexer.tail - parser.structural_indexes.get());
  /***
   * This is related to https://github.com/simdjson/simdjson/issues/906
   * Basically, we want to make sure that if the parsing continues beyond the last (valid)
   * structural character, it quickly stops.
   * Only three structural characters can be repeated without triggering an error in JSON:  [,] and }.
   * We repeat the padding character (at 'len'). We don't know what it is, but if the parsing
   * continues, then it must be [,] or }.
   * Suppose it is ] or }. We backtrack to the first character, what could it be that would
   * not trigger an error? It could be ] or } but no, because you can't start a document that way.
   * It can't be a comma, a colon or any simple value. So the only way we could continue is
   * if the repeated character is [. But if so, the document must start with [. But if the document
   * starts with [, it should end with ]. If we enforce that rule, then we would get
   * ][[ which is invalid.
   **/
  parser.structural_indexes[parser.n_structural_indexes] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 1] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 2] = 0;
  parser.next_structural_index = 0;
  // a valid JSON file cannot have zero structural indexes - we should have found something
  if (simdjson_unlikely(parser.n_structural_indexes == 0u)) {
    return EMPTY;
  }
  if (simdjson_unlikely(parser.structural_indexes[parser.n_structural_indexes - 1] > len)) {
    return UNEXPECTED_ERROR;
  }
  if (partial) {
    auto new_structural_indexes = find_next_document_index(parser);
    if (new_structural_indexes == 0 && parser.n_structural_indexes > 0) {
      return CAPACITY; // If the buffer is partial but the document is incomplete, it's too big to parse.
    }
    parser.n_structural_indexes = new_structural_indexes;
  }
  return checker.errors();
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/find_next_document_index.h */
/* begin file src/generic/stage1/utf8_validator.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

/**
 * Validates that the string is actual UTF-8.
 */
template<class checker>
bool generic_validate_utf8(const uint8_t * input, size_t length) {
    checker c{};
    buf_block_reader<64> reader(input, length);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      c.check_next_input(in);
      reader.advance();
    }
    uint8_t block[64]{};
    reader.get_remainder(block);
    simd::simd8x64<uint8_t> in(block);
    c.check_next_input(in);
    reader.advance();
    return c.errors() == error_code::SUCCESS;
}

bool generic_validate_utf8(const char * input, size_t length) {
    return generic_validate_utf8<utf8_checker>((const uint8_t *)input,length);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage1/utf8_validator.h */

//
// Stage 2
//
/* begin file src/westmere/stringparsing.h */
#ifndef SIMDJSON_WESTMERE_STRINGPARSING_H
#define SIMDJSON_WESTMERE_STRINGPARSING_H

namespace {
namespace SIMDJSON_IMPLEMENTATION {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_really_inline bool has_backslash() { return bs_bits != 0; }
  simdjson_really_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_really_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + 16);
  v0.store(dst);
  v1.store(dst + 16);
  uint64_t bs_and_quote = simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
    uint32_t(bs_and_quote),      // bs_bits
    uint32_t(bs_and_quote >> 32) // quote_bits
  };
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/generic/stage2/stringparsing.h */
// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace stringparsing {

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
SIMDJSON_WARN_UNUSED
simdjson_really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
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

SIMDJSON_WARN_UNUSED simdjson_really_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst) {
  src++;
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return nullptr;
}

SIMDJSON_UNUSED SIMDJSON_WARN_UNUSED simdjson_really_inline error_code parse_string_to_buffer(const uint8_t *src, uint8_t *&current_string_buf_loc, std::string_view &s) {
  if (src[0] != '"') { return STRING_ERROR; }
  auto end = stringparsing::parse_string(src, current_string_buf_loc);
  if (!end) { return STRING_ERROR; }
  s = std::string_view((const char *)current_string_buf_loc, end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}

} // namespace stringparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/stringparsing.h */

#endif // SIMDJSON_WESTMERE_STRINGPARSING_H
/* end file src/generic/stage2/stringparsing.h */
/* begin file src/westmere/numberparsing.h */
#ifndef SIMDJSON_WESTMERE_NUMBERPARSING_H
#define SIMDJSON_WESTMERE_NUMBERPARSING_H

namespace {
namespace SIMDJSON_IMPLEMENTATION {

static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
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

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#define SWAR_NUMBER_PARSING

/* begin file src/generic/stage2/numberparsing.h */
#include <cmath>
#include <limits>

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace numberparsing {

#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [FASTFLOAT_SMALLEST_POWER,
// FASTFLOAT_LARGEST_POWER] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / power_of_ten[-power];
    } else {
      d = d * power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It wa
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }

  // We are going to need to do some 64-bit arithmetic to get a more precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= FASTFLOAT_SMALLEST_POWER
  // and power <= FASTFLOAT_LARGEST_POWER
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  uint64_t factor_mantissa = mantissa_64[power - FASTFLOAT_SMALLEST_POWER];
  
  // The exponent is 1024 + 63 + power 
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power
  //
  // The 65536 is (1<<16) and corresponds to 
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to 
  // floor(log(5**power)/log(2)) 
  //
  // Note that this is not magic: 152170/(1<<16) is 
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a 
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;
  

  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  value128 product = full_multiplication(i, factor_mantissa);
  uint64_t lower = product.low;
  uint64_t upper = product.high;

  // We know that upper has at most one leading zero because
  // both i and  factor_mantissa have a leading one. This means
  // that the result is at least as large as ((1<<63)*(1<<63))/(1<<64).

  // As long as the first 9 bits of "upper" are not "1", then we
  // know that we have an exact computed value for the leading
  // 55 bits because any imprecision would play out as a +1, in
  // the worst case.
  if (simdjson_unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
    uint64_t factor_mantissa_low =
        mantissa_128[power - FASTFLOAT_SMALLEST_POWER];
    // next, we compute the 64-bit x 128-bit multiplication, getting a 192-bit
    // result (three 64-bit values)
    product = full_multiplication(i, factor_mantissa_low);
    uint64_t product_low = product.low;
    uint64_t product_middle2 = product.high;
    uint64_t product_middle1 = lower;
    uint64_t product_high = upper;
    uint64_t product_middle = product_middle1 + product_middle2;
    if (product_middle < product_middle1) {
      product_high++; // overflow carry
    }
    // We want to check whether mantissa *i + i would affect our result.
    // This does happen, e.g. with 7.3177701707893310e+15.
    if (((product_middle + 1 == 0) && ((product_high & 0x1FF) == 0x1FF) &&
         (product_low + i < product_low))) { // let us be prudent and bail out.
      return false;
    }
    upper = product_high;
    lower = product_middle;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).

  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  if (simdjson_unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
               ((mantissa & 3) == 1))) {
    // if mantissa & 1 == 1 we might need to round up.
    //
    // Scenarios:
    // 1. We are not in the middle. Then we should round up.
    //
    // 2. We are right in the middle. Whether we round up depends
    // on the last significant bit: if it is "one" then we round
    // up (round to even) otherwise, we do not.
    //
    // So if the last significant bit is 1, we can safely round up.
    // Hence we only need to bail out if (mantissa & 3) == 1.
    // Otherwise we may need more accuracy or analysis to determine whether
    // we are exactly between two floating-point numbers.
    // It can be triggered with 1e23.
    // Note: because the factor_mantissa and factor_mantissa_low are
    // almost always rounded down (except for small positive powers),
    // almost always should round up.
    return false;
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    lz--; // undo previous addition
  }
  mantissa &= ~(1ULL << 52);
  uint64_t real_exponent = exponent - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    return false;
  }
  mantissa |= real_exponent << 52;
  mantissa |= (((uint64_t)negative) << 63);
  memcpy(&d, &mantissa, sizeof(d));
  return true;
}

static bool parse_float_strtod(const uint8_t *ptr, double *outDouble) {
  char *endptr;
  *outDouble = strtod((const char *)ptr, &endptr);
  // Some libraries will set errno = ERANGE when the value is subnormal,
  // yet we may want to be able to parse subnormal values.
  // However, we do not want to tolerate NAN or infinite values.
  //
  // Values like infinity or NaN are not allowed in the JSON specification.
  // If you consume a large value and you map it to "infinity", you will no
  // longer be able to serialize back a standard-compliant JSON. And there is
  // no realistic application where you might need values so large than they
  // can't fit in binary64. The maximal value is about  1.7976931348623157 x
  // 10^308 It is an unimaginable large number. There will never be any piece of
  // engineering involving as many as 10^308 parts. It is estimated that there
  // are about 10^80 atoms in the universe.  The estimate for the total number
  // of electrons is similar. Using a double-precision floating-point value, we
  // can represent easily the number of atoms in the universe. We could  also
  // represent the number of ways you can pick any three individual atoms at
  // random in the universe. If you ever encounter a number much larger than
  // 10^308, you know that you have a bug. RapidJSON will reject a document with
  // a float that does not fit in binary64. JSON for Modern C++ (nlohmann/json)
  // will flat out throw an exception.
  //
  if ((endptr == (const char *)ptr) || (!std::isfinite(*outDouble))) {
    return false;
  }
  return true;
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(SIMDJSON_UNUSED const uint8_t * src, W writer) {
  double d;
  if (parse_float_strtod(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow. 
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off 
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do 
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time. 
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in 
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline int significant_digits(const uint8_t * start_digits, int digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - int(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, int digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  // digit count is off by 1 because of the decimal (assuming there was one).
  if (simdjson_unlikely(digit_count-1 >= 19 && significant_digits(start_digits, digit_count) >= 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < FASTFLOAT_SMALLEST_POWER) || (exponent > FASTFLOAT_LARGEST_POWER)) {
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_strtod(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool clean_end = is_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (!clean_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// SAX functions
namespace {
// Parse any number from 0 to 18,446,744,073,709,551,615
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  if (digit_count > 20) { return NUMBER_ERROR; }
  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return i;
}

// Parse any number from 0 to 18,446,744,073,709,551,615
// Call this version of the method if you regularly expect 8- or 16-digit numbers.
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_large_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
      if (parse_digit(*p, i)) { // digit 17
        p++;
        if (parse_digit(*p, i)) { // digit 18
          p++;
          if (parse_digit(*p, i)) { // digit 19
            p++;
            if (parse_digit(*p, i)) { // digit 20
              p++;
              if (parse_digit(*p, i)) { return NUMBER_ERROR; } // 21 digits is an error
              // Positive overflow check:
              // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
              //   biggest uint64_t.
              // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
              //   If we got here, it's a 20 digit number starting with the digit "1".
              // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
              //   than 1,553,255,926,290,448,384.
              // - That is smaller than the smallest possible 20-digit number the user could write:
              //   10,000,000,000,000,000,000.
              // - Therefore, if the number is positive and lower than that, it's overflow.
              // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
              //
              if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
            }
          }
        }
      }
    } // 16 digits
  } else { // 8 digits
    // Less than 8 digits can't overflow, simpler logic here.
    if (parse_digit(*p, i)) { p++; } else { return NUMBER_ERROR; }
    while (parse_digit(*p, i)) { p++; }
  }

  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }
  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - src);
  if (digit_count == 0 || ('0' == *src && digit_count > 1)) { return NUMBER_ERROR; }
  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return NUMBER_ERROR; }
  if (digit_count == longest_digit_count) {
    if(negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return NUMBER_ERROR; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return negative ? (~i+1) : i;
}

SIMDJSON_UNUSED simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src || (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 >= 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src >= 19;
    }
  } else {
    overflow = p-src >= 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
    overflow = overflow || exponent < FASTFLOAT_SMALLEST_POWER || exponent > FASTFLOAT_LARGEST_POWER;
  }

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_strtod(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/numberparsing.h */

#endif //  SIMDJSON_WESTMERE_NUMBERPARSING_H
/* end file src/generic/stage2/numberparsing.h */
/* begin file src/generic/stage2/tape_builder.h */
/* begin file src/generic/stage2/json_iterator.h */
/* begin file src/generic/stage2/logger.h */
// This is for an internal-only stage 2 specific logger.
// Set LOG_ENABLED = true to log what stage 2 is doing!
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace logger {

  static constexpr const char * DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";

  static constexpr const bool LOG_ENABLED = false;
  static constexpr const int LOG_EVENT_LEN = 20;
  static constexpr const int LOG_BUFFER_LEN = 30;
  static constexpr const int LOG_SMALL_BUFFER_LEN = 10;
  static constexpr const int LOG_INDEX_LEN = 5;

  static int log_depth; // Not threadsafe. Log only.

  // Helper to turn unprintable or newline characters into spaces
  static simdjson_really_inline char printable_char(char c) {
    if (c >= 0x20) {
      return c;
    } else {
      return ' ';
    }
  }

  // Print the header and set up log_start
  static simdjson_really_inline void log_start() {
    if (LOG_ENABLED) {
      log_depth = 0;
      printf("\n");
      printf("| %-*s | %-*s | %-*s | %-*s | Detail |\n", LOG_EVENT_LEN, "Event", LOG_BUFFER_LEN, "Buffer", LOG_SMALL_BUFFER_LEN, "Next", 5, "Next#");
      printf("|%.*s|%.*s|%.*s|%.*s|--------|\n", LOG_EVENT_LEN+2, DASHES, LOG_BUFFER_LEN+2, DASHES, LOG_SMALL_BUFFER_LEN+2, DASHES, 5+2, DASHES);
    }
  }

  // Logs a single line of 
  template<typename S>
  static simdjson_really_inline void log_line(S &structurals, const char *title_prefix, const char *title, const char *detail) {
    if (LOG_ENABLED) {
      printf("| %*s%s%-*s ", log_depth*2, "", title_prefix, LOG_EVENT_LEN - log_depth*2 - int(strlen(title_prefix)), title);
      auto current_index = structurals.at_beginning() ? nullptr : structurals.next_structural-1;
      auto next_index = structurals.next_structural;
      auto current = current_index ? &structurals.buf[*current_index] : (const uint8_t*)"                                                       ";
      auto next = &structurals.buf[*next_index];
      {
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_BUFFER_LEN;i++) {
          printf("%c", printable_char(current[i]));
        }
        printf(" ");
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_SMALL_BUFFER_LEN;i++) {
          printf("%c", printable_char(next[i]));
        }
        printf(" ");
      }
      if (current_index) {
        printf("| %*u ", LOG_INDEX_LEN, *current_index);
      } else {
        printf("| %-*s ", LOG_INDEX_LEN, "");
      }
      // printf("| %*u ", LOG_INDEX_LEN, structurals.next_tape_index());
      printf("| %-s ", detail);
      printf("|\n");
    }
  }

} // namespace logger
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/logger.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

class json_iterator {
public:
  const uint8_t* const buf;
  uint32_t *next_structural;
  dom_parser_implementation &dom_parser;
  uint32_t depth{0};

  /**
   * Walk the JSON document.
   *
   * The visitor receives callbacks when values are encountered. All callbacks pass the iterator as
   * the first parameter; some callbacks have other parameters as well:
   *
   * - visit_document_start() - at the beginning.
   * - visit_document_end() - at the end (if things were successful).
   *
   * - visit_array_start() - at the start `[` of a non-empty array.
   * - visit_array_end() - at the end `]` of a non-empty array.
   * - visit_empty_array() - when an empty array is encountered.
   *
   * - visit_object_end() - at the start `]` of a non-empty object.
   * - visit_object_start() - at the end `]` of a non-empty object.
   * - visit_empty_object() - when an empty object is encountered.
   * - visit_key(const uint8_t *key) - when a key in an object field is encountered. key is
   *                                   guaranteed to point at the first quote of the string (`"key"`).
   * - visit_primitive(const uint8_t *value) - when a value is a string, number, boolean or null.
   * - visit_root_primitive(iter, uint8_t *value) - when the top-level value is a string, number, boolean or null.
   *
   * - increment_count(iter) - each time a value is found in an array or object.
   */
  template<bool STREAMING, typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code walk_document(V &visitor) noexcept;

  /**
   * Create an iterator capable of walking a JSON document.
   *
   * The document must have already passed through stage 1.
   */
  simdjson_really_inline json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index);

  /**
   * Look at the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *peek() const noexcept;
  /**
   * Advance to the next token.
   *
   * Tokens can be strings, numbers, booleans, null, or operators (`[{]},:`)).
   *
   * They may include invalid JSON as well (such as `1.2.3` or `ture`).
   */
  simdjson_really_inline const uint8_t *advance() noexcept;
  /**
   * Get the remaining length of the document, from the start of the current token.
   */
  simdjson_really_inline size_t remaining_len() const noexcept;
  /**
   * Check if we are at the end of the document.
   *
   * If this is true, there are no more tokens.
   */
  simdjson_really_inline bool at_eof() const noexcept;
  /**
   * Check if we are at the beginning of the document.
   */
  simdjson_really_inline bool at_beginning() const noexcept;
  simdjson_really_inline uint8_t last_structural() const noexcept;

  /**
   * Log that a value has been found.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_value(const char *type) const noexcept;
  /**
   * Log the start of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_start_value(const char *type) const noexcept;
  /**
   * Log the end of a multipart value.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_end_value(const char *type) const noexcept;
  /**
   * Log an error.
   *
   * Set ENABLE_LOGGING=true in logger.h to see logging.
   */
  simdjson_really_inline void log_error(const char *error) const noexcept;

  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_primitive(V &visitor, const uint8_t *value) noexcept;
  template<typename V>
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_primitive(V &visitor, const uint8_t *value) noexcept;
};

template<bool STREAMING, typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::walk_document(V &visitor) noexcept {
  logger::log_start();

  //
  // Start the document
  //
  if (at_eof()) { return EMPTY; }
  log_start_value("document");
  SIMDJSON_TRY( visitor.visit_document_start(*this) );

  //
  // Read first value
  //
  {
    auto value = advance();

    // Make sure the outer hash or array is closed before continuing; otherwise, there are ways we
    // could get into memory corruption. See https://github.com/simdjson/simdjson/issues/906
    if (!STREAMING) {
      switch (*value) {
        case '{': if (last_structural() != '}') { return TAPE_ERROR; }; break;
        case '[': if (last_structural() != ']') { return TAPE_ERROR; }; break;
      }
    }

    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_root_primitive(*this, value) ); break;
    }
  }
  goto document_end;

//
// Object parser states
//
object_begin:
  log_start_value("object");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = false;
  SIMDJSON_TRY( visitor.visit_object_start(*this) );

  {
    auto key = advance();
    if (*key != '"') { log_error("Object does not start with a key"); return TAPE_ERROR; }
    SIMDJSON_TRY( visitor.increment_count(*this) );
    SIMDJSON_TRY( visitor.visit_key(*this, key) );
  }

object_field:
  if (simdjson_unlikely( *advance() != ':' )) { log_error("Missing colon after key in object"); return TAPE_ERROR; }
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

object_continue:
  switch (*advance()) {
    case ',':
      SIMDJSON_TRY( visitor.increment_count(*this) );
      {
        auto key = advance();
        if (simdjson_unlikely( *key != '"' )) { log_error("Key string missing at beginning of field in object"); return TAPE_ERROR; }
        SIMDJSON_TRY( visitor.visit_key(*this, key) );
      }
      goto object_field;
    case '}': log_end_value("object"); SIMDJSON_TRY( visitor.visit_object_end(*this) ); goto scope_end;
    default: log_error("No comma between object fields"); return TAPE_ERROR;
  }

scope_end:
  depth--;
  if (depth == 0) { goto document_end; }
  if (dom_parser.is_array[depth]) { goto array_continue; }
  goto object_continue;

//
// Array parser states
//
array_begin:
  log_start_value("array");
  depth++;
  if (depth >= dom_parser.max_depth()) { log_error("Exceeded max depth!"); return DEPTH_ERROR; }
  dom_parser.is_array[depth] = true;
  SIMDJSON_TRY( visitor.visit_array_start(*this) );
  SIMDJSON_TRY( visitor.increment_count(*this) );

array_value:
  {
    auto value = advance();
    switch (*value) {
      case '{': if (*peek() == '}') { advance(); log_value("empty object"); SIMDJSON_TRY( visitor.visit_empty_object(*this) ); break; } goto object_begin;
      case '[': if (*peek() == ']') { advance(); log_value("empty array"); SIMDJSON_TRY( visitor.visit_empty_array(*this) ); break; } goto array_begin;
      default: SIMDJSON_TRY( visitor.visit_primitive(*this, value) ); break;
    }
  }

array_continue:
  switch (*advance()) {
    case ',': SIMDJSON_TRY( visitor.increment_count(*this) ); goto array_value;
    case ']': log_end_value("array"); SIMDJSON_TRY( visitor.visit_array_end(*this) ); goto scope_end;
    default: log_error("Missing comma between array values"); return TAPE_ERROR;
  }

document_end:
  log_end_value("document");
  SIMDJSON_TRY( visitor.visit_document_end(*this) );

  dom_parser.next_structural_index = uint32_t(next_structural - &dom_parser.structural_indexes[0]);

  // If we didn't make it to the end, it's an error
  if ( !STREAMING && dom_parser.next_structural_index != dom_parser.n_structural_indexes ) {
    log_error("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
    return TAPE_ERROR;
  }

  return SUCCESS;

} // walk_document()

simdjson_really_inline json_iterator::json_iterator(dom_parser_implementation &_dom_parser, size_t start_structural_index)
  : buf{_dom_parser.buf},
    next_structural{&_dom_parser.structural_indexes[start_structural_index]},
    dom_parser{_dom_parser} {
}

simdjson_really_inline const uint8_t *json_iterator::peek() const noexcept {
  return &buf[*(next_structural)];
}
simdjson_really_inline const uint8_t *json_iterator::advance() noexcept {
  return &buf[*(next_structural++)];
}
simdjson_really_inline size_t json_iterator::remaining_len() const noexcept {
  return dom_parser.len - *(next_structural-1);
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return next_structural == &dom_parser.structural_indexes[dom_parser.n_structural_indexes];
}
simdjson_really_inline bool json_iterator::at_beginning() const noexcept {
  return next_structural == dom_parser.structural_indexes.get();
}
simdjson_really_inline uint8_t json_iterator::last_structural() const noexcept {
  return buf[dom_parser.structural_indexes[dom_parser.n_structural_indexes - 1]];
}

simdjson_really_inline void json_iterator::log_value(const char *type) const noexcept {
  logger::log_line(*this, "", type, "");
}

simdjson_really_inline void json_iterator::log_start_value(const char *type) const noexcept {
  logger::log_line(*this, "+", type, "");
  if (logger::LOG_ENABLED) { logger::log_depth++; }
}

simdjson_really_inline void json_iterator::log_end_value(const char *type) const noexcept {
  if (logger::LOG_ENABLED) { logger::log_depth--; }
  logger::log_line(*this, "-", type, "");
}

simdjson_really_inline void json_iterator::log_error(const char *error) const noexcept {
  logger::log_line(*this, "", "ERROR", error);
}

template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_root_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_root_string(*this, value);
    case 't': return visitor.visit_root_true_atom(*this, value);
    case 'f': return visitor.visit_root_false_atom(*this, value);
    case 'n': return visitor.visit_root_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_root_number(*this, value);
    default:
      log_error("Document starts with a non-value character");
      return TAPE_ERROR;
  }
}
template<typename V>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::visit_primitive(V &visitor, const uint8_t *value) noexcept {
  switch (*value) {
    case '"': return visitor.visit_string(*this, value);
    case 't': return visitor.visit_true_atom(*this, value);
    case 'f': return visitor.visit_false_atom(*this, value);
    case 'n': return visitor.visit_null_atom(*this, value);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return visitor.visit_number(*this, value);
    default:
      log_error("Non-value found when value was expected!");
      return TAPE_ERROR;
  }
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/logger.h */
/* begin file src/generic/stage2/tape_writer.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_writer {
  /** The next place to write to tape */
  uint64_t *next_tape_loc;
  
  /** Write a signed 64-bit value to tape. */
  simdjson_really_inline void append_s64(int64_t value) noexcept;

  /** Write an unsigned 64-bit value to tape. */
  simdjson_really_inline void append_u64(uint64_t value) noexcept;

  /** Write a double value to tape. */
  simdjson_really_inline void append_double(double value) noexcept;

  /**
   * Append a tape entry (an 8-bit type,and 56 bits worth of value).
   */
  simdjson_really_inline void append(uint64_t val, internal::tape_type t) noexcept;

  /**
   * Skip the current tape entry without writing.
   *
   * Used to skip the start of the container, since we'll come back later to fill it in when the
   * container ends.
   */
  simdjson_really_inline void skip() noexcept;

  /**
   * Skip the number of tape entries necessary to write a large u64 or i64.
   */
  simdjson_really_inline void skip_large_integer() noexcept;

  /**
   * Skip the number of tape entries necessary to write a double.
   */
  simdjson_really_inline void skip_double() noexcept;

  /**
   * Write a value to a known location on tape.
   *
   * Used to go back and write out the start of a container after the container ends.
   */
  simdjson_really_inline static void write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept;

private:
  /**
   * Append both the tape entry, and a supplementary value following it. Used for types that need
   * all 64 bits, such as double and uint64_t.
   */
  template<typename T>
  simdjson_really_inline void append2(uint64_t val, T val2, internal::tape_type t) noexcept;
}; // struct number_writer

simdjson_really_inline void tape_writer::append_s64(int64_t value) noexcept {
  append2(0, value, internal::tape_type::INT64);
}

simdjson_really_inline void tape_writer::append_u64(uint64_t value) noexcept {
  append(0, internal::tape_type::UINT64);
  *next_tape_loc = value;
  next_tape_loc++;
}

/** Write a double value to tape. */
simdjson_really_inline void tape_writer::append_double(double value) noexcept {
  append2(0, value, internal::tape_type::DOUBLE);
}

simdjson_really_inline void tape_writer::skip() noexcept {
  next_tape_loc++;
}

simdjson_really_inline void tape_writer::skip_large_integer() noexcept {
  next_tape_loc += 2;
}

simdjson_really_inline void tape_writer::skip_double() noexcept {
  next_tape_loc += 2;
}

simdjson_really_inline void tape_writer::append(uint64_t val, internal::tape_type t) noexcept {
  *next_tape_loc = val | ((uint64_t(char(t))) << 56);
  next_tape_loc++;
}

template<typename T>
simdjson_really_inline void tape_writer::append2(uint64_t val, T val2, internal::tape_type t) noexcept {
  append(val, t);
  static_assert(sizeof(val2) == sizeof(*next_tape_loc), "Type is not 64 bits!");
  memcpy(next_tape_loc, &val2, sizeof(val2));
  next_tape_loc++;
}

simdjson_really_inline void tape_writer::write(uint64_t &tape_loc, uint64_t val, internal::tape_type t) noexcept {
  tape_loc = val | ((uint64_t(char(t))) << 56);
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/tape_writer.h */
/* begin file src/generic/stage2/atomparsing.h */
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_really_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
SIMDJSON_WARN_UNUSED
simdjson_really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | is_not_structural_or_whitespace(src[4])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | is_not_structural_or_whitespace(src[5])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | is_not_structural_or_whitespace(src[4])) == 0;
}

SIMDJSON_WARN_UNUSED
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/atomparsing.h */

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {

struct tape_builder {
  template<bool STREAMING>
  SIMDJSON_WARN_UNUSED static simdjson_really_inline error_code parse_document(
    dom_parser_implementation &dom_parser,
    dom::document &doc) noexcept;

  /** Called when a non-empty document starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_document_start(json_iterator &iter) noexcept;
  /** Called when a non-empty document ends without error. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_document_end(json_iterator &iter) noexcept;

  /** Called when a non-empty array starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_array_start(json_iterator &iter) noexcept;
  /** Called when a non-empty array ends. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_array_end(json_iterator &iter) noexcept;
  /** Called when an empty array is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_empty_array(json_iterator &iter) noexcept;

  /** Called when a non-empty object starts. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_object_start(json_iterator &iter) noexcept;
  /**
   * Called when a key in a field is encountered.
   *
   * primitive, visit_object_start, visit_empty_object, visit_array_start, or visit_empty_array
   * will be called after this with the field value.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_key(json_iterator &iter, const uint8_t *key) noexcept;
  /** Called when a non-empty object ends. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_object_end(json_iterator &iter) noexcept;
  /** Called when an empty object is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_empty_object(json_iterator &iter) noexcept;

  /**
   * Called when a string, number, boolean or null is found.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_primitive(json_iterator &iter, const uint8_t *value) noexcept;
  /**
   * Called when a string, number, boolean or null is found at the top level of a document (i.e.
   * when there is no array or object and the entire document is a single string, number, boolean or
   * null.
   *
   * This is separate from primitive() because simdjson's normal primitive parsing routines assume
   * there is at least one more token after the value, which is only true in an array or object.
   */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_string(json_iterator &iter, const uint8_t *value, bool key = false) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_number(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_string(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_number(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

  /** Called each time a new field or element in an array or object is found. */
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code increment_count(json_iterator &iter) noexcept;

  /** Next location to write to tape */
  tape_writer tape;
private:
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc;

  simdjson_really_inline tape_builder(dom::document &doc) noexcept;

  simdjson_really_inline uint32_t next_tape_index(json_iterator &iter) const noexcept;
  simdjson_really_inline void start_container(json_iterator &iter) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_really_inline error_code empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  simdjson_really_inline uint8_t *on_start_string(json_iterator &iter) noexcept;
  simdjson_really_inline void on_end_string(uint8_t *dst) noexcept;
}; // class tape_builder

template<bool STREAMING>
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::parse_document(
    dom_parser_implementation &dom_parser,
    dom::document &doc) noexcept {
  dom_parser.doc = &doc;
  json_iterator iter(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
  tape_builder builder(doc);
  return iter.walk_document<STREAMING>(builder);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_root_primitive(*this, value);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_primitive(*this, value);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_empty_object(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_empty_array(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_document_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_object_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_array_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_object_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_array_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_document_end(json_iterator &iter) noexcept {
  constexpr uint32_t start_tape_index = 0;
  tape.append(start_tape_index, internal::tape_type::ROOT);
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter), internal::tape_type::ROOT);
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_key(json_iterator &iter, const uint8_t *key) noexcept {
  return visit_string(iter, key, true);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::increment_count(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].count++; // we have a key value pair in the object at parser.dom_parser.depth - 1
  return SUCCESS;
}

simdjson_really_inline tape_builder::tape_builder(dom::document &doc) noexcept : tape{doc.tape.get()}, current_string_buf_loc{doc.string_buf.get()} {}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_string(json_iterator &iter, const uint8_t *value, bool key) noexcept {
  iter.log_value(key ? "key" : "string");
  uint8_t *dst = on_start_string(iter);
  dst = stringparsing::parse_string(value, dst);
  if (dst == nullptr) {
    iter.log_error("Invalid escape in string");
    return STRING_ERROR;
  }
  on_end_string(dst);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_string(json_iterator &iter, const uint8_t *value) noexcept {
  return visit_string(iter, value);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_number(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("number");
  return numberparsing::parse_number(value, tape);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_number(json_iterator &iter, const uint8_t *value) noexcept {
  //
  // We need to make a copy to make sure that the string is space terminated.
  // This is not about padding the input, which should already padded up
  // to len + SIMDJSON_PADDING. However, we have no control at this stage
  // on how the padding was done. What if the input string was padded with nulls?
  // It is quite common for an input string to have an extra null character (C string).
  // We do not want to allow 9\0 (where \0 is the null character) inside a JSON
  // document, but the string "9\0" by itself is fine. So we make a copy and
  // pad the input with spaces when we know that there is just one input element.
  // This copy is relatively expensive, but it will almost never be called in
  // practice unless you are in the strange scenario where you have many JSON
  // documents made of single atoms.
  //
  uint8_t *copy = static_cast<uint8_t *>(malloc(iter.remaining_len() + SIMDJSON_PADDING));
  if (copy == nullptr) { return MEMALLOC; }
  memcpy(copy, value, iter.remaining_len());
  memset(copy + iter.remaining_len(), ' ', SIMDJSON_PADDING);
  error_code error = visit_number(iter, copy);
  free(copy);
  return error;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  if (!atomparsing::is_valid_true_atom(value)) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  if (!atomparsing::is_valid_true_atom(value, iter.remaining_len())) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  if (!atomparsing::is_valid_false_atom(value)) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  if (!atomparsing::is_valid_false_atom(value, iter.remaining_len())) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  if (!atomparsing::is_valid_null_atom(value)) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  if (!atomparsing::is_valid_null_atom(value, iter.remaining_len())) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

// private:

simdjson_really_inline uint32_t tape_builder::next_tape_index(json_iterator &iter) const noexcept {
  return uint32_t(tape.next_tape_loc - iter.dom_parser.doc->tape.get());
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  auto start_index = next_tape_index(iter);
  tape.append(start_index+2, start);
  tape.append(start_index, end);
  return SUCCESS;
}

simdjson_really_inline void tape_builder::start_container(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].tape_index = next_tape_index(iter);
  iter.dom_parser.open_containers[iter.depth].count = 0;
  tape.skip(); // We don't actually *write* the start element until the end.
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code tape_builder::end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  // Write the ending tape element, pointing at the start location
  const uint32_t start_tape_index = iter.dom_parser.open_containers[iter.depth].tape_index;
  tape.append(start_tape_index, end);
  // Write the start tape element, pointing at the end location (and including count)
  // count can overflow if it exceeds 24 bits... so we saturate
  // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
  const uint32_t count = iter.dom_parser.open_containers[iter.depth].count;
  const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter) | (uint64_t(cntsat) << 32), start);
  return SUCCESS;
}

simdjson_really_inline uint8_t *tape_builder::on_start_string(json_iterator &iter) noexcept {
  // we advance the point, accounting for the fact that we have a NULL termination
  tape.append(current_string_buf_loc - iter.dom_parser.doc->string_buf.get(), internal::tape_type::STRING);
  return current_string_buf_loc + sizeof(uint32_t);
}

simdjson_really_inline void tape_builder::on_end_string(uint8_t *dst) noexcept {
  uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
  // TODO check for overflow in case someone has a crazy string (>=4GB?)
  // But only add the overflow check when the document itself exceeds 4GB
  // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
  memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
  // NULL termination is still handy if you expect all your strings to
  // be NULL terminated? It comes at a small cost
  *dst = 0;
  current_string_buf_loc = dst + 1;
}

} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
/* end file src/generic/stage2/atomparsing.h */

//
// Implementation-specific overrides
//

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

simdjson_really_inline uint64_t json_string_scanner::find_escaped(uint64_t backslash) {
  if (!backslash) { uint64_t escaped = prev_escaped; prev_escaped = 0; return escaped; }
  return find_escaped_branchless(backslash);
}

} // namespace stage1

SIMDJSON_WARN_UNUSED error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return westmere::stage1::json_minifier::minify<64>(buf, len, dst, dst_len);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, bool streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return westmere::stage1::json_structural_indexer::index<64>(_buf, _len, *this, streaming);
}

SIMDJSON_WARN_UNUSED bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return westmere::stage1::generic_validate_utf8(buf,len);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, false);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

/* begin file src/westmere/end_implementation.h */
#undef SIMDJSON_IMPLEMENTATION
SIMDJSON_UNTARGET_REGION
/* end file src/westmere/end_implementation.h */
/* end file src/westmere/end_implementation.h */
#endif

SIMDJSON_POP_DISABLE_WARNINGS
/* end file src/westmere/end_implementation.h */
