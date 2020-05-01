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
