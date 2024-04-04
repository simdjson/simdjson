#define SIMDJSON_IMPLEMENTATION lsx
#include "simdjson/lsx/base.h"
#include "simdjson/lsx/intrinsics.h"
#include "simdjson/lsx/bitmanipulation.h"
#include "simdjson/lsx/bitmask.h"
#include "simdjson/lsx/numberparsing_defs.h"
#include "simdjson/lsx/simd.h"
#include "simdjson/lsx/stringparsing_defs.h"

#define SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT 1
