/**
 * @file Base declarations for all simdjson headers
 * @private
 */
#ifndef SIMDJSON_BASE_H
#define SIMDJSON_BASE_H

#include "simdjson/common_defs.h"
#include "simdjson/compiler_check.h"
#include "simdjson/error.h"
#include "simdjson/portability.h"
#include "simdjson/concepts.h"

/**
 * @brief The top level simdjson namespace, containing everything the library provides.
 */
namespace simdjson {

SIMDJSON_PUSH_DISABLE_UNUSED_WARNINGS

/** The maximum document size supported by simdjson. */
constexpr size_t SIMDJSON_MAXSIZE_BYTES = 0xFFFFFFFF;

/**
 * The amount of padding needed in a buffer to parse JSON.
 *
 * The input buf should be readable up to buf + SIMDJSON_PADDING
 * this is a stopgap; there should be a better description of the
 * main loop and its behavior that abstracts over this
 * See https://github.com/simdjson/simdjson/issues/174
 */
constexpr size_t SIMDJSON_PADDING = 64;

/**
 * By default, simdjson supports this many nested objects and arrays.
 *
 * This is the default for parser::max_depth().
 */
constexpr size_t DEFAULT_MAX_DEPTH = 1024;

SIMDJSON_POP_DISABLE_UNUSED_WARNINGS

class implementation;
struct padded_string;
class padded_string_view;
enum class stage1_mode;

namespace internal {

template<typename T>
class atomic_ptr;
class dom_parser_implementation;
class escape_json_string;
class tape_ref;
struct value128;
enum class tape_type;

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_BASE_H
