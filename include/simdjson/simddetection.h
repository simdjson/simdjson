/* From https://github.com/endorno/pytorch/blob/master/torch/lib/TH/generic/simd/simd.h
Slightly modified.

Copyright (c) 2016-     Facebook, Inc            (Adam Paszke)
Copyright (c) 2014-     Facebook, Inc            (Soumith Chintala)
Copyright (c) 2011-2014 Idiap Research Institute (Ronan Collobert)
Copyright (c) 2012-2014 Deepmind Technologies    (Koray Kavukcuoglu)
Copyright (c) 2011-2012 NEC Laboratories America (Koray Kavukcuoglu)
Copyright (c) 2011-2013 NYU                      (Clement Farabet)
Copyright (c) 2006-2010 NEC Laboratories America (Ronan Collobert, Leon Bottou, Iain Melvin, Jason Weston)
Copyright (c) 2006      Idiap Research Institute (Samy Bengio)
Copyright (c) 2001-2004 Idiap Research Institute (Ronan Collobert, Samy Bengio, Johnny Mariethoz)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the names of Facebook, Deepmind Technologies, NYU, NEC Laboratories America
   and IDIAP Research Institute nor the names of its contributors may be
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

#ifndef TH_SIMD_INC
#define TH_SIMD_INC

#include <stdint.h>
#include <stdlib.h>
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)
#include <cpuid.h>
#endif

// Can be found on Intel ISA Reference for CPUID
#define CPUID_AVX2_BIT 0x20       // Bit 5 of EBX for EAX=0x7
#define CPUID_AVX_BIT  0x10000000 // Bit 28 of ECX for EAX=0x1
#define CPUID_SSE_BIT  0x2000000  // bit 25 of EDX for EAX=0x1
#define CPUID_SSE42_BIT 1 << 20   // bit 20 of EcX for EAX=0x1

// Helper macros for initialization
#define FUNCTION_IMPL(NAME, EXT) \
    { .function=(void *)NAME,    \
      .supportedSimdExt=EXT      \
    }

#define INIT_DISPATCH_PTR(OP)    \
  do {                           \
    int i;                       \
    for (i = 0; i < sizeof(THVector_(OP ## _DISPATCHTABLE)) / sizeof(FunctionDescription); ++i) { \
      THVector_(OP ## _DISPATCHPTR) = THVector_(OP ## _DISPATCHTABLE)[i].function;                     \
      if (THVector_(OP ## _DISPATCHTABLE)[i].supportedSimdExt & hostSimdExts) {                       \
        break;                                                                                     \
      }                                                                                            \
    }                                                                                              \
  } while(0)


typedef struct FunctionDescription
{
  void *function;
  uint32_t supportedSimdExt;
} FunctionDescription;


enum SIMDExtensions {
  DEFAULT   = 0x0,
  NEON      = 0x1,
  VSX       = 0x2,
  AVX2      = 0x4,
  AVX       = 0x8,
  SSE       = 0x10,
  SSE42     = 0x20,
  PCLMULQDQ = 0x40
};


#if defined(__arm__) || defined(__aarch64__) // incl. armel, armhf, arm64

 #if defined(__NEON__)

static inline uint32_t detectHostSIMDExtensions()
{
  return SIMDExtensions::NEON;
}

 #else //ARM without NEON

static inline uint32_t detectHostSIMDExtensions()
{
  return SIMDExtensions::DEFAULT;
}

 #endif

#elif defined(__PPC64__)

 #if defined(__VSX__)

static inline uint32_t detectHostSIMDExtensions()
{
  return SIMDExtensions::VSX;
}

 #else //PPC64 without VSX

static inline uint32_t detectHostSIMDExtensions()
{
  return SIMDExtensions::DEFAULT;
}

 #endif
  
#else   // x86
static inline void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
#if defined(_MSC_VER)
  uint32_t cpuInfo[4];
  __cpuid(cpuInfo, *eax);
  *eax = cpuInfo[0];
  *ebx = cpuInfo[1];
  *ecx = cpuInfo[2];
  *edx = cpuInfo[3];
#elif defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)
  uint32_t level = *eax;
  __get_cpuid (level, eax, ebx, ecx, edx);
#else
  uint32_t a = *eax, b, c = *ecx, d;
  asm volatile ( "cpuid\n\t"
		 : "+a"(a), "=b"(b), "+c"(c), "=d"(d) );
  *eax = a;
  *ebx = b;
  *ecx = c;
  *edx = d;
#endif
}

static inline uint32_t detectHostSIMDExtensions()
{
  uint32_t eax, ebx, ecx, edx;
  uint32_t hostSimdExts = 0x0;
  int TH_NO_AVX = 1, TH_NO_AVX2 = 1, TH_NO_SSE = 1, TH_NO_SSE42 = 1;
  char *evar;

  evar = getenv("TH_NO_AVX2");
  if (evar == NULL || strncmp(evar, "1", 2) != 0)
    TH_NO_AVX2 = 0;

  // Check for AVX2. Requires separate CPUID
  eax = 0x7;
  ecx = 0x0;
  cpuid(&eax, &ebx, &ecx, &edx);
  if ((ebx & CPUID_AVX2_BIT) && TH_NO_AVX2 == 0) {
    hostSimdExts |= SIMDExtensions::AVX2;
  }

  // Detect and enable AVX and SSE
  eax = 0x1;
  cpuid(&eax, &ebx, &ecx, &edx);
  evar = getenv("TH_NO_AVX");
  if (evar == NULL || strncmp(evar, "1", 2) != 0)
    TH_NO_AVX = 0;
  if (ecx & CPUID_AVX_BIT && TH_NO_AVX == 0) {
    hostSimdExts |= SIMDExtensions::AVX;
  }

  evar = getenv("TH_NO_SSE");
  if (evar == NULL || strncmp(evar, "1", 2) != 0)
    TH_NO_SSE = 0;  
  if (edx & CPUID_SSE_BIT && TH_NO_SSE == 0) {
    hostSimdExts |= SIMDExtensions::SSE;
  }

  evar = getenv("TH_NO_SSE42");
  if (evar == NULL || strncmp(evar, "1", 2) != 0)
    TH_NO_SSE42 = 0;  
  if (ecx & CPUID_SSE42_BIT && TH_NO_SSE42 == 0) {
    hostSimdExts |= SIMDExtensions::SSE42;
  }

  return hostSimdExts;
}

#endif // end SIMD extension detection code

#endif
