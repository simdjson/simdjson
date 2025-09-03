#ifndef SIMDJSON_RVV_BEGIN_H
#define SIMDJSON_RVV_BEGIN_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif


#undef  SIMDJSON_IMPLEMENTATION
#define SIMDJSON_IMPLEMENTATION rvv

// include RVV intrinsics and definitions
#include "simdjson/rvv/intrinsics.h"
#include "simdjson/rvv/bitmanipulation.h"
#include "simdjson/rvv/bitmask.h"
#include "simdjson/rvv/numberparsing_defs.h"
#include "simdjson/rvv/simd.h"
#include "simdjson/rvv/stringparsing_defs.h"

#ifndef SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT
#define SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT 1
#endif

#endif // SIMDJSON_RVV_BEGIN_H