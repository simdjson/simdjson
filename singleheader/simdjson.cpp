/* auto-generated on 2023-07-20 10:48:25 -0700. Do not edit! */
/* including simdjson.cpp:  */
/* begin file simdjson.cpp */
#define SIMDJSON_SRC_SIMDJSON_CPP

#include <base.h>

SIMDJSON_PUSH_DISABLE_UNUSED_WARNINGS

#include <to_chars.cpp>
#include <from_chars.cpp>
#include <internal/error_tables.cpp>
#include <internal/jsoncharutils_tables.cpp>
#include <internal/numberparsing_tables.cpp>
#include <internal/simdprune_tables.cpp>

#include <simdjson/generic/dependencies.h>
#include <generic/dependencies.h>
#include <generic/stage1/dependencies.h>
#include <generic/stage2/dependencies.h>

#include <implementation.cpp>

/* defining SIMDJSON_CONDITIONAL_INCLUDE */
#define SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_IMPLEMENTATION_ARM64
#include <arm64.cpp>
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include <fallback.cpp>
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
#include <haswell.cpp>
#endif
#if SIMDJSON_IMPLEMENTATION_ICELAKE
#include <icelake.cpp>
#endif
#if SIMDJSON_IMPLEMENTATION_PPC64
#include <ppc64.cpp>
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include <westmere.cpp>
#endif

/* undefining SIMDJSON_CONDITIONAL_INCLUDE */
#undef SIMDJSON_CONDITIONAL_INCLUDE

SIMDJSON_POP_DISABLE_UNUSED_WARNINGS

/* end file simdjson.cpp */
