#ifndef SIMDJSON_IMPLEMENTATIONS_H
#define SIMDJSON_IMPLEMENTATIONS_H

// Implementation-internal files (must be included before the implementations themselves, to keep
// amalgamation working--otherwise, the first time a file is included, it might be put inside the
// #ifdef SIMDJSON_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other implementations can't
// compile unless that implementation is turned on).
#include "simdjson/internal/isadetection.h"
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"
#include "simdjson/internal/simdprune_tables.h"

// Implementations
#include "simdjson/arm64.h"
#include "simdjson/haswell.h"
#include "simdjson/westmere.h"
#include "simdjson/ppc64.h"
#include "simdjson/fallback.h"
#include "simdjson/builtin.h"

#endif // SIMDJSON_IMPLEMENTATIONS_H