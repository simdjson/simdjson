#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

#include "to_chars.cpp"
#include "from_chars.cpp"
#include "internal/error_tables.cpp"
#include "internal/jsoncharutils_tables.cpp"
#include "internal/numberparsing_tables.cpp"
#include "internal/simdprune_tables.cpp"
#include "implementation.cpp"

#if SIMDJSON_IMPLEMENTATION_ARM64
#include "arm64/implementation.cpp"
#include "arm64/dom_parser_implementation.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
#include "fallback/implementation.cpp"
#include "fallback/dom_parser_implementation.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
#include "haswell/implementation.cpp"
#include "haswell/dom_parser_implementation.cpp"
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
#include "westmere/implementation.cpp"
#include "westmere/dom_parser_implementation.cpp"
#endif

SIMDJSON_POP_DISABLE_WARNINGS
