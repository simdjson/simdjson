#define SIMDJSON_IMPLEMENTATION ppc64
#include "simdjson/ppc64/base.h"
#include "simdjson/ppc64/intrinsics.h"
#include "simdjson/ppc64/bitmanipulation.h"
#include "simdjson/ppc64/bitmask.h"
#include "simdjson/ppc64/numberparsing_defs.h"
#include "simdjson/ppc64/simd.h"
#include "simdjson/ppc64/stringparsing_defs.h"

#define SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT 1