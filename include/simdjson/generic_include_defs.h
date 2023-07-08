#ifndef SIMDJSON_GENERIC_INCLUDE_DEFS_H
#define SIMDJSON_GENERIC_INCLUDE_DEFS_H

// BEGIN NOAMALGAMATE
// Used by generic files to ensure they are only included once
#ifndef SIMDJSON_IMPLEMENTATION // TODO ensure we really only do these in editor
#ifdef SIMDJSON_IMPLEMENTATIONS_H
#error "Coding error! If implementations.h is included, we expect SIMDJSON_IMPLEMENTATION to be defined"
#endif
#include "simdjson/fallback/begin.h"
#include "simdjson/fallback/numberparsing_defs.h"
#include "simdjson/fallback/bitmanipulation.h"
#endif
// END NOAMALGAMATE
#define SIMDJSON_GENERIC_ONCE(INCLUDED) ((INCLUDED & (1 << SIMDJSON_IMPLEMENTATION_MASK)) == 0)
#define SIMDJSON_GENERIC_INCLUDED(INCLUDED) (INCLUDED | (1 << SIMDJSON_IMPLEMENTATION_MASK))

#endif
