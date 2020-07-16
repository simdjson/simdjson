#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

#include "error.cpp"
#include "implementation.cpp"

// Anything in the top level directory MUST be included outside of the #if statements
// below, or amalgamation will screw them up!
#include "isadetection.h"
#include "jsoncharutils_tables.h"
#include "simdprune_tables.h"

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
