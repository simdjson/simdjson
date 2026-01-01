#define SIMDJSON_IMPLEMENTATION lasx
#if SIMDJSON_CAN_ALWAYS_RUN_LASX
// nothing needed.
#else
SIMDJSON_TARGET_REGION("lasx,lsx")
#endif

#include "simdjson/lasx/base.h"
#include "simdjson/lasx/intrinsics.h"
#include "simdjson/lasx/bitmanipulation.h"
#include "simdjson/lasx/bitmask.h"
#include "simdjson/lasx/numberparsing_defs.h"
#include "simdjson/lasx/simd.h"
#include "simdjson/lasx/stringparsing_defs.h"

#define SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT 1


