#ifndef SIMDJSON_DOCUMENT_PARSER_H
#define SIMDJSON_DOCUMENT_PARSER_H

#include "simdjson/document.h"
#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/padded_string.h"
#include <string>

namespace simdjson {

/**
  * A persistent document parser.
  *
  * Use this if you intend to parse more than one document. It holds the internal memory necessary
  * to do parsing, as well as memory for a single document that is overwritten on each parse.
  *
  * This class cannot be copied, only moved, to avoid unintended allocations.
  *
  * @note This is not thread safe: one parser cannot produce two documents at the same time!
  */
class document::parser {
public:
  /**
  * Create a JSON parser with zero capacity. Call allocate_capacity() to initialize it.
  */
  parser()=default;
  ~parser()=default;

  /**
   * Take another parser's buffers and state.
   *
   * @param other The parser to take. Its capacity is zeroed.
   */
  parser(document::parser &&other) = default;
  parser(const document::parser &) = delete; // Disallow copying
  /**
   * Take another parser's buffers and state.
   *
   * @param other The parser to take. Its capacity is zeroed.
   */
  parser &operator=(document::parser &&other) = default;
  parser &operator=(const document::parser &) = delete; // Disallow copying

  /**
   * Parse a JSON document and return a reference to it.
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated. If realloc_if_needed is true,
   * it is assumed that the buffer does *not* have enough padding, and it is reallocated, enlarged
   * and copied before parsing.
   *
   * @param buf The JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes, unless
   *            realloc_if_needed is true.
   * @param len The length of the JSON.
   * @param realloc_if_needed Whether to reallocate and enlarge the JSON buffer to add padding.
   * @return the document, or an error if the JSON is invalid.
   */
  inline doc_ref_result parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) noexcept;

  /**
   * Parse a JSON document and return a reference to it.
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated. If realloc_if_needed is true,
   * it is assumed that the buffer does *not* have enough padding, and it is reallocated, enlarged
   * and copied before parsing.
   *
   * @param buf The JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes, unless
   *            realloc_if_needed is true.
   * @param len The length of the JSON.
   * @param realloc_if_needed Whether to reallocate and enlarge the JSON buffer to add padding.
   * @return the document, or an error if the JSON is invalid.
   */
  really_inline doc_ref_result parse(const char *buf, size_t len, bool realloc_if_needed = true) noexcept;

  /**
   * Parse a JSON document and return a reference to it.
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated. If `str.capacity() - str.size()
   * < SIMDJSON_PADDING`, the string will be copied to a string with larger capacity before parsing.
   *
   * @param s The JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes, or
   *          a new string will be created with the extra padding.
   * @return the document, or an error if the JSON is invalid.
   */
  really_inline doc_ref_result parse(const std::string &s) noexcept;

  /**
   * Parse a JSON document and return a reference to it.
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * @param s The JSON to parse.
   * @return the document, or an error if the JSON is invalid.
   */
  really_inline doc_ref_result parse(const padded_string &s) noexcept;

  // We do not want to allow implicit conversion from C string to std::string.
  really_inline doc_ref_result parse(const char *buf) noexcept = delete;

  /**
   * Parse a buffer containing many JSON documents.
   *
   *   document::parser parser;
   *   for (const document &doc : parser.parse_many(buf, len)) {
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
   *   document::parser parser;
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
   * If the parser is unallocated, it will be auto-allocated to batch_size. If it is already
   * allocated, it must have a capacity at least as large as batch_size.
   *
   * @param buf The concatenated JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes.
   * @param len The length of the concatenated JSON.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return The stream. If there is an error, it will be returned during iteration. An empty input
   *         will yield 0 documents rather than an EMPTY error. Errors:
   *         - MEMALLOC if the parser is unallocated and memory allocation fails
   *         - CAPACITY if the parser already has a capacity, and it is less than batch_size
   *         - other json errors if parsing fails.
   */
  inline stream parse_many(const uint8_t *buf, size_t len, size_t batch_size = 1000000) noexcept;

  /**
   * Parse a buffer containing many JSON documents.
   *
   *   document::parser parser;
   *   for (const document &doc : parser.parse_many(buf, len)) {
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
   *   document::parser parser;
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
   * If the parser is unallocated, it will be auto-allocated to batch_size. If it is already
   * allocated, it must have a capacity at least as large as batch_size.
   *
   * @param buf The concatenated JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes.
   * @param len The length of the concatenated JSON.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return The stream. If there is an error, it will be returned during iteration. An empty input
   *         will yield 0 documents rather than an EMPTY error. Errors:
   *         - MEMALLOC if the parser is unallocated and memory allocation fails
   *         - CAPACITY if the parser already has a capacity, and it is less than batch_size
   *         - other json errors if parsing fails
   */
  inline stream parse_many(const char *buf, size_t len, size_t batch_size = 1000000) noexcept;

  /**
   * Parse a buffer containing many JSON documents.
   *
   *   document::parser parser;
   *   for (const document &doc : parser.parse_many(buf, len)) {
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
   *   document::parser parser;
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
   * If the parser is unallocated, it will be auto-allocated to batch_size. If it is already
   * allocated, it must have a capacity at least as large as batch_size.
   *
   * @param s The concatenated JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return he stream. If there is an error, it will be returned during iteration. An empty input
   *         will yield 0 documents rather than an EMPTY error. Errors:
   *         - MEMALLOC if the parser is unallocated and memory allocation fails
   *         - CAPACITY if the parser already has a capacity, and it is less than batch_size
   *         - other json errors if parsing fails
   */
  inline stream parse_many(const std::string &s, size_t batch_size = 1000000) noexcept;

