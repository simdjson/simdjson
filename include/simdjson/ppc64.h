#ifndef SIMDJSON_PPC64_H
#define SIMDJSON_PPC64_H

#ifdef SIMDJSON_FALLBACK_H
#error "ppc64.h must be included before fallback.h"
#endif

#include "simdjson/portability.h"

#ifndef SIMDJSON_IMPLEMENTATION_PPC64
#define SIMDJSON_IMPLEMENTATION_PPC64 (SIMDJSON_IS_PPC64)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_PPC64 SIMDJSON_IMPLEMENTATION_PPC64 && SIMDJSON_IS_PPC64

#include "simdjson/internal/isadetection.h"
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"
#include "simdjson/internal/simdprune_tables.h"

#if SIMDJSON_IMPLEMENTATION_PPC64

namespace simdjson {
/**
 * Implementation for ALTIVEC (PPC64).
 */
namespace ppc64 {
} // namespace ppc64
} // namespace simdjson

#include "simdjson/ppc64/implementation.h"

#include "simdjson/ppc64/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/ppc64/intrinsics.h"
#include "simdjson/ppc64/bitmanipulation.h"
#include "simdjson/ppc64/bitmask.h"
#include "simdjson/ppc64/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/ppc64/stringparsing.h"
#include "simdjson/ppc64/numberparsing.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/ondemand.h"

// Inline definitions
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
#include "simdjson/generic/ondemand-inl.h"
#include "simdjson/ppc64/end.h"

#endif // SIMDJSON_IMPLEMENTATION_PPC64

#endif // SIMDJSON_PPC64_H
