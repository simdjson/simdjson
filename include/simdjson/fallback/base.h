#ifndef SIMDJSON_FALLBACK_BASE_H
#define SIMDJSON_FALLBACK_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
/**
 * Fallback implementation (runs on any machine).
 */
namespace fallback {

class implementation;

} // namespace fallback
} // namespace simdjson

#endif // SIMDJSON_FALLBACK_BASE_H
