#define SIMDJSON_IMPLEMENTATION rvv_vls
#include "simdjson/rvv-vls/base.h"
#include "simdjson/rvv-vls/intrinsics.h"
#include "simdjson/rvv-vls/bitmanipulation.h"
#include "simdjson/rvv-vls/bitmask.h"
#include "simdjson/rvv-vls/simd.h"
#include "simdjson/rvv-vls/stringparsing_defs.h"
#include "simdjson/rvv-vls/numberparsing_defs.h"

#define SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT 1
