#ifndef SIMDJSON_CASCADELAKE_H
#define SIMDJSON_CASCADELAKE_H

#include "simdjson/implementation-base.h"

#if SIMDJSON_IMPLEMENTATION_CASCADELAKE

#if SIMDJSON_CAN_ALWAYS_RUN_CASCADELAKE
#define SIMDJSON_TARGET_CASCADELAKE
#define SIMDJSON_UNTARGET_CASCADELAKE
#else
#define SIMDJSON_TARGET_CASCADELAKE SIMDJSON_TARGET_REGION("avx512f,avx512dq,avx512cd,avx512bw,avx512vl,avx2,bmi,pclmul,lzcnt")
#define SIMDJSON_UNTARGET_CASCADELAKE SIMDJSON_UNTARGET_REGION
#endif

namespace simdjson {
/**
 * Implementation for Cascadelake (Intel AVX512).
 */
namespace cascadelake {
} // namespace cascadelake
} // namespace simdjson

//
// These two need to be included outside SIMDJSON_TARGET_CASCADELAKE
//
#include "simdjson/cascadelake/implementation.h"
#include "simdjson/cascadelake/intrinsics.h"

//
// The rest need to be inside the region
//
#include "simdjson/cascadelake/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/cascadelake/bitmanipulation.h"
#include "simdjson/cascadelake/bitmask.h"
#include "simdjson/cascadelake/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/cascadelake/stringparsing.h"
#include "simdjson/cascadelake/numberparsing.h"
#include "simdjson/cascadelake/end.h"

#endif // SIMDJSON_IMPLEMENTATION_CASCADELAKE
#endif // SIMDJSON_CASCADELAKE_H
