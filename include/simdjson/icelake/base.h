#ifndef SIMDJSON_ICELAKE_BASE_H
#define SIMDJSON_ICELAKE_BASE_H

#ifndef SIMDJSON_AMALGAMATED
#include "simdjson/base.h"
#endif // SIMDJSON_AMALGAMATED

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_ICELAKE
namespace simdjson {
/**
 * Implementation for Icelake (Intel AVX512).
 */
namespace icelake {

class implementation;

} // namespace icelake
} // namespace simdjson

#endif // SIMDJSON_ICELAKE_BASE_H
