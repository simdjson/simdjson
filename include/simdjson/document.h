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

namespace simdjson::internal {
constexpr const uint64_t JSON_VALUE_MASK = 0x00FFFFFFFFFFFFFF;
enum class tape_type;
class tape_ref;
} // namespace simdjson::internal

namespace simdjson {

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
  document() noexcept=default;
  ~document() noexcept=default;

  /**
   * Take another document's buffers.
   *
   * @param other The document to take. Its capacity is zeroed and it is invalidated.
   */
  document(document &&other) noexcept = default;
  document(const document &) = delete; // Disallow copying
  /**
   * Take another document's buffers.
   *
   * @param other The document to take. Its capacity is zeroed.
   */
  document &operator=(document &&other) noexcept = default;
  document &operator=(const document &) = delete; // Disallow copying

  /** The default batch size for parse_many and load_many */
  static constexpr size_t DEFAULT_BATCH_SIZE = 1000000;

  // Nested classes
  class element;
  class array;
  class object;
  class key_value_pair;
  class parser;
  class stream;

  class doc_result;
  class element_result;
  class array_result;
  class object_result;
  class stream_result;

  /**
   * Get the root element of this document as a JSON array.
   */
  element root() const noexcept;
  /**
   * Get the root element of this document as a JSON array.
   */
  array_result as_array() const noexcept;
  /**
   * Get the root element of this document as a JSON object.
   */
  object_result as_object() const noexcept;
  /**
   * Get the root element of this document.
   */
  operator element() const noexcept;

#if SIMDJSON_EXCEPTIONS
  /**
   * Read the root element of this document as a JSON array.
   *
   * @return The JSON array.
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not an array
   */
  operator array() const noexcept(false);
  /**
   * Read this element as a JSON object (key/value pairs).
   *
   * @return The JSON object.
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not an object
   */
  operator object() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

