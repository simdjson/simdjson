#ifndef SIMDJSON_DOM_ELEMENT_H
#define SIMDJSON_DOM_ELEMENT_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/array.h"

namespace simdjson {
namespace dom {

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
class element {
public:
  /** Create a new, invalid element. */
  simdjson_inline element() noexcept;

  /** The type of this element. */
  simdjson_inline element_type type() const noexcept;

  /**
   * Cast this element to an array.
   *
   * @returns An object that can be used to iterate the array, or:
   *          INCORRECT_TYPE if the JSON element is not an array.
   */
  inline simdjson_result<array> get_array() const noexcept;
  /**
   * Cast this element to an object.
   *
   * @returns An object that can be used to look up or iterate the object's fields, or:
   *          INCORRECT_TYPE if the JSON element is not an object.
   */
  inline simdjson_result<object> get_object() const noexcept;
  /**
   * Cast this element to a null-terminated C string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * The length of the string is given by get_string_length(). Because JSON strings
   * may contain null characters, it may be incorrect to use strlen to determine the
   * string length.
   *
   * It is possible to get a single string_view instance which represents both the string
   * content and its length: see get_string().
   *
   * @returns A pointer to a null-terminated UTF-8 string. This string is stored in the parser and will
   *          be invalidated the next time it parses a document or when it is destroyed.
   *          Returns INCORRECT_TYPE if the JSON element is not a string.
   */
  inline simdjson_result<const char *> get_c_str() const noexcept;
  /**
   * Gives the length in bytes of the string.
   *
   * It is possible to get a single string_view instance which represents both the string
   * content and its length: see get_string().
   *
   * @returns A string length in bytes.
   *          Returns INCORRECT_TYPE if the JSON element is not a string.
   */
  inline simdjson_result<size_t> get_string_length() const noexcept;
  /**
   * Cast this element to a string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next time it
   *          parses a document or when it is destroyed.
   *          Returns INCORRECT_TYPE if the JSON element is not a string.
   */
  inline simdjson_result<std::string_view> get_string() const noexcept;
  /**
   * Cast this element to a signed integer.
   *
   * @returns A signed 64-bit integer.
   *          Returns INCORRECT_TYPE if the JSON element is not an integer, or NUMBER_OUT_OF_RANGE
   *          if it is negative.
   */
  inline simdjson_result<int64_t> get_int64() const noexcept;
  /**
   * Cast this element to an unsigned integer.
   *
   * @returns An unsigned 64-bit integer.
   *          Returns INCORRECT_TYPE if the JSON element is not an integer, or NUMBER_OUT_OF_RANGE
   *          if it is too large.
   */
  inline simdjson_result<uint64_t> get_uint64() const noexcept;
  /**
   * Cast this element to a double floating-point.
   *
   * @returns A double value.
   *          Returns INCORRECT_TYPE if the JSON element is not a number.
   */
  inline simdjson_result<double> get_double() const noexcept;
  /**
   * Cast this element to a bool.
   *
   * @returns A bool value.
   *          Returns INCORRECT_TYPE if the JSON element is not a boolean.
   */
  inline simdjson_result<bool> get_bool() const noexcept;

  /**
   * Whether this element is a json array.
   *
   * Equivalent to is<array>().
   */
  inline bool is_array() const noexcept;
  /**
   * Whether this element is a json object.
   *
   * Equivalent to is<object>().
   */
  inline bool is_object() const noexcept;
  /**
   * Whether this element is a json string.
   *
   * Equivalent to is<std::string_view>() or is<const char *>().
   */
  inline bool is_string() const noexcept;
  /**
   * Whether this element is a json number that fits in a signed 64-bit integer.
   *
   * Equivalent to is<int64_t>().
   */
  inline bool is_int64() const noexcept;
  /**
   * Whether this element is a json number that fits in an unsigned 64-bit integer.
   *
   * Equivalent to is<uint64_t>().
   */
  inline bool is_uint64() const noexcept;
  /**
   * Whether this element is a json number that fits in a double.
   *
   * Equivalent to is<double>().
   */
  inline bool is_double() const noexcept;

  /**
   * Whether this element is a json number.
   *
   * Both integers and floating points will return true.
   */
  inline bool is_number() const noexcept;

  /**
   * Whether this element is a json `true` or `false`.
   *
   * Equivalent to is<bool>().
   */
  inline bool is_bool() const noexcept;
  /**
   * Whether this element is a json `null`.
   */
  inline bool is_null() const noexcept;

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
  simdjson_inline bool is() const noexcept;

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
   * You may use get_double(), get_bool(), get_uint64(), get_int64(),
   * get_object(), get_array() or get_string() instead.
   *
   * @tparam T bool, double, uint64_t, int64_t, std::string_view, const char *, dom::array, dom::object
   *
   * @returns The value cast to the given type, or:
   *          INCORRECT_TYPE if the value cannot be cast to the given type.
   */

