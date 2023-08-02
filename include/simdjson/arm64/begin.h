#define SIMDJSON_IMPLEMENTATION arm64
#include "simdjson/arm64/base.h"
#include "simdjson/arm64/intrinsics.h"
#include "simdjson/arm64/bitmanipulation.h"
#include "simdjson/arm64/bitmask.h"
#include "simdjson/arm64/numberparsing_defs.h"
#include "simdjson/arm64/simd.h"
#include "simdjson/arm64/stringparsing_defs.h"

#define SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT 1