#ifndef SIMDJSON_DOCUMENT_H
#define SIMDJSON_DOCUMENT_H

#include <cstring>
#include <memory>
#include <string>
#include <limits>
#include <sstream>
#include "simdjson/common_defs.h"
#include "simdjson/simdjson.h"
#include "simdjson/padded_string.h"

namespace simdjson::dom {

class parser;
class element;
class array;
class object;
class key_value_pair;
class document;
class document_stream;

/** The default batch size for parser.parse_many() and parser.load_many() */
static constexpr size_t DEFAULT_BATCH_SIZE = 1000000;

} // namespace simdjson::dom

namespace simdjson {

template<> struct simdjson_result<dom::element>;
template<> struct simdjson_result<dom::array>;
template<> struct simdjson_result<dom::object>;

template<typename T>
class minify;

} // namespace simdjson

namespace simdjson::internal {

using namespace simdjson::dom;

constexpr const uint64_t JSON_VALUE_MASK = 0x00FFFFFFFFFFFFFF;
enum class tape_type;
class tape_ref;
  /**
    * The possible types in the tape. Internal only.
    */
  enum class tape_type {
    ROOT = 'r',
    START_ARRAY = '[',
    START_OBJECT = '{',
    END_ARRAY = ']',
    END_OBJECT = '}',
    STRING = '"',
    INT64 = 'l',
    UINT64 = 'u',
    DOUBLE = 'd',
    TRUE_VALUE = 't',
    FALSE_VALUE = 'f',
    NULL_VALUE = 'n'
  };

  /**
  * A reference to an element on the tape. Internal only.
  */
  class tape_ref {
  public:
    really_inline tape_ref() noexcept;
    really_inline tape_ref(const document *doc, size_t json_index) noexcept;
    inline size_t after_element() const noexcept;
    really_inline tape_type type() const noexcept;
    really_inline uint64_t tape_value() const noexcept;
    template<typename T>
    really_inline T next_tape_value() const noexcept;
    inline std::string_view get_string_view() const noexcept;

    /** The document this element references. */
    const document *doc;

    /** The index of this element on `doc.tape[]` */
    size_t json_index;
  };
} // namespace simdjson::internal

namespace simdjson::dom {

/**
 * JSON array.
 */
class array : protected internal::tape_ref {
public:
  /** Create a new, invalid array */
  really_inline array() noexcept;

  class iterator : protected internal::tape_ref {
  public:
    /**
     * Get the actual value
     */
    inline element operator*() const noexcept;
    /**
     * Get the next value.
     *
     * Part of the std::iterator interface.
     */
    inline void operator++() noexcept;
    /**
     * Check if these values come from the same place in the JSON.
     *
     * Part of the std::iterator interface.
     */
    inline bool operator!=(const iterator& other) const noexcept;
  private:
    really_inline iterator(const document *doc, size_t json_index) noexcept;
    friend class array;
  };

