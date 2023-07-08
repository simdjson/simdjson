#include "simdjson/arm64/implementation.h"

#define SIMDJSON_IMPLEMENTATION arm64
#define SIMDJSON_IMPLEMENTATION_MASK (1<<1)

#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/arm64/intrinsics.h"
#include "simdjson/arm64/bitmanipulation.h"
#include "simdjson/arm64/bitmask.h"
#include "simdjson/arm64/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/arm64/stringparsing.h"
#include "simdjson/arm64/numberparsing.h"
