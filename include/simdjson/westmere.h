#ifndef SIMDJSON_WESTMERE_H
#define SIMDJSON_WESTMERE_H

#include "simdjson/implementation-base.h"

#if SIMDJSON_IMPLEMENTATION_WESTMERE

#if SIMDJSON_CAN_ALWAYS_RUN_WESTMERE
#define SIMDJSON_TARGET_WESTMERE
#define SIMDJSON_UNTARGET_WESTMERE
#else
#define SIMDJSON_TARGET_WESTMERE SIMDJSON_TARGET_REGION("sse4.2,pclmul")
#define SIMDJSON_UNTARGET_WESTMERE SIMDJSON_UNTARGET_REGION
#endif

namespace simdjson {
/**
 * Implementation for Westmere (Intel SSE4.2).
 */
namespace westmere {
} // namespace westmere
} // namespace simdjson

//
// These two need to be included outside SIMDJSON_TARGET_WESTMERE
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
#include "simdjson/westmere/end.h"

#endif // SIMDJSON_IMPLEMENTATION_WESTMERE
#endif // SIMDJSON_WESTMERE_COMMON_H
