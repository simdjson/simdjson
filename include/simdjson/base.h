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
#include "simdjson/constevalutil.h"

/**
 * @brief The top level simdjson namespace, containing everything the library provides.
 */
namespace simdjson {

SIMDJSON_PUSH_DISABLE_UNUSED_WARNINGS

/** The maximum document size supported by simdjson. */
constexpr size_t SIMDJSON_MAXSIZE_BYTES = 0xFFFFFFFF;
/** The maximum depth of nested objects and arrays supported by simdjson.
 A depth of SIMDJSON_MAXSIZE_BYTES/2 is not reasonable and would be
 adversarial, but it serves as an upper bound for validation purposes. */
constexpr size_t SIMDJSON_MAX_DEPTH = SIMDJSON_MAXSIZE_BYTES/2;

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

/**
 * Stream format for parse_many/iterate_many.
 */
enum class stream_format {
  whitespace_delimited, ///< Whitespace-delimited JSON documents (default, includes NDJSON/JSONL)
  json_sequence,        ///< RFC 7464 JSON text sequences (RS-delimited)
  comma_delimited,      ///< Comma-separated JSON documents (e.g., `{...},{...},{...}`)
  comma_delimited_array ///< A single JSON array whose elements are iterated as
                        ///< comma-separated documents (e.g., `[{...},{...},{...}]`).
                        ///< The parser strips the outer `[` / `]` plus any
                        ///< surrounding JSON whitespace (space, tab, LF, CR)
                        ///< and then behaves like `comma_delimited` over the
                        ///< remaining bytes.
};

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
