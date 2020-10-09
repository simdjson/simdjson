#ifndef SIMDJSON_HASWELL_H
#define SIMDJSON_HASWELL_H

#ifdef SIMDJSON_WESTMERE_H
#error "haswell.h must be included before westmere.h"
#endif
#ifdef SIMDJSON_FALLBACK_H
#error "haswell.h must be included before fallback.h"
#endif

#include "simdjson/portability.h"

// Default Haswell to on if this is x86-64. Even if we're not compiled for it, it could be selected
// at runtime.
#ifndef SIMDJSON_IMPLEMENTATION_HASWELL
#define SIMDJSON_IMPLEMENTATION_HASWELL (SIMDJSON_IS_X86_64)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_HASWELL ((SIMDJSON_IMPLEMENTATION_HASWELL) && (SIMDJSON_IS_X86_64) && (__AVX2__) && (__BMI__) && (__PCLMUL__) && (__LZCNT__))

#if SIMDJSON_IMPLEMENTATION_HASWELL

#define SIMDJSON_TARGET_HASWELL SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt")

namespace simdjson {
/**
 * Implementation for Haswell (Intel AVX2).
 */
namespace haswell {
} // namespace haswell
} // namespace simdjson

//
// These two need to be included outside SIMDJSON_TARGET_REGION
//
#include "simdjson/haswell/implementation.h"
#include "simdjson/haswell/intrinsics.h"

//
// The rest need to be inside the region
//
#include "simdjson/haswell/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/haswell/bitmanipulation.h"
#include "simdjson/haswell/bitmask.h"
#include "simdjson/haswell/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/haswell/stringparsing.h"
#include "simdjson/haswell/numberparsing.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/ondemand.h"

// Inline definitions
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
#include "simdjson/generic/ondemand-inl.h"

#include "simdjson/haswell/end.h"

#endif // SIMDJSON_IMPLEMENTATION_HASWELL
#endif // SIMDJSON_HASWELL_COMMON_H
