#ifndef SIMDJSON_BUILTIN_IMPLEMENTATION_H
#define SIMDJSON_BUILTIN_IMPLEMENTATION_H

#include "simdjson/implementation_detection.h"

#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(arm64)
#include "simdjson/arm64/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
#include "simdjson/fallback/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(haswell)
#include "simdjson/haswell/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(icelake)
#include "simdjson/icelake/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(ppc64)
#include "simdjson/ppc64/implementation.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(westmere)
#include "simdjson/westmere/implementation.h"
#else
#error Unknown SIMDJSON_BUILTIN_IMPLEMENTATION
#endif

#endif // SIMDJSON_BUILTIN_IMPLEMENTATION_H