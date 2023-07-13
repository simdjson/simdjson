#ifndef SIMDJSON_ICELAKE_BASE_H
#define SIMDJSON_ICELAKE_BASE_H

#include "simdjson/base.h"

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_ICELAKE
namespace simdjson {
/**
 * Implementation for Icelake (Intel AVX512).
 */
namespace icelake {

class implementation;

} // namespace icelake
} // namespace simdjson

#ifdef SIMDJSON_IN_EDITOR_IMPL
// If we're editing one of the files in this directory, begin the implementation!
#ifndef SIMDJSON_IMPLEMENTATION
#include "simdjson/icelake/begin.h"
#endif
#endif // SIMDJSON_IN_EDITOR_IMPL

#endif // SIMDJSON_ICELAKE_BASE_H
