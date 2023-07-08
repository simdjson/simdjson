//
// These two need to be included outside SIMDJSON_TARGET_ICELAKE
//
#include "simdjson/icelake/implementation.h"
#include "simdjson/icelake/intrinsics.h"
#include "simdjson/icelake/target.h"

#define SIMDJSON_IMPLEMENTATION icelake
#define SIMDJSON_IMPLEMENTATION_MASK (1<<3)
SIMDJSON_TARGET_ICELAKE

#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/icelake/bitmanipulation.h"
#include "simdjson/icelake/bitmask.h"
#include "simdjson/icelake/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/icelake/stringparsing.h"
#include "simdjson/icelake/numberparsing.h"
