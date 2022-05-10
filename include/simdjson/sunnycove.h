#ifndef SIMDJSON_SUNNYCOVE_H
#define SIMDJSON_SUNNYCOVE_H

#include "simdjson/implementation-base.h"

#if SIMDJSON_IMPLEMENTATION_SUNNYCOVE

#if SIMDJSON_CAN_ALWAYS_RUN_SUNNYCOVE
#define SIMDJSON_TARGET_SUNNYCOVE
#define SIMDJSON_UNTARGET_SUNNYCOVE
#else
#define SIMDJSON_TARGET_SUNNYCOVE SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt")
#define SIMDJSON_UNTARGET_SUNNYCOVE SIMDJSON_UNTARGET_REGION
#endif

namespace simdjson {
/**
 * Implementation for Sunnycove (Intel AVX-512).
 */
namespace sunnycove {
} // namespace sunnycove
} // namespace simdjson

//
// These two need to be included outside SIMDJSON_TARGET_SUNNYCOVE
//
#include "simdjson/sunnycove/implementation.h"
#include "simdjson/sunnycove/intrinsics.h"

//
// The rest need to be inside the region
//
#include "simdjson/sunnycove/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/sunnycove/bitmanipulation.h"
#include "simdjson/sunnycove/bitmask.h"
#include "simdjson/sunnycove/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/sunnycove/stringparsing.h"
#include "simdjson/sunnycove/numberparsing.h"
#include "simdjson/sunnycove/end.h"

#endif // SIMDJSON_IMPLEMENTATION_SUNNYCOVE
#endif // SIMDJSON_SUNNYCOVE_COMMON_H
