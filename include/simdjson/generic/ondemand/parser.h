#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class array;
class object;
class value;
class raw_json_string;

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
  inline parser() noexcept = default;

  inline parser(parser &&other) noexcept = default;
  simdjson_really_inline parser(const parser &other) = delete;
  simdjson_really_inline parser &operator=(const parser &other) = delete;

  /** Deallocate the JSON parser. */
  inline ~parser() noexcept = default;

  /**
   * Start iterating an on-demand JSON document.
   * 
   *   ondemand::parser parser;
   *   document doc = parser.iterate(json);
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
   * those bytes are initialized to, as long as they are allocated.
   *
   * @param json The JSON to parse.
   * 
   * @return The document, or an error:
   *         - MEMALLOC if realloc_if_needed the parser does not have enough capacity, and memory
   *           allocation fails.
   *         - EMPTY if the document is all whitespace.
   *         - UTF8_ERROR if the document is not valid UTF-8.
   *         - UNESCAPED_CHARS if a string contains control characters that must be escaped
   *         - UNCLOSED_STRING if there is an unclosed string in the document.
   */
  simdjson_warn_unused simdjson_result<document> iterate(const padded_string &json) & noexcept;
  simdjson_warn_unused simdjson_result<document> iterate(const std::string &json) & noexcept = delete;
  /**
   * @private
   * 
   * Start iterating an on-demand JSON document.
   * 
   *   ondemand::parser parser;
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
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated.
   *
   * @param json The JSON to parse.
   * 
   * @return The iterator, or an error:
   *         - MEMALLOC if realloc_if_needed the parser does not have enough capacity, and memory
   *           allocation fails.
   *         - EMPTY if the document is all whitespace.
   *         - UTF8_ERROR if the document is not valid UTF-8.
   *         - UNESCAPED_CHARS if a string contains control characters that must be escaped
   *         - UNCLOSED_STRING if there is an unclosed string in the document.
   */
  simdjson_warn_unused simdjson_result<json_iterator> iterate_raw(const padded_string &json) & noexcept;

private:
  dom_parser_implementation dom_parser{};
  size_t _capacity{0};
  size_t _max_depth{0};
  std::unique_ptr<uint8_t[]> string_buf{};

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return The error, if there is one.
   */
  simdjson_warn_unused error_code allocate(size_t capacity, size_t max_depth=DEFAULT_MAX_DEPTH) noexcept;

  friend class json_iterator;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::parser> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::parser> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::parser &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::parser> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private
};

} // namespace simdjson