  template<typename T>
  inline simdjson_result<T> get() const noexcept {
    // Unless the simdjson library provides an inline implementation, calling this method should
    // immediately fail.
    static_assert(!sizeof(T), "The get method with given type is not implemented by the simdjson library. "
      "The supported types are Boolean (bool), numbers (double, uint64_t, int64_t), "
      "strings (std::string_view, const char *), arrays (dom::array) and objects (dom::object). "
      "We recommend you use get_double(), get_bool(), get_uint64(), get_int64(), "
      "get_object(), get_array() or get_string() instead of the get template.");
  }

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
   * @param value The variable to set to the value. May not be set if there is an error.
   *
   * @returns The error that occurred, or SUCCESS if there was no error.
   */
  template<typename T>
  simdjson_warn_unused simdjson_inline error_code get(T &value) const noexcept;

  /**
   * Get the value as the provided type (T), setting error if it's not the given type.
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
   * @param value The variable to set to the given type. value is undefined if there is an error.
   * @param error The variable to store the error. error is set to error_code::SUCCEED if there is an error.
   */
  template<typename T>
  inline void tie(T &value, error_code &error) && noexcept;

#if SIMDJSON_EXCEPTIONS
  /**
   * Read this element as a boolean.
   *
   * @return The boolean value
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a boolean.
   */
  inline operator bool() const noexcept(false);

  /**
   * Read this element as a null-terminated UTF-8 string.
   *
   * Be mindful that JSON allows strings to contain null characters.
   *
   * Does *not* convert other types to a string; requires that the JSON type of the element was
   * an actual string.
   *
   * @return The string value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a string.
   */
  inline explicit operator const char*() const noexcept(false);

  /**
   * Read this element as a null-terminated UTF-8 string.
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
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer does not fit in 64 bits or is negative
   */
  inline operator uint64_t() const noexcept(false);
  /**
   * Read this element as an signed integer.
   *
   * @return The integer value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an integer
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer does not fit in 64 bits
   */
  inline operator int64_t() const noexcept(false);
  /**
   * Read this element as an double.
   *
   * @return The double value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a number
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
   *   int64_t(parser.parse(R"({ "a\n": 1 })"_padded)["a\n"]) == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](std::string_view key) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   int64_t(parser.parse(R"({ "a\n": 1 })"_padded)["a\n"]) == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const char *key) const noexcept;
  simdjson_result<element> operator[](int) const noexcept = delete;


  /**
   * Get the value associated with the given JSON pointer.  We use the RFC 6901
   * https://tools.ietf.org/html/rfc6901 standard.
   *
   *   dom::parser parser;
   *   element doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})"_padded);
   *   doc.at_pointer("/foo/a/1") == 20
   *   doc.at_pointer("/foo")["a"].at(1) == 20
   *   doc.at_pointer("")["foo"]["a"].at(1) == 20
   *
   * It is allowed for a key to be the empty string:
   *
   *   dom::parser parser;
   *   object obj = parser.parse(R"({ "": { "a": [ 10, 20, 30 ] }})"_padded);
   *   obj.at_pointer("//a/1") == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at_pointer(const std::string_view json_pointer) const noexcept;

  /**
   * Get the value associated with the given JSONPath expression. We only support
   * JSONPath queries that trivially convertible to JSON Pointer queries: key
   * names and array indices.
   *
   * https://datatracker.ietf.org/doc/html/draft-normington-jsonpath-00
   *
   * @return The value associated with the given JSONPath expression, or:
   *         - INVALID_JSON_POINTER if the JSONPath to JSON Pointer conversion fails
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
  */
  inline simdjson_result<element> at_path(std::string_view json_path) const noexcept;

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
  /**
   *
   * Version 0.4 of simdjson used an incorrect interpretation of the JSON Pointer standard
   * and allowed the following :
   *
   *   dom::parser parser;
   *   element doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})"_padded);
   *   doc.at("foo/a/1") == 20
   *
   * Though it is intuitive, it is not compliant with RFC 6901
   * https://tools.ietf.org/html/rfc6901
   *
   * For standard compliance, use the at_pointer function instead.
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  [[deprecated("For standard compliance, use at_pointer instead, and prefix your pointers with a slash '/', see RFC6901 ")]]
  inline simdjson_result<element> at(const std::string_view json_pointer) const noexcept;
#endif // SIMDJSON_DISABLE_DEPRECATED_API

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
   *   int64_t(parser.parse(R"({ "a\n": 1 })"_padded)["a\n"]) == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key(std::string_view key) const noexcept;

  /**
   * Get the value associated with the given key in a case-insensitive manner.
   *
   * Note: The key will be matched against **unescaped** JSON.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key_case_insensitive(std::string_view key) const noexcept;

  /**
   * operator< defines a total order for element allowing to use them in
   * ordered C++ STL containers
   *
   * @return TRUE if the key appears before the other one in the tape
   */
  inline bool operator<(const element &other) const noexcept;

