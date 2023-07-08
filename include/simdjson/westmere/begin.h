#include "simdjson/westmere/target.h"

#define SIMDJSON_IMPLEMENTATION westmere
#define SIMDJSON_IMPLEMENTATION_MASK (1<<5)
SIMDJSON_TARGET_WESTMERE
