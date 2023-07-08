#include "simdjson/ppc64/implementation.h"

#define SIMDJSON_IMPLEMENTATION ppc64
#define SIMDJSON_IMPLEMENTATION_MASK (1<<4)

#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/ppc64/intrinsics.h"
#include "simdjson/ppc64/bitmanipulation.h"
#include "simdjson/ppc64/bitmask.h"
#include "simdjson/ppc64/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/ppc64/stringparsing.h"
#include "simdjson/ppc64/numberparsing.h"
