#ifndef SIMDJSON_ICELAKE_BASE_H
#define SIMDJSON_ICELAKE_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

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
