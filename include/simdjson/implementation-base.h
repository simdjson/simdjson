#ifndef SIMDJSON_IMPLEMENTATION_BASE_H
#define SIMDJSON_IMPLEMENTATION_BASE_H

/**
 * @file
 * @private
 *
 * Includes common stuff needed for implementations.
 */

#include "simdjson/base.h"
#include "simdjson/implementation.h"

// Implementation-internal files (must be included before the implementations themselves, to keep
// amalgamation working--otherwise, the first time a file is included, it might be put inside the
// #ifdef SIMDJSON_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other implementations can't
// compile unless that implementation is turned on).
#include "simdjson/internal/isadetection.h"
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"
#include "simdjson/internal/simdprune_tables.h"

#ifndef SIMDJSON_IMPLEMENTATION_ARM64
#define SIMDJSON_IMPLEMENTATION_ARM64 (SIMDJSON_IS_ARM64)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_ARM64 SIMDJSON_IMPLEMENTATION_ARM64 && SIMDJSON_IS_ARM64

// Default Haswell to on if this is x86-64. Even if we're not compiled for it, it could be selected
// at runtime.
#ifndef SIMDJSON_IMPLEMENTATION_HASWELL
#define SIMDJSON_IMPLEMENTATION_HASWELL (SIMDJSON_IS_X86_64)
#endif
// To see why  (__BMI__) && (__PCLMUL__) && (__LZCNT__) are not part of this next line, see
// https://github.com/simdjson/simdjson/issues/1247
#define SIMDJSON_CAN_ALWAYS_RUN_HASWELL ((SIMDJSON_IMPLEMENTATION_HASWELL) && (SIMDJSON_IS_X86_64) && (__AVX2__))

// Default Westmere to on if this is x86-64, unless we'll always select Haswell.
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

#endif // SIMDJSON_IMPLEMENTATION_BASE_H