  /**
   * Return the first array element.
   *
   * Part of the std::iterable interface.
   */
  inline iterator begin() const noexcept;
  /**
   * One past the last array element.
   *
   * Part of the std::iterable interface.
   */
  inline iterator end() const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   dom::parser parser;
   *   array a = parser.parse(R"([ { "foo": { "a": [ 10, 20, 30 ] }} ])");
   *   a.at("0/foo/a/1") == 20
   *   a.at("0")["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at(const std::string_view &json_pointer) const noexcept;

  /**
   * Get the value at the given index.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  inline simdjson_result<element> at(size_t index) const noexcept;

private:
  really_inline array(const document *doc, size_t json_index) noexcept;
  friend class element;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::minify;
};

/**
 * JSON object.
 */
class object : protected internal::tape_ref {
public:
  /** Create a new, invalid object */
  really_inline object() noexcept;

  class iterator : protected internal::tape_ref {
  public:
    /**
     * Get the actual key/value pair
     */
    inline const key_value_pair operator*() const noexcept;
    /**
     * Get the next key/value pair.
     *
     * Part of the std::iterator interface.
     */
    inline void operator++() noexcept;
    /**
     * Check if these key value pairs come from the same place in the JSON.
     *
     * Part of the std::iterator interface.
     */
    inline bool operator!=(const iterator& other) const noexcept;
    /**
     * Get the key of this key/value pair.
     */
    inline std::string_view key() const noexcept;
    /**
     * Get the key of this key/value pair.
     */
    inline const char *key_c_str() const noexcept;
    /**
     * Get the value of this key/value pair.
     */
    inline element value() const noexcept;
  private:
    really_inline iterator(const document *doc, size_t json_index) noexcept;
    friend class object;
  };

  /**
   * Return the first key/value pair.
   *
   * Part of the std::iterable interface.
   */
  inline iterator begin() const noexcept;
  /**
   * One past the last key/value pair.
   *
   * Part of the std::iterable interface.
   */
  inline iterator end() const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].get<uint64_t>().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].get<uint64_t>().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const std::string_view &key) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].get<uint64_t>().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].get<uint64_t>().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const char *key) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   dom::parser parser;
   *   object obj = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   obj.at("foo/a/1") == 20
   *   obj.at("foo")["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at(const std::string_view &json_pointer) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].get<uint64_t>().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].get<uint64_t>().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key(const std::string_view &key) const noexcept;

  /**
   * Get the value associated with the given key in a case-insensitive manner.
   *
   * Note: The key will be matched against **unescaped** JSON.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key_case_insensitive(const std::string_view &key) const noexcept;

private:
  really_inline object(const document *doc, size_t json_index) noexcept;
  friend class element;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::minify;
};

/**
 * A parsed JSON document.
 *
 * This class cannot be copied, only moved, to avoid unintended allocations.
 */
class document {
public:
  /**
   * Create a document container with zero capacity.
   *
   * The parser will allocate capacity as needed.
   */
  document() noexcept = default;
  ~document() noexcept = default;

  /**
   * Take another document's buffers.
   *
   * @param other The document to take. Its capacity is zeroed and it is invalidated.
   */
  document(document &&other) noexcept = default;
  /** @private */
  document(const document &) = delete; // Disallow copying
  /**
   * Take another document's buffers.
   *
   * @param other The document to take. Its capacity is zeroed.
   */
  document &operator=(document &&other) noexcept = default;
  /** @private */
  document &operator=(const document &) = delete; // Disallow copying

  /**
   * Get the root element of this document as a JSON array.
   */
  element root() const noexcept;

  /**
   * @private Dump the raw tape for debugging.
   *
   * @param os the stream to output to.
   * @return false if the tape is likely wrong (e.g., you did not parse a valid JSON).
   */
  bool dump_raw_tape(std::ostream &os) const noexcept;

  /** @private Structural values. */
  std::unique_ptr<uint64_t[]> tape;

  /** @private String values.
   *
   * Should be at least byte_capacity.
   */
  std::unique_ptr<uint8_t[]> string_buf;

private:
  inline error_code allocate(size_t len) noexcept;
  template<typename T>
  friend class simdjson::minify;
  friend class parser;
}; // class document

/**
 * A JSON element.
 *
 * References an element in a JSON document, representing a JSON null, boolean, string, number,
 * array or object.
 */
class element : protected internal::tape_ref {
public:
  /** Create a new, invalid element. */
  really_inline element() noexcept;

  /** Whether this element is a json `null`. */
  really_inline bool is_null() const noexcept;

  /**
   * Tell whether the value can be cast to the given primitive type.
   *
   * Supported types:
   * - Boolean: bool
   * - Number: double, uint64_t, int64_t
   * - String: std::string_view, const char *
   * - Array: array
   */
  template<typename T>
  really_inline bool is() const noexcept;

  /**
   * Get the value as the given primitive type.
   *
   * Supported types:
   * - Boolean: bool
   * - Number: double, uint64_t, int64_t
   * - String: std::string_view, const char *
   * - Array: array
   *
   * @returns The value cast to the given type, or:
   *          INCORRECT_TYPE if the value cannot be cast to the given type.
   */
  template<typename T>
  really_inline simdjson_result<T> get() const noexcept;

#if SIMDJSON_EXCEPTIONS
  /**
   * Read this element as a boolean.
   *
   * @return The boolean value
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a boolean.
   */
  inline operator bool() const noexcept(false);

  /**
   * Read this element as a null-terminated string.
   *
   * Does *not* convert other types to a string; requires that the JSON type of the element was
   * an actual string.
   *
   * @return The string value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a string.
   */
  inline explicit operator const char*() const noexcept(false);

  /**
   * Read this element as a null-terminated string.
   *
   * Does *not* convert other types to a string; requires that the JSON type of the element was
   * an actual string.
   *
   * @return The string value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a string.
   */
  inline operator std::string_view() const noexcept(false);

  /**
   * Read this element as an unsigned integer.
   *
   * @return The integer value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an integer
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer doesn't fit in 64 bits or is negative
   */
  inline operator uint64_t() const noexcept(false);
  /**
   * Read this element as an signed integer.
   *
   * @return The integer value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an integer
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer doesn't fit in 64 bits
   */
  inline operator int64_t() const noexcept(false);
  /**
   * Read this element as an double.
   *
   * @return The double value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a number
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer doesn't fit in 64 bits or is negative
   */
  inline operator double() const noexcept(false);
  /**
   * Read this element as a JSON array.
   *
   * @return The JSON array.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an array
   */
  inline operator array() const noexcept(false);
  /**
   * Read this element as a JSON object (key/value pairs).
   *
   * @return The JSON object.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an object
   */
  inline operator object() const noexcept(false);

  /**
   * Iterate over each element in this array.
   *
   * @return The beginning of the iteration.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an array
   */
  inline dom::array::iterator begin() const noexcept(false);

  /**
   * Iterate over each element in this array.
   *
   * @return The end of the iteration.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an array
   */
  inline dom::array::iterator end() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].get<uint64_t>().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].get<uint64_t>().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const std::string_view &key) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].get<uint64_t>().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].get<uint64_t>().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const char *key) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   dom::parser parser;
   *   element doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   doc.at("/foo/a/1") == 20
   *   doc.at("/")["foo"]["a"].at(1) == 20
   *   doc.at("")["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at(const std::string_view &json_pointer) const noexcept;

  /**
   * Get the value at the given index.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  inline simdjson_result<element> at(size_t index) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].get<uint64_t>().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].get<uint64_t>().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key(const std::string_view &key) const noexcept;

  /**
   * Get the value associated with the given key in a case-insensitive manner.
   *
   * Note: The key will be matched against **unescaped** JSON.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key_case_insensitive(const std::string_view &key) const noexcept;

  /** @private for debugging. Prints out the root element. */
  inline bool dump_raw_tape(std::ostream &out) const noexcept;

private:
  really_inline element(const document *doc, size_t json_index) noexcept;
  friend class document;
  friend class object;
  friend class array;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::minify;
};

