#ifndef SIMDJSON_IMPLEMENTATIONS_H
#define SIMDJSON_IMPLEMENTATIONS_H

#include "simdjson/implementation-base.h"

//
// First, figure out which implementations can be run. Doing it here makes it so we don't have to worry about the order
// in which we include them.
//

#ifndef SIMDJSON_IMPLEMENTATION_ARM64
#define SIMDJSON_IMPLEMENTATION_ARM64 (SIMDJSON_IS_ARM64)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_ARM64 SIMDJSON_IMPLEMENTATION_ARM64 && SIMDJSON_IS_ARM64

#ifdef __has_include
// How do we detect that a compiler supports vbmi2?
// For sure if the following header is found, we are ok?
#if __has_include(<avx512vbmi2intrin.h>)
#define SIMDJSON_COMPILER_SUPPORTS_VBMI2 1
#endif
#endif

#ifdef _MSC_VER
#if _MSC_VER >= 1920
// Visual Studio 2019 and up support VBMI2 under x64 even if the header
// avx512vbmi2intrin.h is not found.
#define SIMDJSON_COMPILER_SUPPORTS_VBMI2 1
#endif
#endif

// By default, we allow AVX512.
#ifndef SIMDJSON_AVX512_ALLOWED
#define SIMDJSON_AVX512_ALLOWED 1
#endif

// Default Icelake to on if this is x86-64. Even if we're not compiled for it, it could be selected
// at runtime.
#ifndef SIMDJSON_IMPLEMENTATION_ICELAKE
#define SIMDJSON_IMPLEMENTATION_ICELAKE ((SIMDJSON_IS_X86_64) && (SIMDJSON_AVX512_ALLOWED) && (SIMDJSON_COMPILER_SUPPORTS_VBMI2))
#endif

#ifdef _MSC_VER
// To see why  (__BMI__) && (__PCLMUL__) && (__LZCNT__) are not part of this next line, see
// https://github.com/simdjson/simdjson/issues/1247
#define SIMDJSON_CAN_ALWAYS_RUN_ICELAKE ((SIMDJSON_IMPLEMENTATION_ICELAKE) && (__AVX2__) && (__AVX512F__) && (__AVX512DQ__) && (__AVX512CD__) && (__AVX512BW__) && (__AVX512VL__) && (__AVX512VBMI2__))
#else
#define SIMDJSON_CAN_ALWAYS_RUN_ICELAKE ((SIMDJSON_IMPLEMENTATION_ICELAKE) && (__AVX2__) && (__BMI__) && (__PCLMUL__) && (__LZCNT__) && (__AVX512F__) && (__AVX512DQ__) && (__AVX512CD__) && (__AVX512BW__) && (__AVX512VL__) && (__AVX512VBMI2__))
#endif

// Default Haswell to on if this is x86-64. Even if we're not compiled for it, it could be selected
// at runtime.
#ifndef SIMDJSON_IMPLEMENTATION_HASWELL
#define SIMDJSON_IMPLEMENTATION_HASWELL SIMDJSON_IS_X86_64
#endif
#ifdef _MSC_VER
// To see why  (__BMI__) && (__PCLMUL__) && (__LZCNT__) are not part of this next line, see
// https://github.com/simdjson/simdjson/issues/1247
#define SIMDJSON_CAN_ALWAYS_RUN_HASWELL ((SIMDJSON_IMPLEMENTATION_HASWELL) && (SIMDJSON_IS_X86_64) && (__AVX2__))
#else
#define SIMDJSON_CAN_ALWAYS_RUN_HASWELL ((SIMDJSON_IMPLEMENTATION_HASWELL) && (SIMDJSON_IS_X86_64) && (__AVX2__) && (__BMI__) && (__PCLMUL__) && (__LZCNT__))
#endif

// Default Westmere to on if this is x86-64. Note that the macro SIMDJSON_REQUIRES_HASWELL appears unused.
#ifndef SIMDJSON_IMPLEMENTATION_WESTMERE
#define SIMDJSON_IMPLEMENTATION_WESTMERE (SIMDJSON_IS_X86_64 && !SIMDJSON_REQUIRES_HASWELL)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_WESTMERE (SIMDJSON_IMPLEMENTATION_WESTMERE && SIMDJSON_IS_X86_64 && __SSE4_2__ && __PCLMUL__)

#ifndef SIMDJSON_IMPLEMENTATION_PPC64
#define SIMDJSON_IMPLEMENTATION_PPC64 (SIMDJSON_IS_PPC64)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_PPC64 SIMDJSON_IMPLEMENTATION_PPC64 && SIMDJSON_IS_PPC64

// Default Fallback to on unless a builtin implementation has already been selected.
#ifndef SIMDJSON_IMPLEMENTATION_FALLBACK
#define SIMDJSON_IMPLEMENTATION_FALLBACK 1 // (!SIMDJSON_CAN_ALWAYS_RUN_ARM64 && !SIMDJSON_CAN_ALWAYS_RUN_HASWELL && !SIMDJSON_CAN_ALWAYS_RUN_WESTMERE && !SIMDJSON_CAN_ALWAYS_RUN_PPC64)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_FALLBACK SIMDJSON_IMPLEMENTATION_FALLBACK

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

// Implementations
#include "simdjson/arm64.h"
#include "simdjson/fallback.h"
#include "simdjson/icelake.h"
#include "simdjson/haswell.h"
#include "simdjson/ppc64.h"
#include "simdjson/westmere.h"

// Builtin implementation
#include "simdjson/builtin.h"

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_IMPLEMENTATIONS_H