#define SIMDJSON_IMPLEMENTATION westmere
#define SIMDJSON_TARGET_WESTMERE SIMDJSON_TARGET_REGION("sse4.2,pclmul")

#include "westmere/intrinsics.h" // Generally need to be included outside SIMDJSON_TARGET_REGION
#include "westmere/implementation.h"

SIMDJSON_TARGET_WESTMERE

#include "westmere/bitmanipulation.h"
#include "westmere/bitmask.h"
#include "westmere/simd.h"
