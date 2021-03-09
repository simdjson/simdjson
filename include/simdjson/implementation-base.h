#ifndef SIMDJSON_IMPLEMENTATION_BASE_H
#define SIMDJSON_IMPLEMENTATION_BASE_H

/**
 * @file
 *
 * Includes common stuff needed for implementations.
 */

#include "simdjson/base.h"
#include "simdjson/implementation.h"

// Implementation-internal files (must be included before the implementations themselves, to keep
// amalgamation working--otherwise, the first time a file is included, it might be put inside the
// #ifdef SIMDJSON_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other implementations can't
// compile unless that implementation is turned on).
#include "simdjson/internal/isadetection.h"
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"
#include "simdjson/internal/simdprune_tables.h"

#endif // SIMDJSON_IMPLEMENTATION_BASE_H