/**
 * Key/value pair in an object.
 */
class key_value_pair {
public:
  std::string_view key;
  element value;

private:
  really_inline key_value_pair(const std::string_view &_key, element _value) noexcept;
  friend class object;
};

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
  really_inline parser(size_t max_capacity = SIMDJSON_MAXSIZE_BYTES) noexcept;

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
  inline simdjson_result<element> load(const std::string &path) noexcept; 

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
  inline simdjson_result<element> parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) noexcept;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  really_inline simdjson_result<element> parse(const char *buf, size_t len, bool realloc_if_needed = true) noexcept;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  really_inline simdjson_result<element> parse(const std::string &s) noexcept;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  really_inline simdjson_result<element> parse(const padded_string &s) noexcept;

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
  std::unique_ptr<uint32_t[]> structural_indexes;

  /** @private Tape location of each open { or [ */
  std::unique_ptr<uint32_t[]> containing_scope_offset;
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  /** @private Return address of each open { or [ */
  std::unique_ptr<void*[]> ret_address;
#else
  /** @private Return address of each open { or [ */
  std::unique_ptr<char[]> ret_address;
#endif

  /** @private Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc;

  /** @private Use `if (parser.parse(...).error())` instead */
  bool valid{false};
  /** @private Use `parser.parse(...).error()` instead */
  error_code error{UNINITIALIZED};

  /** @private Use `parser.parse(...).value()` instead */
  document doc;

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

  //
  // Parser callbacks: these are internal!
  //

  /** @private this should be called when parsing (right before writing the tapes) */
  inline void init_stage2() noexcept;
  really_inline error_code on_error(error_code new_error_code) noexcept; ///< @private
  really_inline error_code on_success(error_code success_code) noexcept; ///< @private
  really_inline bool on_start_document(uint32_t depth) noexcept; ///< @private
  really_inline bool on_start_object(uint32_t depth) noexcept; ///< @private
  really_inline bool on_start_array(uint32_t depth) noexcept; ///< @private
  // TODO we're not checking this bool
  really_inline bool on_end_document(uint32_t depth) noexcept; ///< @private
  really_inline bool on_end_object(uint32_t depth) noexcept; ///< @private
  really_inline bool on_end_array(uint32_t depth) noexcept; ///< @private
  really_inline bool on_true_atom() noexcept; ///< @private
  really_inline bool on_false_atom() noexcept; ///< @private
  really_inline bool on_null_atom() noexcept; ///< @private
  really_inline uint8_t *on_start_string() noexcept; ///< @private
  really_inline bool on_end_string(uint8_t *dst) noexcept; ///< @private
  really_inline bool on_number_s64(int64_t value) noexcept; ///< @private
  really_inline bool on_number_u64(uint64_t value) noexcept; ///< @private
  really_inline bool on_number_double(double value) noexcept; ///< @private

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

  inline void write_tape(uint64_t val, internal::tape_type t) noexcept;
  inline void annotate_previous_loc(uint32_t saved_loc, uint64_t val) noexcept;

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

} // namespace simdjson::dom

