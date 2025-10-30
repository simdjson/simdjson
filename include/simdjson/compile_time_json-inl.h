/**
 * @file compile_time_json-inl.h
 * @brief Implementation details for compile-time JSON parsing
 *
 * This file contains inline implementations and helper utilities for compile-time
 * JSON parsing. Currently, the main implementation is self-contained in the header.
 */

#ifndef SIMDJSON_GENERIC_COMPILE_TIME_JSON_INL_H

#if SIMDJSON_STATIC_REFLECTION

#include <meta>
#include <array>
#include <string_view>
#include <cstdint>
#include "simdjson/compile_time_json.h"

namespace simdjson {
namespace compile_time {
// Currently, all implementations are in compile_time_json.h

} // namespace compile_time
} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_COMPILE_TIME_JSON_INL_H
