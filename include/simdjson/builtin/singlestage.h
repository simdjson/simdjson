#ifndef SIMDJSON_BUILTIN_SINGLESTAGE_H
#define SIMDJSON_BUILTIN_SINGLESTAGE_H

#include "simdjson/builtin.h"
#include "simdjson/builtin/base.h"

#include "simdjson/generic/singlestage/dependencies.h"

#define SIMDJSON_AMALGAMATED

#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(arm64)
#include "simdjson/arm64/singlestage.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
#include "simdjson/fallback/singlestage.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(haswell)
#include "simdjson/haswell/singlestage.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(icelake)
#include "simdjson/icelake/singlestage.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(ppc64)
#include "simdjson/ppc64/singlestage.h"
#elif SIMDJSON_BUILTIN_IMPLEMENTATION_IS(westmere)
#include "simdjson/westmere/singlestage.h"
#else
#error Unknown SIMDJSON_BUILTIN_IMPLEMENTATION
#endif

#undef SIMDJSON_AMALGAMATED

namespace simdjson {
  /**
   * @copydoc simdjson::SIMDJSON_BUILTIN_IMPLEMENTATION::singlestage
   */
  namespace singlestage = SIMDJSON_BUILTIN_IMPLEMENTATION::singlestage;
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_SINGLESTAGE_H