  /**
   * operator== allows to verify if two element values reference the
   * same JSON item
   *
   * @return TRUE if the two values references the same JSON element
   */
  inline bool operator==(const element &other) const noexcept;

  /** @private for debugging. Prints out the root element. */
  inline bool dump_raw_tape(std::ostream &out) const noexcept;

private:
  simdjson_inline element(const internal::tape_ref &tape) noexcept;
  internal::tape_ref tape;
  friend class document;
  friend class object;
  friend class array;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::internal::string_builder;

};

} // namespace dom

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<dom::element> : public internal::simdjson_result_base<dom::element> {
public:
  simdjson_inline simdjson_result() noexcept; ///< @private
  simdjson_inline simdjson_result(dom::element &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_inline simdjson_result<dom::element_type> type() const noexcept;
  template<typename T>
  simdjson_inline bool is() const noexcept;
  template<typename T>
  simdjson_inline simdjson_result<T> get() const noexcept;
  template<typename T>
  simdjson_warn_unused simdjson_inline error_code get(T &value) const noexcept;

  simdjson_inline simdjson_result<dom::array> get_array() const noexcept;
  simdjson_inline simdjson_result<dom::object> get_object() const noexcept;
  simdjson_inline simdjson_result<const char *> get_c_str() const noexcept;
  simdjson_inline simdjson_result<size_t> get_string_length() const noexcept;
  simdjson_inline simdjson_result<std::string_view> get_string() const noexcept;
  simdjson_inline simdjson_result<int64_t> get_int64() const noexcept;
  simdjson_inline simdjson_result<uint64_t> get_uint64() const noexcept;
  simdjson_inline simdjson_result<double> get_double() const noexcept;
  simdjson_inline simdjson_result<bool> get_bool() const noexcept;

  simdjson_inline bool is_array() const noexcept;
  simdjson_inline bool is_object() const noexcept;
  simdjson_inline bool is_string() const noexcept;
  simdjson_inline bool is_int64() const noexcept;
  simdjson_inline bool is_uint64() const noexcept;
  simdjson_inline bool is_double() const noexcept;
  simdjson_inline bool is_number() const noexcept;
  simdjson_inline bool is_bool() const noexcept;
  simdjson_inline bool is_null() const noexcept;

  simdjson_inline simdjson_result<dom::element> operator[](std::string_view key) const noexcept;
  simdjson_inline simdjson_result<dom::element> operator[](const char *key) const noexcept;
  simdjson_result<dom::element> operator[](int) const noexcept = delete;
  simdjson_inline simdjson_result<dom::element> at_pointer(const std::string_view json_pointer) const noexcept;
  simdjson_inline simdjson_result<dom::element> at_path(const std::string_view json_path) const noexcept;
  [[deprecated("For standard compliance, use at_pointer instead, and prefix your pointers with a slash '/', see RFC6901 ")]]
  simdjson_inline simdjson_result<dom::element> at(const std::string_view json_pointer) const noexcept;
  simdjson_inline simdjson_result<dom::element> at(size_t index) const noexcept;
  simdjson_inline simdjson_result<dom::element> at_key(std::string_view key) const noexcept;
  simdjson_inline simdjson_result<dom::element> at_key_case_insensitive(std::string_view key) const noexcept;

#if SIMDJSON_EXCEPTIONS
  simdjson_inline operator bool() const noexcept(false);
  simdjson_inline explicit operator const char*() const noexcept(false);
  simdjson_inline operator std::string_view() const noexcept(false);
  simdjson_inline operator uint64_t() const noexcept(false);
  simdjson_inline operator int64_t() const noexcept(false);
  simdjson_inline operator double() const noexcept(false);
  simdjson_inline operator dom::array() const noexcept(false);
  simdjson_inline operator dom::object() const noexcept(false);

  simdjson_inline dom::array::iterator begin() const noexcept(false);
  simdjson_inline dom::array::iterator end() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

} // namespace simdjson

#endif // SIMDJSON_DOM_DOCUMENT_H
