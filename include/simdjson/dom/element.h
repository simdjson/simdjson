#ifndef SIMDJSON_DOM_ELEMENT_H
#define SIMDJSON_DOM_ELEMENT_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/tape_ref.h"
#include "simdjson/minify.h"
#include <ostream>

namespace simdjson {
namespace dom {

class array;
class document;
class object;

/**
 * The actual concrete type of a JSON element
 * This is the type it is most easily cast to with get<>.
 */
enum class element_type {
  ARRAY = '[',     ///< dom::array
  OBJECT = '{',    ///< dom::object
  INT64 = 'l',     ///< int64_t
  UINT64 = 'u',    ///< uint64_t: any integer that fits in uint64_t but *not* int64_t
  DOUBLE = 'd',    ///< double: Any number with a "." or "e" that fits in double.
  STRING = '"',    ///< std::string_view
  BOOL = 't',      ///< bool
  NULL_VALUE = 'n' ///< null
};

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

  /** The type of this element. */
  really_inline element_type type() const noexcept;

  /** Whether this element is a json `null`. */
  really_inline bool is_null() const noexcept;

  /**
   * Tell whether the value can be cast to provided type (T).
   *
   * Supported types:
   * - Boolean: bool
   * - Number: double, uint64_t, int64_t
   * - String: std::string_view, const char *
   * - Array: dom::array
   * - Object: dom::object
   *
   * @tparam T bool, double, uint64_t, int64_t, std::string_view, const char *, dom::array, dom::object
   */
  template<typename T>
  really_inline bool is() const noexcept;

  /**
   * Get the value as the provided type (T).
   *
   * Supported types:
   * - Boolean: bool
   * - Number: double, uint64_t, int64_t
   * - String: std::string_view, const char *
   * - Array: dom::array
   * - Object: dom::object
   *
   * @tparam T bool, double, uint64_t, int64_t, std::string_view, const char *, dom::array, dom::object
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
 * Print JSON to an output stream.
 *
 * By default, the value will be printed minified.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const element &value);

/**
 * Print element type to an output stream.
 *
 * @param out The output stream.
 * @param value The value to print.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, element_type type);

} // namespace dom

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<dom::element> : public internal::simdjson_result_base<dom::element> {
public:
  really_inline simdjson_result() noexcept; ///< @private
  really_inline simdjson_result(dom::element &&value) noexcept; ///< @private
  really_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<dom::element_type> type() const noexcept;
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
inline std::ostream& operator<<(std::ostream& out, const simdjson_result<dom::element> &value) noexcept(false);
#endif

} // namespace simdjson

#endif // SIMDJSON_DOM_DOCUMENT_H