  /**
   * Dump the raw tape for debugging.
   *
   * @param os the stream to output to.
   * @return false if the tape is likely wrong (e.g., you did not parse a valid JSON).
   */
  bool dump_raw_tape(std::ostream &os) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document &doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   doc["/foo/a/1"] == 20
   *   doc["/"]["foo"]["a"].at(1) == 20
   *   doc[""]["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](std::string_view json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document &doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   doc["/foo/a/1"] == 20
   *   doc["/"]["foo"]["a"].at(1) == 20
   *   doc[""]["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](const char *json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document &doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
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
  inline element_result at(std::string_view json_pointer) const noexcept;

  /**
   * Get the value at the given index.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  inline element_result at(size_t index) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   document::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].as_uint64_t().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].as_uint64_t().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key(std::string_view s) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * Note: The key will be matched against **unescaped** JSON:
   *
   *   document::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].as_uint64_t().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].as_uint64_t().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key(const char *s) const noexcept;

  std::unique_ptr<uint64_t[]> tape;
  std::unique_ptr<uint8_t[]> string_buf;// should be at least byte_capacity

private:
  inline error_code set_capacity(size_t len) noexcept;
  template<typename T>
  friend class minify;
}; // class document

template<typename T>
class minify;

/**
 * A parsed document reference, or an error if the parse failed.
 *
 *     document::parser parser;
 *     document &doc = parser.parse(json);
 *
 * ## Document Ownership
 *
 * The `document &` refers to an internal document the parser reuses on each `parse()` call. It will
 * become invalidated on the next `parse()`.
 *
 * This is more efficient for common cases where documents are parsed and used one at a time. If you
 * need to keep the document around longer, you may *take* it from the parser by casting it:
 *
 *     document &doc = parser.parse(); // take ownership
 *
 * If you do this, the parser will automatically allocate a new document on the next `parse()` call.
 *
 * ## Error Codes vs. Exceptions
 *
 * This result type allows the user to pick whether to use exceptions or not.
 *
 * Use like this to avoid exceptions:
 *
 *     auto [doc, error] = parser.parse(json);
 *     if (error) { exit(1); }
 *
 * Use like this if you'd prefer to use exceptions:
 *
 *     document &doc = parser.parse(json);
 *
 */
class document::doc_result : public simdjson_result<document&> {
public:
  /**
   * Read this document as a JSON objec.
   *
   * @return The object value, or:
   *         - UNEXPECTED_TYPE if the JSON document is not an object
   */
  inline object_result as_object() const noexcept;

  /**
   * Read this document as a JSON array.
   *
   * @return The array value, or:
   *         - UNEXPECTED_TYPE if the JSON document is not an array
   */
  inline array_result as_array() const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document &doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   doc["/foo/a/1"] == 20
   *   doc["/"]["foo"]["a"].at(1) == 20
   *   doc[""]["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](std::string_view json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document &doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   doc["/foo/a/1"] == 20
   *   doc["/"]["foo"]["a"].at(1) == 20
   *   doc[""]["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](const char *json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document &doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
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
  inline element_result at(std::string_view json_pointer) const noexcept;

  /**
   * Get the value at the given index.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  inline element_result at(size_t index) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   document::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].as_uint64_t().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].as_uint64_t().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key(std::string_view s) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * Note: The key will be matched against **unescaped** JSON:
   *
   *   document::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].as_uint64_t().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].as_uint64_t().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key(const char *s) const noexcept;

  ~doc_result()=default;
  doc_result(document &doc, error_code error) noexcept;
  friend class document::parser;
  friend class document::stream;
}; // class document::doc_result

namespace internal {
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
  protected:
    really_inline tape_ref() noexcept;
    really_inline tape_ref(const document *_doc, size_t _json_index) noexcept;
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

    friend class simdjson::document::key_value_pair;
    template<typename T>
    friend class simdjson::minify;
  };
} // namespace simdjson::internal

/**
 * A JSON element.
 *
 * References an element in a JSON document, representing a JSON null, boolean, string, number,
 * array or object.
 */
class document::element : protected internal::tape_ref {
public:
  /** Create a new, invalid element. */
  really_inline element() noexcept;

  /** Whether this element is a json `null`. */
  really_inline bool is_null() const noexcept;
  /** Whether this is a JSON `true` or `false` */
  really_inline bool is_bool() const noexcept;
  /** Whether this is a JSON number (e.g. 1, 1.0 or 1e2) */
  really_inline bool is_number() const noexcept;
  /** Whether this is a JSON integer (e.g. 1 or -1, but *not* 1.0 or 1e2) */
  really_inline bool is_integer() const noexcept;
  /** Whether this is a JSON integer in [9223372036854775808, 18446744073709551616)
   * that is, a value too large for a signed 64-bit integer, but that still fits
   * in a 64-bit word. Note that is_integer() is true when is_unsigned_integer()
   * is true.*/
  really_inline bool is_unsigned_integer() const noexcept;
  /** Whether this is a JSON number but not an integer */
  really_inline bool is_float() const noexcept;
  /** Whether this is a JSON string (e.g. "abc") */
  really_inline bool is_string() const noexcept;
  /** Whether this is a JSON array (e.g. []) */
  really_inline bool is_array() const noexcept;
  /** Whether this is a JSON array (e.g. []) */
  really_inline bool is_object() const noexcept;

  /**
   * Read this element as a boolean (json `true` or `false`).
   *
   * @return The boolean value, or:
   *         - UNEXPECTED_TYPE error if the JSON element is not a boolean
   */
  inline simdjson_result<bool> as_bool() const noexcept;

  /**
   * Read this element as a null-terminated string.
   *
   * Does *not* convert other types to a string; requires that the JSON type of the element was
   * an actual string.
   *
   * @return A `string_view` into the string, or:
   *         - UNEXPECTED_TYPE error if the JSON element is not a string
   */
  inline simdjson_result<const char *> as_c_str() const noexcept;

  /**
   * Read this element as a C++ string_view (string with length).
   *
   * Does *not* convert other types to a string; requires that the JSON type of the element was
   * an actual string.
   *
   * @return A `string_view` into the string, or:
   *         - UNEXPECTED_TYPE error if the JSON element is not a string
   */
  inline simdjson_result<std::string_view> as_string() const noexcept;

  /**
   * Read this element as an unsigned integer.
   *
   * @return The uninteger value, or:
   *         - UNEXPECTED_TYPE if the JSON element is not an integer
   *         - NUMBER_OUT_OF_RANGE if the integer doesn't fit in 64 bits or is negative
   */
  inline simdjson_result<uint64_t> as_uint64_t() const noexcept;

  /**
   * Read this element as a signed integer.
   *
   * @return The integer value, or:
   *         - UNEXPECTED_TYPE if the JSON element is not an integer
   *         - NUMBER_OUT_OF_RANGE if the integer doesn't fit in 64 bits
   */
  inline simdjson_result<int64_t> as_int64_t() const noexcept;

  /**
   * Read this element as a floating point value.
   *
   * @return The double value, or:
   *         - UNEXPECTED_TYPE if the JSON element is not a number
   */
  inline simdjson_result<double> as_double() const noexcept;

  /**
   * Read this element as a JSON array.
   *
   * @return The array value, or:
   *         - UNEXPECTED_TYPE if the JSON element is not an array
   */
  inline array_result as_array() const noexcept;

  /**
   * Read this element as a JSON object (key/value pairs).
   *
   * @return The object value, or:
   *         - UNEXPECTED_TYPE if the JSON element is not an object
   */
  inline object_result as_object() const noexcept;

#if SIMDJSON_EXCEPTIONS
  /**
   * Read this element as a boolean.
   *
   * @return The boolean value
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not a boolean.
   */
  inline operator bool() const noexcept(false);

  /**
   * Read this element as a null-terminated string.
   *
   * Does *not* convert other types to a string; requires that the JSON type of the element was
   * an actual string.
   *
   * @return The string value.
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not a string.
   */
  inline explicit operator const char*() const noexcept(false);

  /**
   * Read this element as a null-terminated string.
   *
   * Does *not* convert other types to a string; requires that the JSON type of the element was
   * an actual string.
   *
   * @return The string value.
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not a string.
   */
  inline operator std::string_view() const noexcept(false);

  /**
   * Read this element as an unsigned integer.
   *
   * @return The integer value.
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not an integer
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer doesn't fit in 64 bits or is negative
   */
  inline operator uint64_t() const noexcept(false);
  /**
   * Read this element as an signed integer.
   *
   * @return The integer value.
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not an integer
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer doesn't fit in 64 bits
   */
  inline operator int64_t() const noexcept(false);
  /**
   * Read this element as an double.
   *
   * @return The double value.
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not a number
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer doesn't fit in 64 bits or is negative
   */
  inline operator double() const noexcept(false);
  /**
   * Read this element as a JSON array.
   *
   * @return The JSON array.
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not an array
   */
  inline operator document::array() const noexcept(false);
  /**
   * Read this element as a JSON object (key/value pairs).
   *
   * @return The JSON object.
   * @exception simdjson_error(UNEXPECTED_TYPE) if the JSON element is not an object
   */
  inline operator document::object() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document &doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   doc["/foo/a/1"] == 20
   *   doc["/"]["foo"]["a"].at(1) == 20
   *   doc[""]["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](std::string_view json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document &doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   doc["/foo/a/1"] == 20
   *   doc["/"]["foo"]["a"].at(1) == 20
   *   doc[""]["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](const char *json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document &doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
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
  inline element_result at(std::string_view json_pointer) const noexcept;

  /**
   * Get the value at the given index.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  inline element_result at(size_t index) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   document::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].as_uint64_t().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].as_uint64_t().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key(std::string_view s) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * Note: The key will be matched against **unescaped** JSON:
   *
   *   document::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].as_uint64_t().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].as_uint64_t().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key(const char *s) const noexcept;

private:
  really_inline element(const document *_doc, size_t _json_index) noexcept;
  friend class document;
  friend class document::element_result;
  template<typename T>
  friend class minify;
};

/**
 * Represents a JSON array.
 */
class document::array : protected internal::tape_ref {
public:
  /** Create a new, invalid array */
  really_inline array() noexcept;

  class iterator : tape_ref {
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
    really_inline iterator(const document *_doc, size_t _json_index) noexcept;
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
   *   document::parser parser;
   *   document::array a = parser.parse(R"([ { "foo": { "a": [ 10, 20, 30 ] }} ])");
   *   a["0/foo/a/1"] == 20
   *   a["0"]["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](std::string_view json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document::array a = parser.parse(R"([ { "foo": { "a": [ 10, 20, 30 ] }} ])");
   *   a["0/foo/a/1"] == 20
   *   a["0"]["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](const char *json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document::array a = parser.parse(R"([ { "foo": { "a": [ 10, 20, 30 ] }} ])");
   *   a.at("0/foo/a/1") == 20
   *   a.at("0")["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result at(std::string_view json_pointer) const noexcept;

  /**
   * Get the value at the given index.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  inline element_result at(size_t index) const noexcept;

private:
  really_inline array(const document *_doc, size_t _json_index) noexcept;
  friend class document::element;
  friend class document::element_result;
  template<typename T>
  friend class minify;
};

/**
 * Represents a JSON object.
 */
class document::object : protected internal::tape_ref {
public:
  /** Create a new, invalid object */
  really_inline object() noexcept;

  class iterator : protected internal::tape_ref {
  public:
    /**
     * Get the actual key/value pair
     */
    inline const document::key_value_pair operator*() const noexcept;
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
    really_inline iterator(const document *_doc, size_t _json_index) noexcept;
    friend class document::object;
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
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document::object obj = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   obj["foo/a/1"] == 20
   *   obj["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](std::string_view json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document::object obj = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   obj["foo/a/1"] == 20
   *   obj["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result operator[](const char *json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.
   *
   *   document::parser parser;
   *   document::object obj = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})");
   *   obj.at("foo/a/1") == 20
   *   obj.at("foo")["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline element_result at(std::string_view json_pointer) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   document::parser parser;
   *   parser.parse(R"({ "a\n": 1 })")["a\n"].as_uint64_t().value == 1
   *   parser.parse(R"({ "a\n": 1 })")["a\\n"].as_uint64_t().error == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key(std::string_view s) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * Note: The key will be matched against **unescaped** JSON.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key(const char *s) const noexcept;

  /**
   * Get the value associated with the given key, the provided key is
   * considered to have length characters.
   *
   * Note: The key will be matched against **unescaped** JSON.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key(const char *s, size_t length) const noexcept;
  /**
   * Get the value associated with the given key in a case-insensitive manner.
   *
   * Note: The key will be matched against **unescaped** JSON.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline element_result at_key_case_insensitive(const char *s) const noexcept;

private:
  really_inline object(const document *_doc, size_t _json_index) noexcept;
  friend class document::element;
  friend class document::element_result;
  template<typename T>
  friend class minify;
};

/**
 * Key/value pair in an object.
 */
class document::key_value_pair {
public:
  std::string_view key;
  document::element value;

private:
  really_inline key_value_pair(std::string_view _key, document::element _value) noexcept;
  friend class document::object;
};


 /** The result of a JSON navigation that may fail. */
class document::element_result : public simdjson_result<document::element> {
public:
  really_inline element_result() noexcept;
  really_inline element_result(element &&value) noexcept;
  really_inline element_result(error_code error) noexcept;

  /** Whether this is a JSON `null` */
  inline simdjson_result<bool> is_null() const noexcept;
  inline simdjson_result<bool> as_bool() const noexcept;
  inline simdjson_result<std::string_view> as_string() const noexcept;
  inline simdjson_result<const char *> as_c_str() const noexcept;
  inline simdjson_result<uint64_t> as_uint64_t() const noexcept;
  inline simdjson_result<int64_t> as_int64_t() const noexcept;
  inline simdjson_result<double> as_double() const noexcept;
  inline array_result as_array() const noexcept;
  inline object_result as_object() const noexcept;

  inline element_result operator[](std::string_view json_pointer) const noexcept;
  inline element_result operator[](const char *json_pointer) const noexcept;
  inline element_result at(std::string_view json_pointer) const noexcept;
  inline element_result at(size_t index) const noexcept;
  inline element_result at_key(std::string_view key) const noexcept;
  inline element_result at_key(const char *key) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline operator bool() const noexcept(false);
  inline explicit operator const char*() const noexcept(false);
  inline operator std::string_view() const noexcept(false);
  inline operator uint64_t() const noexcept(false);
  inline operator int64_t() const noexcept(false);
  inline operator double() const noexcept(false);
  inline operator array() const noexcept(false);
  inline operator object() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

/** The result of a JSON conversion that may fail. */
class document::array_result : public simdjson_result<document::array> {
public:
  really_inline array_result() noexcept;
  really_inline array_result(array value) noexcept;
  really_inline array_result(error_code error) noexcept;

  inline element_result operator[](std::string_view json_pointer) const noexcept;
  inline element_result operator[](const char *json_pointer) const noexcept;
  inline element_result at(std::string_view json_pointer) const noexcept;
  inline element_result at(size_t index) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline array::iterator begin() const noexcept(false);
  inline array::iterator end() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

/** The result of a JSON conversion that may fail. */
class document::object_result : public simdjson_result<document::object> {
public:
  really_inline object_result() noexcept;
  really_inline object_result(object value) noexcept;
  really_inline object_result(error_code error) noexcept;

  inline element_result operator[](std::string_view json_pointer) const noexcept;
  inline element_result operator[](const char *json_pointer) const noexcept;
  inline element_result at(std::string_view json_pointer) const noexcept;
  inline element_result at_key(std::string_view key) const noexcept;
  inline element_result at_key(const char *key) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline object::iterator begin() const noexcept(false);
  inline object::iterator end() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
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
class document::parser {
public:
  /**
  * Create a JSON parser.
  *
  * The new parser will have zero capacity.
  *
  * @param max_capacity The maximum document length the parser can automatically handle. The parser
  *    will allocate more capacity on an as needed basis (when it sees documents too big to handle)
  *    up to this amount. The parser still starts with zero capacity no matter what this number is:
  *    to allocate an initial capacity, call set_capacity() after constructing the parser. Defaults
  *    to SIMDJSON_MAXSIZE_BYTES (the largest single document simdjson can process).
  * @param max_depth The maximum depth--number of nested objects and arrays--this parser can handle.
  *    This will not be allocated until parse() is called for the first time. Defaults to
  *    DEFAULT_MAX_DEPTH.
  */
  really_inline parser(size_t max_capacity = SIMDJSON_MAXSIZE_BYTES, size_t max_depth = DEFAULT_MAX_DEPTH) noexcept;
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
   * Load a JSON document from a file and return a reference to it.
   *
   *   document::parser parser;
   *   const document &doc = parser.load("jsonexamples/twitter.json");
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
  inline doc_result load(const std::string& path) noexcept; 

  /**
   * Load a file containing many JSON documents.
   *
   *   document::parser parser;
   *   for (const document &doc : parser.parse_many(path)) {
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
   *   document::parser parser;
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
  inline document::stream load_many(const std::string& path, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept; 

  /**
   * Parse a JSON document and return a temporary reference to it.
   *
   *   document::parser parser;
   *   const document &doc = parser.parse(buf, len);
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
  inline doc_result parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) noexcept;

  /**
   * Parse a JSON document and return a temporary reference to it.
   *
   *   document::parser parser;
   *   const document &doc = parser.parse(buf, len);
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
  really_inline doc_result parse(const char *buf, size_t len, bool realloc_if_needed = true) noexcept;

  /**
   * Parse a JSON document and return a temporary reference to it.
   *
   *   document::parser parser;
   *   const document &doc = parser.parse(s);
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
   * If s.capacity() is less than SIMDJSON_PADDING, the string will be copied into an enlarged
   * temporary buffer before parsing.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than len, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param s The JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes, or
   *          a new string will be created with the extra padding.
   * @return The document, or an error:
   *         - MEMALLOC if the string does not have enough padding or the parser does not have
   *           enough capacity, and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails.
   */
  really_inline doc_result parse(const std::string &s) noexcept;

  /**
   * Parse a JSON document and return a temporary reference to it.
   *
   *   document::parser parser;
   *   const document &doc = parser.parse(s);
   *
   * ### IMPORTANT: Document Lifetime
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than batch_size, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param s The JSON to parse.
   * @return The document, or an error:
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails.
   */
  really_inline doc_result parse(const padded_string &s) noexcept;

  // We do not want to allow implicit conversion from C string to std::string.
  really_inline doc_result parse(const char *buf) noexcept = delete;

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
  inline stream parse_many(const uint8_t *buf, size_t len, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;

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
   *         - other json errors if parsing fails
   */
  inline stream parse_many(const char *buf, size_t len, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;

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
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails
   *         - CAPACITY if the parser does not have enough capacity and batch_size > max_capacity.
   *         - other json errors if parsing fails
   */
  inline stream parse_many(const std::string &s, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;

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
   * @param s The concatenated JSON to parse.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return The stream. If there is an error, it will be returned during iteration. An empty input
   *         will yield 0 documents rather than an EMPTY error. Errors:
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails
   *         - CAPACITY if the parser does not have enough capacity and batch_size > max_capacity.
   *         - other json errors if parsing fails
   */
  inline stream parse_many(const padded_string &s, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;

  // We do not want to allow implicit conversion from C string to std::string.
  really_inline doc_result parse_many(const char *buf, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept = delete;

  /**
   * The largest document this parser can automatically support.
   *
   * The parser may reallocate internal buffers as needed up to this amount.
   *
   * @return Maximum capacity, in bytes.
   */
  really_inline size_t max_capacity() const noexcept;

  /**
   * The largest document this parser can support without reallocating.
   *
   * @return Current capacity, in bytes.
   */
  really_inline size_t capacity() const noexcept;

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

  /**
   * Set capacity. This is the largest document this parser can support without reallocating.
   *
   * This will allocate or deallocate as necessary.
   *
   * @param capacity The new capacity, in bytes.
   *
   * @return MEMALLOC if unsuccessful, SUCCESS otherwise.
   */
  WARN_UNUSED inline error_code set_capacity(size_t capacity) noexcept;

  /**
   * Set the maximum level of nested object and arrays supported by this parser.
   *
   * This will allocate or deallocate as necessary.
   *
   * @param max_depth The new maximum depth, in bytes.
   *
   * @return MEMALLOC if unsuccessful, SUCCESS otherwise.
   */
  WARN_UNUSED inline error_code set_max_depth(size_t max_depth) noexcept;

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * Equivalent to calling set_capacity() and set_max_depth().
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return true if successful, false if allocation failed.
   */
  WARN_UNUSED inline bool allocate_capacity(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH) noexcept;

  // type aliases for backcompat
  /** @deprecated Use the new DOM API instead */
  class Iterator;
  /** @deprecated Use simdjson_error instead */
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

private:
  //
  // The maximum document length this parser supports.
  //
  // Buffers are large enough to handle any document up to this length.
  //
  size_t _capacity{0};

  //
  // The maximum document length this parser will automatically support.
  //
  // The parser will not be automatically allocated above this amount.
  //
  size_t _max_capacity;

  //
  // The maximum depth (number of nested objects and arrays) supported by this parser.
  //
  // Defaults to DEFAULT_MAX_DEPTH.
  //
  size_t _max_depth;

  //
  // The loaded buffer (reused each time load() is called)
  //
  std::unique_ptr<char[], decltype(&aligned_free_char)> loaded_bytes;

  // Capacity of loaded_bytes buffer.
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

  // Ensure we have enough capacity to handle at least desired_capacity bytes,
  // and auto-allocate if not.
  inline error_code ensure_capacity(size_t desired_capacity) noexcept;

  //
  // Read the file into loaded_bytes
  //
  inline simdjson_result<size_t> read_file(const std::string &path) noexcept;

#if SIMDJSON_EXCEPTIONS
  // Used internally to get the document
  inline const document &get_document() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

  friend class document::parser::Iterator;
  friend class document::stream;
}; // class parser

/**
 * Minifies a JSON element or document, printing the smallest possible valid JSON.
 *
 *   document::parser parser;
 *   document &doc = parser.parse("   [ 1 , 2 , 3 ] "_padded);
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

/**
 * Print JSON to an output stream.
 *
 * By default, the document will be printed minified.
 *
 * @param out The output stream.
 * @param value The document to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const document &value) { return out << minify(value); }
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const document::element &value) { return out << minify(value); };
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const document::array &value) { return out << minify(value); }
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const document::object &value) { return out << minify(value); }
/**
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const document::key_value_pair &value) { return out << minify(value); }

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
inline std::ostream& operator<<(std::ostream& out, const document::doc_result &value) noexcept(false) { return out << minify(value); }
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
inline std::ostream& operator<<(std::ostream& out, const document::element_result &value) noexcept(false) { return out << minify(value); }
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
inline std::ostream& operator<<(std::ostream& out, const document::array_result &value) noexcept(false) { return out << minify(value); }
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
inline std::ostream& operator<<(std::ostream& out, const document::object_result &value) noexcept(false) { return out << minify(value); }

#endif

} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_H