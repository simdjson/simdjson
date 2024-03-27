#ifndef SIMDJSON_BUILTIN_H
#define SIMDJSON_BUILTIN_H

#include "simdjson/builtin/base.h"
#include "simdjson/builtin/implementation.h"

#include "simdjson/generic/dependencies.h"

#define SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(arm64)
#include "simdjson/arm64.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
#include "simdjson/fallback.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(haswell)
#include "simdjson/haswell.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(icelake)
#include "simdjson/icelake.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(ppc64)
#include "simdjson/ppc64.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(westmere)
#include "simdjson/westmere.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(lsx)
#include "simdjson/lsx.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(lasx)
#include "simdjson/lasx.h"
#else
#error Unknown SIMDJSON_BUILTIN_IMPLEMENTATION
#endif

#undef SIMDJSON_CONDITIONAL_INCLUDE

#endif // SIMDJSON_BUILTIN_H
