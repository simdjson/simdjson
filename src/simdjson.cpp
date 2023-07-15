#define SIMDJSON_SRC_SIMDJSON_CPP

#include "simdjson/common_defs.h"

SIMDJSON_PUSH_DISABLE_UNUSED_WARNINGS

#include "generic/dependencies.h"
#include "simdjson/amalgamated.h"

#include "to_chars.cpp"
#include "from_chars.cpp"
#include "internal/error_tables.cpp"
#include "internal/jsoncharutils_tables.cpp"
#include "internal/numberparsing_tables.cpp"
#include "internal/simdprune_tables.cpp"
#include "implementation.cpp"

#if SIMDJSON_IMPLEMENTATION_ARM64
#include "arm64.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include "fallback.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_ICELAKE
#include "icelake.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_PPC64
#include "ppc64.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include "westmere.cpp"
#endif

SIMDJSON_POP_DISABLE_UNUSED_WARNINGS

