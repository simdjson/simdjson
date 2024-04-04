#define SIMDJSON_IMPLEMENTATION lasx
#include "simdjson/lasx/base.h"
#include "simdjson/lasx/intrinsics.h"
#include "simdjson/lasx/bitmanipulation.h"
#include "simdjson/lasx/bitmask.h"
#include "simdjson/lasx/numberparsing_defs.h"
#include "simdjson/lasx/simd.h"
#include "simdjson/lasx/stringparsing_defs.h"

#define SIMDJSON_SKIP_BACKSLASH_SHORT_CIRCUIT 1
