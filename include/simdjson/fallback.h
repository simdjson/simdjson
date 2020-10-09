#ifndef SIMDJSON_FALLBACK_H
#define SIMDJSON_FALLBACK_H

#include "simdjson/portability.h"

// Default Fallback to on unless a builtin implementation has already been selected.
#ifndef SIMDJSON_IMPLEMENTATION_FALLBACK
#define SIMDJSON_IMPLEMENTATION_FALLBACK 1 // (!SIMDJSON_CAN_ALWAYS_RUN_ARM64 && !SIMDJSON_CAN_ALWAYS_RUN_HASWELL && !SIMDJSON_CAN_ALWAYS_RUN_WESTMERE)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_FALLBACK SIMDJSON_IMPLEMENTATION_FALLBACK

#if SIMDJSON_IMPLEMENTATION_FALLBACK

namespace simdjson {
/**
 * Fallback implementation (runs on any machine).
 */
namespace fallback {
} // namespace fallback
} // namespace simdjson

#include "simdjson/fallback/implementation.h"

#include "simdjson/fallback/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/fallback/bitmanipulation.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/fallback/stringparsing.h"
#include "simdjson/fallback/numberparsing.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/ondemand.h"

// Inline definitions
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
#include "simdjson/generic/ondemand-inl.h"

#include "simdjson/fallback/end.h"

#endif // SIMDJSON_IMPLEMENTATION_FALLBACK
#endif // SIMDJSON_FALLBACK_H
