#define SIMDJSON_IMPLEMENTATION westmere
#include "simdjson/westmere/base.h"
#include "simdjson/westmere/intrinsics.h"

#if !SIMDJSON_CAN_ALWAYS_RUN_WESTMERE
SIMDJSON_TARGET_REGION("sse4.2,pclmul,popcnt")
#endif

#include "simdjson/westmere/bitmanipulation.h"
#include "simdjson/westmere/bitmask.h"
#include "simdjson/westmere/numberparsing_defs.h"
#include "simdjson/westmere/simd.h"
#include "simdjson/westmere/stringparsing_defs.h"
