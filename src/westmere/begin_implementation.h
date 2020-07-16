#include "simdjson.h"
#include "westmere/intrinsics.h" // Generally need to be included outside SIMDJSON_TARGET_REGION
#include "westmere/implementation.h"

#define SIMDJSON_IMPLEMENTATION westmere
SIMDJSON_TARGET_REGION("sse4.2,pclmul")
