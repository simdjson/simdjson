//
// These two need to be included outside SIMDJSON_TARGET_HASWELL
//
#include "simdjson/haswell/implementation.h"
#include "simdjson/haswell/intrinsics.h"
#include "simdjson/haswell/target.h"

#define SIMDJSON_IMPLEMENTATION haswell
#define SIMDJSON_IMPLEMENTATION_MASK (1<<2)
SIMDJSON_TARGET_HASWELL

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/haswell/bitmanipulation.h"
#include "simdjson/haswell/bitmask.h"
#include "simdjson/haswell/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/haswell/stringparsing.h"
#include "simdjson/haswell/numberparsing.h"
