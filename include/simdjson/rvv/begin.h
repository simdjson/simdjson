#define SIMDJSON_IMPLEMENTATION rvv

// include RVV intrinsics and definitions
#include <riscv_vector.h>
#include "simdjson/rvv/base.h"
#include "simdjson/rvv/intrinsics.h"
#include "simdjson/rvv/bitmanipulation.h"
#include "simdjson/rvv/bitmask.h"
#include "simdjson/rvv/numberparsing_defs.h"
#include "simdjson/rvv/simd.h"
#include "simdjson/rvv/stringparsing_defs.h"

#define SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT 1