namespace simdjson {

/**
 * Minifies a JSON element or document, printing the smallest possible valid JSON.
 *
 *   dom::parser parser;
 *   element doc = parser.parse("   [ 1 , 2 , 3 ] "_padded);
 *   cout << minify(doc) << endl; // prints [1,2,3]
 *
 */
template<typename T>
class minify {
public:
  /**
   * Create a new minifier.
   *
   * @param _value The document or element to minify.
   */
  inline minify(const T &_value) noexcept : value{_value} {}

  /**
   * Minify JSON to a string.
   */
  inline operator std::string() const noexcept { std::stringstream s; s << *this; return s.str(); }

  /**
   * Minify JSON to an output stream.
   */
  inline std::ostream& print(std::ostream& out);
private:
  const T &value;
};

/**
 * Minify JSON to an output stream.
 *
 * @param out The output stream.
 * @param formatter The minifier.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
template<typename T>
inline std::ostream& operator<<(std::ostream& out, minify<T> formatter) { return formatter.print(out); }

namespace dom {

// << operators need to be in the same namespace as the class being output, so C++ can find them
// automatically

/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const element &value) { return out << minify(value); };
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const array &value) { return out << minify(value); }
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const object &value) { return out << minify(value); }
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const key_value_pair &value) { return out << minify(value); }

} // namespace dom

#if SIMDJSON_EXCEPTIONS

/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, const simdjson_result<dom::element> &value) noexcept(false) { return out << minify(value); }
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, const simdjson_result<dom::array> &value) noexcept(false) { return out << minify(value); }
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, const simdjson_result<dom::object> &value) noexcept(false) { return out << minify(value); }

#endif

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<dom::element> : public internal::simdjson_result_base<dom::element> {
public:
  really_inline simdjson_result() noexcept; ///< @private
  really_inline simdjson_result(dom::element &&value) noexcept; ///< @private
  really_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<bool> is_null() const noexcept;
  template<typename T>
  inline simdjson_result<bool> is() const noexcept;
  template<typename T>
  inline simdjson_result<T> get() const noexcept;

  inline simdjson_result<dom::element> operator[](const std::string_view &key) const noexcept;
  inline simdjson_result<dom::element> operator[](const char *key) const noexcept;
  inline simdjson_result<dom::element> at(const std::string_view &json_pointer) const noexcept;
  inline simdjson_result<dom::element> at(size_t index) const noexcept;
  inline simdjson_result<dom::element> at_key(const std::string_view &key) const noexcept;
  inline simdjson_result<dom::element> at_key_case_insensitive(const std::string_view &key) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline operator bool() const noexcept(false);
  inline explicit operator const char*() const noexcept(false);
  inline operator std::string_view() const noexcept(false);
  inline operator uint64_t() const noexcept(false);
  inline operator int64_t() const noexcept(false);
  inline operator double() const noexcept(false);
  inline operator dom::array() const noexcept(false);
  inline operator dom::object() const noexcept(false);

  inline dom::array::iterator begin() const noexcept(false);
  inline dom::array::iterator end() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

/** The result of a JSON conversion that may fail. */
template<>
struct simdjson_result<dom::array> : public internal::simdjson_result_base<dom::array> {
public:
  really_inline simdjson_result() noexcept; ///< @private
  really_inline simdjson_result(dom::array value) noexcept; ///< @private
  really_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<dom::element> at(const std::string_view &json_pointer) const noexcept;
  inline simdjson_result<dom::element> at(size_t index) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline dom::array::iterator begin() const noexcept(false);
  inline dom::array::iterator end() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

/** The result of a JSON conversion that may fail. */
template<>
struct simdjson_result<dom::object> : public internal::simdjson_result_base<dom::object> {
public:
  really_inline simdjson_result() noexcept; ///< @private
  really_inline simdjson_result(dom::object value) noexcept; ///< @private
  really_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<dom::element> operator[](const std::string_view &key) const noexcept;
  inline simdjson_result<dom::element> operator[](const char *key) const noexcept;
  inline simdjson_result<dom::element> at(const std::string_view &json_pointer) const noexcept;
  inline simdjson_result<dom::element> at_key(const std::string_view &key) const noexcept;
  inline simdjson_result<dom::element> at_key_case_insensitive(const std::string_view &key) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline dom::object::iterator begin() const noexcept(false);
  inline dom::object::iterator end() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_H