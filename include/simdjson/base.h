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

#if SIMDJSON_SUPPORTS_CHAR8_T
#include <string>
#include <string_view>
#endif

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
  comma_delimited_array,///< A single JSON array whose elements are iterated as
                        ///< comma-separated documents (e.g., `[{...},{...},{...}]`).
                        ///< The parser strips the outer `[` / `]` plus any
                        ///< surrounding JSON whitespace (space, tab, LF, CR)
                        ///< and then behaves like `comma_delimited` over the
                        ///< remaining bytes.
  newline_delimited     ///< NDJSON/JSON Lines where each document occupies exactly
                        ///< one line: documents are separated by line feeds and no
                        ///< document contains a raw line feed. Same inputs as
                        ///< `whitespace_delimited`, but the stronger guarantee lets
                        ///< the parser find the end of a document without walking
                        ///< it. On ondemand `iterate_many`, an unread remainder may
                        ///< be skipped by jumping to the next line feed without
                        ///< structure-validating that remainder. Use
                        ///< `whitespace_delimited` if unsure.
};

namespace internal {

template<typename T>
class atomic_ptr;
class dom_parser_implementation;
class escape_json_string;
class tape_ref;
struct value128;
enum class tape_type;

#if SIMDJSON_SUPPORTS_CHAR8_T
/**
 * Reinterpret a UTF-8 string as a C++20 std::u8string_view. No byte is copied
 * or modified: char8_t and char have the same size, representation and
 * alignment. Every string that simdjson produces is valid UTF-8, so this is a
 * lossless view over the very same memory.
 * @private
 */
simdjson_inline std::u8string_view as_u8string_view(std::string_view v) noexcept {
  return std::u8string_view(reinterpret_cast<const char8_t *>(v.data()), v.size());
}
#endif // SIMDJSON_SUPPORTS_CHAR8_T

/**
 * Assign a UTF-8 string to a string-like receiver. The general case simply
 * assigns the std::string_view: it covers std::string and any user type that
 * can be assigned from a std::string_view.
 * @private
 */
template <typename string_type>
simdjson_inline void assign_utf8(string_type &receiver, std::string_view content) noexcept {
  receiver = content;
}

#if SIMDJSON_SUPPORTS_CHAR8_T
/**
 * Assign a UTF-8 string to a char8_t-based string (e.g., std::u8string). This
 * overload is more specialized than the general one, so overload resolution
 * prefers it whenever the receiver holds char8_t.
 * @private
 */
template <typename traits_type, typename allocator_type>
simdjson_inline void assign_utf8(std::basic_string<char8_t, traits_type, allocator_type> &receiver, std::string_view content) noexcept {
  receiver.assign(reinterpret_cast<const char8_t *>(content.data()), content.size());
}

/**
 * Assign a UTF-8 string to a char8_t-based string view (e.g., std::u8string_view).
 * @private
 */
template <typename traits_type>
simdjson_inline void assign_utf8(std::basic_string_view<char8_t, traits_type> &receiver, std::string_view content) noexcept {
  receiver = std::basic_string_view<char8_t, traits_type>(reinterpret_cast<const char8_t *>(content.data()), content.size());
}
#endif // SIMDJSON_SUPPORTS_CHAR8_T

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_BASE_H
