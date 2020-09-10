#ifndef SIMDJSON_HASWELL_H
#define SIMDJSON_HASWELL_H

#include "simdjson/portability.h"

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
