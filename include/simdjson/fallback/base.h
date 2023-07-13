#ifndef SIMDJSON_FALLBACK_BASE_H
#define SIMDJSON_FALLBACK_BASE_H

#include "simdjson/base.h"

namespace simdjson {
/**
 * Fallback implementation (runs on any machine).
 */
namespace fallback {

class implementation;

} // namespace fallback
} // namespace simdjson

#ifdef SIMDJSON_IN_EDITOR_IMPL
// If we're editing one of the files in this directory, begin the implementation!
#ifndef SIMDJSON_IMPLEMENTATION
#include "simdjson/fallback/begin.h"
#endif
#endif // SIMDJSON_IN_EDITOR_IMPL

#endif // SIMDJSON_FALLBACK_BASE_H
