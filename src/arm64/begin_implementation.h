#include "simdjson.h"
#include "arm64/implementation.h"
#include "arm64/intrinsics.h" // Generally need to be included outside SIMDJSON_TARGET_REGION

#define SIMDJSON_IMPLEMENTATION arm64
