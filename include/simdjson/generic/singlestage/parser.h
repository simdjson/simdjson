#ifndef SIMDJSON_GENERIC_SINGLESTAGE_PARSER_H

#ifndef SIMDJSON_AMALGAMATED
#define SIMDJSON_GENERIC_SINGLESTAGE_PARSER_H
#include "simdjson/generic/singlestage/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#endif // SIMDJSON_AMALGAMATED

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace singlestage {

/**
 * A JSON fragment iterator.
 *
 * This holds the actual iterator as well as the buffer for writing strings.
 */
class parser {
public:
  /**
   * Create a JSON parser.
   *
   * The new parser will have zero capacity.
   */
  inline explicit parser(size_t max_capacity = SIMDJSON_MAXSIZE_BYTES) noexcept;

  inline parser(parser &&other) noexcept = default;
  simdjson_inline parser(const parser &other) = delete;
  simdjson_inline parser &operator=(const parser &other) = delete;
  simdjson_inline parser &operator=(parser &&other) noexcept = default;

  /** Deallocate the JSON parser. */
  inline ~parser() noexcept = default;

  /**
   * Start iterating an on-demand JSON document.
   *
   *   singlestage::parser parser;
   *   document doc = parser.iterate(json);
   *
   * It is expected that the content is a valid UTF-8 file, containing a valid JSON document.
   * Otherwise the iterate method may return an error. In particular, the whole input should be
   * valid: we do not attempt to tolerate incorrect content either before or after a JSON
   * document.
   *
   * ### IMPORTANT: Validate what you use
   *
   * Calling iterate on an invalid JSON document may not immediately trigger an error. The call to
   * iterate does not parse and validate the whole document.
   *
   * ### IMPORTANT: Buffer Lifetime
   *
   * Because parsing is done while you iterate, you *must* keep the JSON buffer around at least as
   * long as the document iteration.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * Only one iteration at a time can happen per parser, and the parser *must* be kept alive during
   * iteration to ensure intermediate buffers can be accessed. Any document must be destroyed before
   * you call parse() again or destroy the parser.
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated. These bytes will be read: if you
   * using a sanitizer that verifies that no uninitialized byte is read, then you should initialize the
   * SIMDJSON_PADDING bytes to avoid runtime warnings.
   *
   * @param json The JSON to parse.
   * @param len The length of the JSON.
   * @param capacity The number of bytes allocated in the JSON (must be at least len+SIMDJSON_PADDING).
   *
   * @return The document, or an error:
   *         - INSUFFICIENT_PADDING if the input has less than SIMDJSON_PADDING extra bytes.
   *         - MEMALLOC if realloc_if_needed the parser does not have enough capacity, and memory
   *           allocation fails.
   *         - EMPTY if the document is all whitespace.
   *         - UTF8_ERROR if the document is not valid UTF-8.
   *         - UNESCAPED_CHARS if a string contains control characters that must be escaped
   *         - UNCLOSED_STRING if there is an unclosed string in the document.
   */
  simdjson_warn_unused simdjson_result<document> iterate(padded_string_view json) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const char *json, size_t len, size_t capacity) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const uint8_t *json, size_t len, size_t capacity) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(std::string_view json, size_t capacity) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const std::string &json) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const simdjson_result<padded_string> &json) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const simdjson_result<padded_string_view> &json) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(padded_string &&json) & noexcept = delete;

  /**
   * @private
   *
   * Start iterating an on-demand JSON document.
   *
   *   singlestage::parser parser;
   *   json_iterator doc = parser.iterate(json);
   *
   * ### IMPORTANT: Buffer Lifetime
   *
   * Because parsing is done while you iterate, you *must* keep the JSON buffer around at least as
   * long as the document iteration.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * Only one iteration at a time can happen per parser, and the parser *must* be kept alive during
   * iteration to ensure intermediate buffers can be accessed. Any document must be destroyed before
   * you call parse() again or destroy the parser.
   *
   * The singlestage::document instance holds the iterator. The document must remain in scope
   * while you are accessing instances of singlestage::value, singlestage::object, singlestage::array.
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated. These bytes will be read: if you
   * using a sanitizer that verifies that no uninitialized byte is read, then you should initialize the
   * SIMDJSON_PADDING bytes to avoid runtime warnings.
   *
   * @param json The JSON to parse.
   *
   * @return The iterator, or an error:
   *         - INSUFFICIENT_PADDING if the input has less than SIMDJSON_PADDING extra bytes.
   *         - MEMALLOC if realloc_if_needed the parser does not have enough capacity, and memory
   *           allocation fails.
   *         - EMPTY if the document is all whitespace.
   *         - UTF8_ERROR if the document is not valid UTF-8.
   *         - UNESCAPED_CHARS if a string contains control characters that must be escaped
   *         - UNCLOSED_STRING if there is an unclosed string in the document.
   */
  simdjson_warn_unused simdjson_result<json_iterator> iterate_raw(padded_string_view json) & noexcept;

  /** The capacity of this parser (the largest document it can process). */
  simdjson_inline size_t capacity() const noexcept;
  /** The maximum capacity of this parser (the largest document it is allowed to process). */
  simdjson_inline size_t max_capacity() const noexcept;
  simdjson_inline void set_max_capacity(size_t max_capacity) noexcept;
  /**
   * The maximum depth of this parser (the most deeply nested objects and arrays it can process).
   * This parameter is only relevant when the macro SIMDJSON_DEVELOPMENT_CHECKS is set to true.
   * The document's instance current_depth() method should be used to monitor the parsing
   * depth and limit it if desired.
   */
  simdjson_inline size_t max_depth() const noexcept;

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * The max_depth parameter is only relevant when the macro SIMDJSON_DEVELOPMENT_CHECKS is set to true.
   * The document's instance current_depth() method should be used to monitor the parsing
   * depth and limit it if desired.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return The error, if there is one.
   */
  simdjson_warn_unused error_code allocate(size_t capacity, size_t max_depth=DEFAULT_MAX_DEPTH) noexcept;

  #ifdef SIMDJSON_THREADS_ENABLED
  /**
   * The parser instance can use threads when they are available to speed up some
   * operations. It is enabled by default. Changing this attribute will change the
   * behavior of the parser for future operations.
   */
  bool threaded{true};
  #endif

  /**
   * Unescape this JSON string, replacing \\ with \, \n with newline, etc. to a user-provided buffer.
   * The result must be valid UTF-8.
   * The provided pointer is advanced to the end of the string by reference, and a string_view instance
   * is returned. You can ensure that your buffer is large enough by allocating a block of memory at least
   * as large as the input JSON plus SIMDJSON_PADDING and then unescape all strings to this one buffer.
   *
   * This unescape function is a low-level function. If you want a more user-friendly approach, you should
   * avoid raw_json_string instances (e.g., by calling unescaped_key() instead of key() or get_string()
   * instead of get_raw_json_string()).
   *
   * ## IMPORTANT: string_view lifetime
   *
   * The string_view is only valid as long as the bytes in dst.
   *
   * @param raw_json_string input
   * @param dst A pointer to a buffer at least large enough to write this string as well as
   *            an additional SIMDJSON_PADDING bytes.
   * @param allow_replacement Whether we allow a replacement if the input string contains unmatched surrogate pairs.
   * @return A string_view pointing at the unescaped string in dst
   * @error STRING_ERROR if escapes are incorrect.
   */
  simdjson_inline simdjson_result<std::string_view> unescape(raw_json_string in, uint8_t *&dst, bool allow_replacement = false) const noexcept;

  /**
   * Unescape this JSON string, replacing \\ with \, \n with newline, etc. to a user-provided buffer.
   * The result may not be valid UTF-8. See https://simonsapin.github.io/wtf-8/
   * The provided pointer is advanced to the end of the string by reference, and a string_view instance
   * is returned. You can ensure that your buffer is large enough by allocating a block of memory at least
   * as large as the input JSON plus SIMDJSON_PADDING and then unescape all strings to this one buffer.
   *
   * This unescape function is a low-level function. If you want a more user-friendly approach, you should
   * avoid raw_json_string instances (e.g., by calling unescaped_key() instead of key() or get_string()
   * instead of get_raw_json_string()).
   *
   * ## IMPORTANT: string_view lifetime
   *
   * The string_view is only valid as long as the bytes in dst.
   *
   * @param raw_json_string input
   * @param dst A pointer to a buffer at least large enough to write this string as well as
   *            an additional SIMDJSON_PADDING bytes.
   * @return A string_view pointing at the unescaped string in dst
   * @error STRING_ERROR if escapes are incorrect.
   */
  simdjson_inline simdjson_result<std::string_view> unescape_wobbly(raw_json_string in, uint8_t *&dst) const noexcept;

private:
  /** @private [for benchmarking access] The implementation to use */
  std::unique_ptr<internal::dom_parser_implementation> implementation{};
  size_t _capacity{0};
  size_t _max_capacity;
  size_t _max_depth{DEFAULT_MAX_DEPTH};
  std::unique_ptr<uint8_t[]> string_buf{};
#if SIMDJSON_DEVELOPMENT_CHECKS
  std::unique_ptr<token_position[]> start_positions{};
#endif

  friend class json_iterator;
};

} // namespace singlestage
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::parser> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::singlestage::parser> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::singlestage::parser &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;
};

} // namespace simdjson

#endif // SIMDJSON_GENERIC_SINGLESTAGE_PARSER_H