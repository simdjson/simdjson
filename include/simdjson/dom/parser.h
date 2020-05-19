#ifndef SIMDJSON_DOM_PARSER_H
#define SIMDJSON_DOM_PARSER_H

#include "simdjson/common_defs.h"
#include "simdjson/dom/document.h"
#include "simdjson/error.h"
#include "simdjson/internal/tape_ref.h"
#include "simdjson/minify.h"
#include "simdjson/padded_string.h"
#include "simdjson/portability.h"
#include <memory>
#include <ostream>
#include <string>

namespace simdjson {

namespace internal {

// expectation: sizeof(scope_descriptor) = 64/8.
struct scope_descriptor {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct scope_descriptor

#ifdef SIMDJSON_USE_COMPUTED_GOTO
typedef void* ret_address;
#else
typedef char ret_address;
#endif

} // namespace internal

namespace dom {

class document_stream;
class element;

/** The default batch size for parser.parse_many() and parser.load_many() */
static constexpr size_t DEFAULT_BATCH_SIZE = 1000000;

/**
  * A persistent document parser.
  *
  * The parser is designed to be reused, holding the internal buffers necessary to do parsing,
  * as well as memory for a single document. The parsed document is overwritten on each parse.
  *
  * This class cannot be copied, only moved, to avoid unintended allocations.
  *
  * @note This is not thread safe: one parser cannot produce two documents at the same time!
  */
class parser {
public:
  /**
  * Create a JSON parser.
  *
  * The new parser will have zero capacity.
  *
  * @param max_capacity The maximum document length the parser can automatically handle. The parser
  *    will allocate more capacity on an as needed basis (when it sees documents too big to handle)
  *    up to this amount. The parser still starts with zero capacity no matter what this number is:
  *    to allocate an initial capacity, call allocate() after constructing the parser.
  *    Defaults to SIMDJSON_MAXSIZE_BYTES (the largest single document simdjson can process).
  */
  really_inline explicit parser(size_t max_capacity = SIMDJSON_MAXSIZE_BYTES) noexcept;
  /**
   * Take another parser's buffers and state.
   *
   * @param other The parser to take. Its capacity is zeroed.
   */
  parser(parser &&other) = default;
  parser(const parser &) = delete; ///< @private Disallow copying
  /**
   * Take another parser's buffers and state.
   *
   * @param other The parser to take. Its capacity is zeroed.
   */
  parser &operator=(parser &&other) = default;
  parser &operator=(const parser &) = delete; ///< @private Disallow copying

  /** Deallocate the JSON parser. */
  ~parser()=default;

