#ifndef SIMDJSON_ICELAKE_H
#define SIMDJSON_ICELAKE_H

#include "simdjson/implementation-base.h"

#if SIMDJSON_IMPLEMENTATION_ICELAKE

namespace simdjson {
/**
 * Implementation for Icelake (Intel AVX512).
 */
namespace icelake {
} // namespace icelake
} // namespace simdjson

//
// These two need to be included outside SIMDJSON_TARGET_ICELAKE
//
#include "simdjson/icelake/implementation.h"
#include "simdjson/icelake/intrinsics.h"

//
// The rest need to be inside the region
//
#include "simdjson/icelake/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/icelake/bitmanipulation.h"
#include "simdjson/icelake/bitmask.h"
#include "simdjson/icelake/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/icelake/stringparsing.h"
#include "simdjson/icelake/numberparsing.h"
#include "simdjson/icelake/end.h"

#endif // SIMDJSON_IMPLEMENTATION_ICELAKE
#endif // SIMDJSON_ICELAKE_H
