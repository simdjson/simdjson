#ifndef SIMDJSON_WESTMERE_H
#define SIMDJSON_WESTMERE_H

#ifdef SIMDJSON_FALLBACK_H
#error "westmere.h must be included before fallback.h"
#endif

#include "simdjson/portability.h"

// Default Westmere to on if this is x86-64, unless we'll always select Haswell.
#ifndef SIMDJSON_IMPLEMENTATION_WESTMERE
#define SIMDJSON_IMPLEMENTATION_WESTMERE (SIMDJSON_IS_X86_64 && !SIMDJSON_REQUIRES_HASWELL)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_WESTMERE (SIMDJSON_IMPLEMENTATION_WESTMERE && SIMDJSON_IS_X86_64 && __SSE4_2__ && __PCLMUL__)

#if SIMDJSON_IMPLEMENTATION_WESTMERE

#define SIMDJSON_TARGET_WESTMERE SIMDJSON_TARGET_REGION("sse4.2,pclmul")

namespace simdjson {
/**
 * Implementation for Westmere (Intel SSE4.2).
 */
namespace westmere {
} // namespace westmere
} // namespace simdjson

//
// These two need to be included outside SIMDJSON_TARGET_REGION
//
#include "simdjson/westmere/implementation.h"
#include "simdjson/westmere/intrinsics.h"

//
// The rest need to be inside the region
//
#include "simdjson/westmere/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/westmere/bitmanipulation.h"
#include "simdjson/westmere/bitmask.h"
#include "simdjson/westmere/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/westmere/stringparsing.h"
#include "simdjson/westmere/numberparsing.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/ondemand.h"

// Inline definitions
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
#include "simdjson/generic/ondemand-inl.h"

#include "simdjson/westmere/end.h"

#endif // SIMDJSON_IMPLEMENTATION_WESTMERE
#endif // SIMDJSON_WESTMERE_COMMON_H
