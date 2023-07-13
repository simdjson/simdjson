#ifndef SIMDJSON_BUILTIN_AMALGAMATED_H
#define SIMDJSON_BUILTIN_AMALGAMATED_H

#include "simdjson/builtin.h"

#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(arm64)
#include "simdjson/arm64/amalgamated.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
#include "simdjson/fallback/amalgamated.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(haswell)
#include "simdjson/haswell/amalgamated.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(icelake)
#include "simdjson/icelake/amalgamated.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(ppc64)
#include "simdjson/ppc64/amalgamated.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(westmere)
#include "simdjson/westmere/amalgamated.h"
#else
#error Unknown SIMDJSON_BUILTIN_IMPLEMENTATION
#endif

#endif // SIMDJSON_BUILTIN_AMALGAMATED_H