  /**
   * Load a JSON document from a file and return a reference to it.
   *
   *   dom::parser parser;
   *   const element doc = parser.load("jsonexamples/twitter.json");
   *
   * ### IMPORTANT: Document Lifetime
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than the file length, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param path The path to load.
   * @return The document, or an error:
   *         - IO_ERROR if there was an error opening or reading the file.
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails.
   */
  inline simdjson_result<element> load(const std::string &path) & noexcept;
  inline simdjson_result<element> load(const std::string &path) &&  = delete ;
  /**
   * Parse a JSON document and return a temporary reference to it.
   *
   *   dom::parser parser;
   *   element doc = parser.parse(buf, len);
   *
   * ### IMPORTANT: Document Lifetime
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated.
   *
   * If realloc_if_needed is true, it is assumed that the buffer does *not* have enough padding,
   * and it is copied into an enlarged temporary buffer before parsing.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than len, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param buf The JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes, unless
   *            realloc_if_needed is true.
   * @param len The length of the JSON.
   * @param realloc_if_needed Whether to reallocate and enlarge the JSON buffer to add padding.
   * @return The document, or an error:
   *         - MEMALLOC if realloc_if_needed is true or the parser does not have enough capacity,
   *           and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails.
   */
  inline simdjson_result<element> parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  inline simdjson_result<element> parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  really_inline simdjson_result<element> parse(const char *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  really_inline simdjson_result<element> parse(const char *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  really_inline simdjson_result<element> parse(const std::string &s) & noexcept;
  really_inline simdjson_result<element> parse(const std::string &s) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  really_inline simdjson_result<element> parse(const padded_string &s) & noexcept;
  really_inline simdjson_result<element> parse(const padded_string &s) && =delete;

  /** @private We do not want to allow implicit conversion from C string to std::string. */
  really_inline simdjson_result<element> parse(const char *buf) noexcept = delete;

  /**
   * Load a file containing many JSON documents.
   *
   *   dom::parser parser;
   *   for (const element doc : parser.load_many(path)) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * ### Format
   *
   * The file must contain a series of one or more JSON documents, concatenated into a single
   * buffer, separated by whitespace. It effectively parses until it has a fully valid document,
   * then starts parsing the next document at that point. (It does this with more parallelism and
   * lookahead than you might think, though.)
   *
   * documents that consist of an object or array may omit the whitespace between them, concatenating
   * with no separator. documents that consist of a single primitive (i.e. documents that are not
   * arrays or objects) MUST be separated with whitespace.
   *
   * ### Error Handling
   *
   * All errors are returned during iteration: if there is a global error such as memory allocation,
   * it will be yielded as the first result. Iteration always stops after the first error.
   *
   * As with all other simdjson methods, non-exception error handling is readily available through
   * the same interface, requiring you to check the error before using the document:
   *
   *   dom::parser parser;
   *   for (auto [doc, error] : parser.load_many(path)) {
   *     if (error) { cerr << error << endl; exit(1); }
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * ### Threads
   *
   * When compiled with SIMDJSON_THREADS_ENABLED, this method will use a single thread under the
   * hood to do some lookahead.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than batch_size, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param s The concatenated JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return The stream. If there is an error, it will be returned during iteration. An empty input
   *         will yield 0 documents rather than an EMPTY error. Errors:
   *         - IO_ERROR if there was an error opening or reading the file.
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and batch_size > max_capacity.
   *         - other json errors if parsing fails.
   */
  inline document_stream load_many(const std::string &path, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;

  /**
   * Parse a buffer containing many JSON documents.
   *
   *   dom::parser parser;
   *   for (const element doc : parser.parse_many(buf, len)) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * ### Format
   *
   * The buffer must contain a series of one or more JSON documents, concatenated into a single
   * buffer, separated by whitespace. It effectively parses until it has a fully valid document,
   * then starts parsing the next document at that point. (It does this with more parallelism and
   * lookahead than you might think, though.)
   *
   * documents that consist of an object or array may omit the whitespace between them, concatenating
   * with no separator. documents that consist of a single primitive (i.e. documents that are not
   * arrays or objects) MUST be separated with whitespace.
   *
   * ### Error Handling
   *
   * All errors are returned during iteration: if there is a global error such as memory allocation,
   * it will be yielded as the first result. Iteration always stops after the first error.
   *
   * As with all other simdjson methods, non-exception error handling is readily available through
   * the same interface, requiring you to check the error before using the document:
   *
   *   dom::parser parser;
   *   for (auto [doc, error] : parser.parse_many(buf, len)) {
   *     if (error) { cerr << error << endl; exit(1); }
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated.
   *
   * ### Threads
   *
   * When compiled with SIMDJSON_THREADS_ENABLED, this method will use a single thread under the
   * hood to do some lookahead.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than batch_size, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param buf The concatenated JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes.
   * @param len The length of the concatenated JSON.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return The stream. If there is an error, it will be returned during iteration. An empty input
   *         will yield 0 documents rather than an EMPTY error. Errors:
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails
   *         - CAPACITY if the parser does not have enough capacity and batch_size > max_capacity.
   *         - other json errors if parsing fails.
   */
  inline document_stream parse_many(const uint8_t *buf, size_t len, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline document_stream parse_many(const char *buf, size_t len, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline document_stream parse_many(const std::string &s, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline document_stream parse_many(const padded_string &s, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;

  /** @private We do not want to allow implicit conversion from C string to std::string. */
  really_inline simdjson_result<element> parse_many(const char *buf, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept = delete;

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return The error, if there is one.
   */
  WARN_UNUSED inline error_code allocate(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH) noexcept;

  /**
   * @private deprecated because it returns bool instead of error_code, which is our standard for
   * failures. Use allocate() instead.
   *
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return true if successful, false if allocation failed.
   */
  [[deprecated("Use allocate() instead.")]]
  WARN_UNUSED inline bool allocate_capacity(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH) noexcept;

  /**
   * The largest document this parser can support without reallocating.
   *
   * @return Current capacity, in bytes.
   */
  really_inline size_t capacity() const noexcept;

  /**
   * The largest document this parser can automatically support.
   *
   * The parser may reallocate internal buffers as needed up to this amount.
   *
   * @return Maximum capacity, in bytes.
   */
  really_inline size_t max_capacity() const noexcept;

  /**
   * The maximum level of nested object and arrays supported by this parser.
   *
   * @return Maximum depth, in bytes.
   */
  really_inline size_t max_depth() const noexcept;

  /**
   * Set max_capacity. This is the largest document this parser can automatically support.
   *
   * The parser may reallocate internal buffers as needed up to this amount.
   *
   * This call will not allocate or deallocate, even if capacity is currently above max_capacity.
   *
   * @param max_capacity The new maximum capacity, in bytes.
   */
  really_inline void set_max_capacity(size_t max_capacity) noexcept;

  /** @private Use the new DOM API instead */
  class Iterator;
  /** @private Use simdjson_error instead */
  using InvalidJSON [[deprecated("Use simdjson_error instead")]] = simdjson_error;

  /** @private Next location to write to in the tape */
  uint32_t current_loc{0};

  /** @private Number of structural indices passed from stage 1 to stage 2 */
  uint32_t n_structural_indexes{0};
  /** @private Structural indices passed from stage 1 to stage 2 */
  std::unique_ptr<uint32_t[]> structural_indexes{};

  /** @private Tape location of each open { or [ */
  std::unique_ptr<internal::scope_descriptor[]> containing_scope{};

  /** @private Return address of each open { or [ */
  std::unique_ptr<internal::ret_address[]> ret_address{};

  /** @private Use `if (parser.parse(...).error())` instead */
  bool valid{false};
  /** @private Use `parser.parse(...).error()` instead */
  error_code error{UNINITIALIZED};

  /** @private Use `parser.parse(...).value()` instead */
  document doc{};

  /** @private returns true if the document parsed was valid */
  [[deprecated("Use the result of parser.parse() instead")]]
  inline bool is_valid() const noexcept;

  /**
   * @private return an error code corresponding to the last parsing attempt, see
   * simdjson.h will return UNITIALIZED if no parsing was attempted
   */
  [[deprecated("Use the result of parser.parse() instead")]]
  inline int get_error_code() const noexcept;

  /** @private return the string equivalent of "get_error_code" */
  [[deprecated("Use error_message() on the result of parser.parse() instead, or cout << error")]]
  inline std::string get_error_message() const noexcept;

  /** @private */
  [[deprecated("Use cout << on the result of parser.parse() instead")]]
  inline bool print_json(std::ostream &os) const noexcept;

  /** @private Private and deprecated: use `parser.parse(...).doc.dump_raw_tape()` instead */
  inline bool dump_raw_tape(std::ostream &os) const noexcept;

private:
  /**
   * The maximum document length this parser will automatically support.
   *
   * The parser will not be automatically allocated above this amount.
   */
  size_t _max_capacity;

  /**
   * The maximum document length this parser supports.
   *
   * Buffers are large enough to handle any document up to this length.
   */
  size_t _capacity{0};

  /**
   * The maximum depth (number of nested objects and arrays) supported by this parser.
   *
   * Defaults to DEFAULT_MAX_DEPTH.
   */
  size_t _max_depth{0};

  /**
   * The loaded buffer (reused each time load() is called)
   */
  std::unique_ptr<char[], decltype(&aligned_free_char)> loaded_bytes;

  /** Capacity of loaded_bytes buffer. */
  size_t _loaded_bytes_capacity{0};

  // all nodes are stored on the doc.tape using a 64-bit word.
  //
  // strings, double and ints are stored as
  //  a 64-bit word with a pointer to the actual value
  //
  //
  //
  // for objects or arrays, store [ or {  at the beginning and } and ] at the
  // end. For the openings ([ or {), we annotate them with a reference to the
  // location on the doc.tape of the end, and for then closings (} and ]), we
  // annotate them with a reference to the location of the opening
  //
  //

  /**
   * Ensure we have enough capacity to handle at least desired_capacity bytes,
   * and auto-allocate if not.
   */
  inline error_code ensure_capacity(size_t desired_capacity) noexcept;

  /** Read the file into loaded_bytes */
  inline simdjson_result<size_t> read_file(const std::string &path) noexcept;

  friend class parser::Iterator;
  friend class document_stream;
}; // class parser

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_DOM_PARSER_H
