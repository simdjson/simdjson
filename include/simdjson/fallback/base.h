#ifndef SIMDJSON_FALLBACK_BASE_H
#define SIMDJSON_FALLBACK_BASE_H

#ifndef SIMDJSON_AMALGAMATED
#include "simdjson/base.h"
#endif // SIMDJSON_AMALGAMATED

namespace simdjson {
/**
 * Fallback implementation (runs on any machine).
 */
namespace fallback {

class implementation;

} // namespace fallback
} // namespace simdjson

#ifndef SIMDJSON_AMALGAMATED
// If we're editing one of the files in this directory, begin the implementation!
#ifndef SIMDJSON_IMPLEMENTATION
#include "simdjson/fallback/begin.h"
#endif
#endif // SIMDJSON_AMALGAMATED

#endif // SIMDJSON_FALLBACK_BASE_H
