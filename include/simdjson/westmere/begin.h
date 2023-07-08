//
// These need to be included outside SIMDJSON_TARGET_WESTMERE
//
#include "simdjson/westmere/implementation.h"
#include "simdjson/westmere/intrinsics.h"
#include "simdjson/westmere/target.h"

#define SIMDJSON_IMPLEMENTATION westmere
#define SIMDJSON_IMPLEMENTATION_MASK (1<<5)
SIMDJSON_TARGET_WESTMERE

#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/westmere/bitmanipulation.h"
#include "simdjson/westmere/bitmask.h"
#include "simdjson/westmere/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/westmere/stringparsing.h"
#include "simdjson/westmere/numberparsing.h"