  /**
   * Parse a buffer containing many JSON documents.
   *
   *   document::parser parser;
   *   for (const document &doc : parser.parse_many(buf, len)) {
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
   *   document::parser parser;
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
   * If the parser is unallocated, it will be auto-allocated to batch_size. If it is already
   * allocated, it must have a capacity at least as large as batch_size.
   *
   * @param s The concatenated JSON to parse.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return he stream. If there is an error, it will be returned during iteration. An empty input
   *         will yield 0 documents rather than an EMPTY error. Errors:
   *         - MEMALLOC if the parser is unallocated and memory allocation fails
   *         - CAPACITY if the parser already has a capacity, and it is less than batch_size
   *         - other json errors if parsing fails
   */
  inline stream parse_many(const padded_string &s, size_t batch_size = 1000000) noexcept;

  // We do not want to allow implicit conversion from C string to std::string.
  really_inline doc_ref_result parse_many(const char *buf, size_t batch_size = 1000000) noexcept = delete;

  /**
   * Current capacity: the largest document this parser can support without reallocating.
   */
  really_inline size_t capacity() const noexcept;

  /**
   * The maximum level of nested object and arrays supported by this parser.
   */
  really_inline size_t max_depth() const noexcept;

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   */
  WARN_UNUSED inline bool allocate_capacity(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH);

  // type aliases for backcompat
  using Iterator = document::iterator;
  using InvalidJSON = simdjson_error;

  // Next location to write to in the tape
  uint32_t current_loc{0};

  // structural indices passed from stage 1 to stage 2
  uint32_t n_structural_indexes{0};
  std::unique_ptr<uint32_t[]> structural_indexes;

  // location and return address of each open { or [
  std::unique_ptr<uint32_t[]> containing_scope_offset;
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  std::unique_ptr<void*[]> ret_address;
#else
  std::unique_ptr<char[]> ret_address;
#endif

  // Next place to write a string
  uint8_t *current_string_buf_loc;

  bool valid{false};
  error_code error{UNINITIALIZED};

  // Document we're writing to
  document doc;

  //
  // TODO these are deprecated; use the results of parse instead.
  //

  // returns true if the document parsed was valid
  inline bool is_valid() const noexcept;

  // return an error code corresponding to the last parsing attempt, see
  // simdjson.h will return UNITIALIZED if no parsing was attempted
  inline int get_error_code() const noexcept;

  // return the string equivalent of "get_error_code"
  inline std::string get_error_message() const noexcept;

  // print the json to std::ostream (should be valid)
  // return false if the tape is likely wrong (e.g., you did not parse a valid
  // JSON).
  /** @deprecated Use cout << parser.parse() */
  inline bool print_json(std::ostream &os) const noexcept;
  inline bool dump_raw_tape(std::ostream &os) const noexcept;

  //
  // Parser callbacks: these are internal!
  //
  // TODO find a way to do this without exposing the interface or crippling performance
  //

  // this should be called when parsing (right before writing the tapes)
  inline void init_stage2() noexcept;
  really_inline error_code on_error(error_code new_error_code) noexcept;
  really_inline error_code on_success(error_code success_code) noexcept;
  really_inline bool on_start_document(uint32_t depth) noexcept;
  really_inline bool on_start_object(uint32_t depth) noexcept;
  really_inline bool on_start_array(uint32_t depth) noexcept;
  // TODO we're not checking this bool
  really_inline bool on_end_document(uint32_t depth) noexcept;
  really_inline bool on_end_object(uint32_t depth) noexcept;
  really_inline bool on_end_array(uint32_t depth) noexcept;
  really_inline bool on_true_atom() noexcept;
  really_inline bool on_false_atom() noexcept;
  really_inline bool on_null_atom() noexcept;
  really_inline uint8_t *on_start_string() noexcept;
  really_inline bool on_end_string(uint8_t *dst) noexcept;
  really_inline bool on_number_s64(int64_t value) noexcept;
  really_inline bool on_number_u64(uint64_t value) noexcept;
  really_inline bool on_number_double(double value) noexcept;
  //
  // Called before a parse is initiated.
  //
  // - Returns CAPACITY if the document is too large
  // - Returns MEMALLOC if we needed to allocate memory and could not
  //
  WARN_UNUSED inline error_code init_parse(size_t len) noexcept;

private:
  //
  // The maximum document length this parser supports.
  //
  // Buffers are large enough to handle any document up to this length.
  //
  size_t _capacity{0};

  //
  // The maximum depth (number of nested objects and arrays) supported by this parser.
  //
  // Defaults to DEFAULT_MAX_DEPTH.
  //
  size_t _max_depth{0};

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

  inline void write_tape(uint64_t val, tape_type t) noexcept;
  inline void annotate_previous_loc(uint32_t saved_loc, uint64_t val) noexcept;

  //
  // Set the current capacity: the largest document this parser can support without reallocating.
  //
  // This will allocate *or deallocate* as necessary.
  //
  // Returns false if allocation fails.
  //
  inline WARN_UNUSED bool set_capacity(size_t capacity);

  //
  // Set the maximum level of nested object and arrays supported by this parser.
  //
  // This will allocate *or deallocate* as necessary.
  //
  // Returns false if allocation fails.
  //
  inline WARN_UNUSED bool set_max_depth(size_t max_depth);

  // Used internally to get the document
  inline const document &get_document() const noexcept(false);

  template<size_t max_depth> friend class document_iterator;
}; // class parser

} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_PARSER_H