#ifndef SIMDJSON_SKYLAKE_H
#define SIMDJSON_SKYLAKE_H

#include "simdjson/implementation-base.h"

#if SIMDJSON_IMPLEMENTATION_SKYLAKE

#if SIMDJSON_CAN_ALWAYS_RUN_SKYLAKE
#define SIMDJSON_TARGET_SKYLAKE
#define SIMDJSON_UNTARGET_SKYLAKE
#else
#define SIMDJSON_TARGET_SKYLAKE SIMDJSON_TARGET_REGION("avx512f,avx512dq,avx512cd,avx512bw,avx512vl,avx2,bmi,pclmul,lzcnt")
#define SIMDJSON_UNTARGET_SKYLAKE SIMDJSON_UNTARGET_REGION
#endif

namespace simdjson {
/**
 * Implementation for SKYLAKE (Intel AVX512).
 */
namespace skylake {
} // namespace skylake
} // namespace simdjson

//
// These two need to be included outside SIMDJSON_TARGET_SKYLAKE
//
#include "simdjson/skylake/implementation.h"
#include "simdjson/skylake/intrinsics.h"

//
// The rest need to be inside the region
//
#include "simdjson/skylake/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/skylake/bitmanipulation.h"
#include "simdjson/skylake/bitmask.h"
#include "simdjson/skylake/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/skylake/stringparsing.h"
#include "simdjson/skylake/numberparsing.h"
#include "simdjson/skylake/end.h"

#endif // SIMDJSON_IMPLEMENTATION_SKYLAKE
#endif // SIMDJSON_SKYLAKE_H
