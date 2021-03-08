#ifndef SIMDJSON_FALLBACK_H
#define SIMDJSON_FALLBACK_H

#include "simdjson/implementation-base.h"

#if SIMDJSON_IMPLEMENTATION_FALLBACK

namespace simdjson {
/**
 * Fallback implementation (runs on any machine).
 */
namespace fallback {
} // namespace fallback
} // namespace simdjson

#include "simdjson/fallback/implementation.h"

#include "simdjson/fallback/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/fallback/bitmanipulation.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/fallback/stringparsing.h"
#include "simdjson/fallback/numberparsing.h"
#include "simdjson/fallback/end.h"

#endif // SIMDJSON_IMPLEMENTATION_FALLBACK
#endif // SIMDJSON_FALLBACK_H
