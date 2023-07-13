#include "simdjson/builtin.h"

#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(arm64)
#include "simdjson/arm64/begin.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
#include "simdjson/fallback/begin.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(haswell)
#include "simdjson/haswell/begin.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(icelake)
#include "simdjson/icelake/begin.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(ppc64)
#include "simdjson/ppc64/begin.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(westmere)
#include "simdjson/westmere/begin.h"
#else
#error Unknown SIMDJSON_BUILTIN_IMPLEMENTATION
#endif