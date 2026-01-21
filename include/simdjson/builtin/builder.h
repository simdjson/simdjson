#ifndef SIMDJSON_BUILTIN_BUILDER_H
#define SIMDJSON_BUILTIN_BUILDER_H

#include "simdjson/builtin.h"
#include "simdjson/builtin/base.h"

#include "simdjson/generic/builder/dependencies.h"

#define SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(arm64)
#include "simdjson/arm64/builder.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
#include "simdjson/fallback/builder.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(haswell)
#include "simdjson/haswell/builder.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(icelake)
#include "simdjson/icelake/builder.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(ppc64)
#include "simdjson/ppc64/builder.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(westmere)
#include "simdjson/westmere/builder.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(lsx)
#include "simdjson/lsx/builder.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(lasx)
#include "simdjson/lasx/builder.h"
#else
#error Unknown SIMDJSON_BUILTIN_IMPLEMENTATION
#endif

#undef SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
  /**
   * @copydoc simdjson::SIMDJSON_BUILTIN_IMPLEMENTATION::builder
   */
  namespace builder = SIMDJSON_BUILTIN_IMPLEMENTATION::builder;
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_BUILDER_H