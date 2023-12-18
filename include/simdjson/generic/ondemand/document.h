#ifndef SIMDJSON_GENERIC_ONDEMAND_DOCUMENT_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_DOCUMENT_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/json_iterator.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A JSON document. It holds a json_iterator instance.
 *
 * Used by tokens to get text, and string buffer location.
 *
 * You must keep the document around during iteration.
 */
class document {
public:
  /**
   * Create a new invalid document.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_inline document() noexcept = default;
  simdjson_inline document(const document &other) noexcept = delete; // pass your documents by reference, not by copy
  simdjson_inline document(document &&other) noexcept = default;
  simdjson_inline document &operator=(const document &other) noexcept = delete;
  simdjson_inline document &operator=(document &&other) noexcept = default;

  /**
   * Cast this JSON value to an array.
   *
   * @returns An object that can be used to iterate the array.
   * @returns INCORRECT_TYPE If the JSON value is not an array.
   */
  simdjson_inline simdjson_result<array> get_array() & noexcept;
  /**
   * Cast this JSON value to an object.
   *
   * @returns An object that can be used to look up or iterate fields.
   * @returns INCORRECT_TYPE If the JSON value is not an object.
   */
  simdjson_inline simdjson_result<object> get_object() & noexcept;
  /**
   * Cast this JSON value to an unsigned integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit unsigned integer.
   */
  simdjson_inline simdjson_result<uint64_t> get_uint64() noexcept;
  /**
   * Cast this JSON value (inside string) to an unsigned integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit unsigned integer.
   */
  simdjson_inline simdjson_result<uint64_t> get_uint64_in_string() noexcept;
  /**
   * Cast this JSON value to a signed integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit integer.
   */
  simdjson_inline simdjson_result<int64_t> get_int64() noexcept;
  /**
   * Cast this JSON value (inside string) to a signed integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit integer.
   */
  simdjson_inline simdjson_result<int64_t> get_int64_in_string() noexcept;
  /**
   * Cast this JSON value to a double.
   *
   * @returns A double.
   * @returns INCORRECT_TYPE If the JSON value is not a valid floating-point number.
   */
  simdjson_inline simdjson_result<double> get_double() noexcept;

  /**
   * Cast this JSON value (inside string) to a double.
   *
   * @returns A double.
   * @returns INCORRECT_TYPE If the JSON value is not a valid floating-point number.
   */
  simdjson_inline simdjson_result<double> get_double_in_string() noexcept;
  /**
   * Cast this JSON value to a string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * Important: Calling get_string() twice on the same document is an error.
   *
   * @param Whether to allow a replacement character for unmatched surrogate pairs.
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next
   *          time it parses a document or when it is destroyed.
   * @returns INCORRECT_TYPE if the JSON value is not a string.
   */
  simdjson_inline simdjson_result<std::string_view> get_string(bool allow_replacement = false) noexcept;
  /**
   * Attempts to fill the provided std::string reference with the parsed value of the current string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * Important: a value should be consumed once. Calling get_string() twice on the same value
   * is an error.
   *
   * Performance: This method may be slower than get_string() or get_string(bool) because it may need to allocate memory.
   * We recommend you avoid allocating an std::string unless you need to.
   *
   * @returns INCORRECT_TYPE if the JSON value is not a string. Otherwise, we return SUCCESS.
   */
  template <typename string_type>
  simdjson_inline error_code get_string(string_type& receiver, bool allow_replacement = false) noexcept;
  /**
   * Cast this JSON value to a string.
   *
   * The string is not guaranteed to be valid UTF-8. See https://simonsapin.github.io/wtf-8/
   *
   * Important: Calling get_wobbly_string() twice on the same document is an error.
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next
   *          time it parses a document or when it is destroyed.
   * @returns INCORRECT_TYPE if the JSON value is not a string.
   */
  simdjson_inline simdjson_result<std::string_view> get_wobbly_string() noexcept;
  /**
   * Cast this JSON value to a raw_json_string.
   *
   * The string is guaranteed to be valid UTF-8, and may have escapes in it (e.g. \\ or \n).
   *
   * @returns A pointer to the raw JSON for the given string.
   * @returns INCORRECT_TYPE if the JSON value is not a string.
   */
  simdjson_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  /**
   * Cast this JSON value to a bool.
   *
   * @returns A bool value.
   * @returns INCORRECT_TYPE if the JSON value is not true or false.
   */
  simdjson_inline simdjson_result<bool> get_bool() noexcept;
  /**
   * Cast this JSON value to a value when the document is an object or an array.
   *
   * You must not have begun iterating through the object or array. When
   * SIMDJSON_DEVELOPMENT_CHECKS is set to 1 (which is the case when building in Debug mode
   * by default), and you have already begun iterating,
   * you will get an OUT_OF_ORDER_ITERATION error. If you have begun iterating, you can use
   * rewind() to reset the document to its initial state before calling this method.
   *
   * @returns A value if a JSON array or object cannot be found.
   * @returns SCALAR_DOCUMENT_AS_VALUE error is the document is a scalar (see is_scalar() function).
   */
  simdjson_inline simdjson_result<value> get_value() noexcept;

  /**
   * Checks if this JSON value is null.  If and only if the value is
   * null, then it is consumed (we advance). If we find a token that
   * begins with 'n' but is not 'null', then an error is returned.
   *
   * @returns Whether the value is null.
   * @returns INCORRECT_TYPE If the JSON value begins with 'n' and is not 'null'.
   */
  simdjson_inline simdjson_result<bool> is_null() noexcept;

  /**
   * Get this value as the given type.
   *
   * Supported types: object, array, raw_json_string, string_view, uint64_t, int64_t, double, bool
   *
   * You may use get_double(), get_bool(), get_uint64(), get_int64(),
   * get_object(), get_array(), get_raw_json_string(), or get_string() instead.
   *
   * @returns A value of the given type, parsed from the JSON.
   * @returns INCORRECT_TYPE If the JSON value is not the given type.
   */
  template<typename T> simdjson_inline simdjson_result<T> get() & noexcept {
    // Unless the simdjson library provides an inline implementation, calling this method should
    // immediately fail.
    static_assert(!sizeof(T), "The get method with given type is not implemented by the simdjson library. "
      "The supported types are ondemand::object, ondemand::array, raw_json_string, std::string_view, uint64_t, "
      "int64_t, double, and bool. We recommend you use get_double(), get_bool(), get_uint64(), get_int64(), "
      " get_object(), get_array(), get_raw_json_string(), or get_string() instead of the get template.");
  }
  /** @overload template<typename T> simdjson_result<T> get() & noexcept */
  template<typename T> simdjson_inline simdjson_result<T> get() && noexcept {
    // Unless the simdjson library provides an inline implementation, calling this method should
    // immediately fail.
    static_assert(!sizeof(T), "The get method with given type is not implemented by the simdjson library. "
      "The supported types are ondemand::object, ondemand::array, raw_json_string, std::string_view, uint64_t, "
      "int64_t, double, and bool. We recommend you use get_double(), get_bool(), get_uint64(), get_int64(), "
      " get_object(), get_array(), get_raw_json_string(), or get_string() instead of the get template.");
  }

  /**
   * Get this value as the given type.
   *
   * Supported types: object, array, raw_json_string, string_view, uint64_t, int64_t, double, bool, value
   *
   * Be mindful that the document instance must remain in scope while you are accessing object, array and value instances.
   *
   * @param out This is set to a value of the given type, parsed from the JSON. If there is an error, this may not be initialized.
   * @returns INCORRECT_TYPE If the JSON value is not an object.
   * @returns SUCCESS If the parse succeeded and the out parameter was set to the value.
   */
  template<typename T> simdjson_inline error_code get(T &out) & noexcept;
  /** @overload template<typename T> error_code get(T &out) & noexcept */
  template<typename T> simdjson_inline error_code get(T &out) && noexcept;

#if SIMDJSON_EXCEPTIONS
  template <class T>
  explicit simdjson_inline operator T() noexcept(false);
  /**
   * Cast this JSON value to an array.
   *
   * @returns An object that can be used to iterate the array.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not an array.
   */
  simdjson_inline operator array() & noexcept(false);
  /**
   * Cast this JSON value to an object.
   *
   * @returns An object that can be used to look up or iterate fields.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not an object.
   */
  simdjson_inline operator object() & noexcept(false);
  /**
   * Cast this JSON value to an unsigned integer.
   *
   * @returns A signed 64-bit integer.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a 64-bit unsigned integer.
   */
  simdjson_inline operator uint64_t() noexcept(false);
  /**
   * Cast this JSON value to a signed integer.
   *
   * @returns A signed 64-bit integer.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a 64-bit integer.
   */
  simdjson_inline operator int64_t() noexcept(false);
  /**
   * Cast this JSON value to a double.
   *
   * @returns A double.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a valid floating-point number.
   */
  simdjson_inline operator double() noexcept(false);
  /**
   * Cast this JSON value to a string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next
   *          time it parses a document or when it is destroyed.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not a string.
   */
  simdjson_inline operator std::string_view() noexcept(false);
  /**
   * Cast this JSON value to a raw_json_string.
   *
   * The string is guaranteed to be valid UTF-8, and may have escapes in it (e.g. \\ or \n).
   *
   * @returns A pointer to the raw JSON for the given string.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not a string.
   */
  simdjson_inline operator raw_json_string() noexcept(false);
  /**
   * Cast this JSON value to a bool.
   *
   * @returns A bool value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not true or false.
   */
  simdjson_inline operator bool() noexcept(false);
  /**
   * Cast this JSON value to a value when the document is an object or an array.
   *
   * You must not have begun iterating through the object or array. When
   * SIMDJSON_DEVELOPMENT_CHECKS is defined, and you have already begun iterating,
   * you will get an OUT_OF_ORDER_ITERATION error. If you have begun iterating, you can use
   * rewind() to reset the document to its initial state before calling this method.
   *
   * @returns A value value if a JSON array or object cannot be found.
   * @exception SCALAR_DOCUMENT_AS_VALUE error is the document is a scalar (see is_scalar() function).
   */
  simdjson_inline operator value() noexcept(false);
#endif
  /**
   * This method scans the array and counts the number of elements.
   * The count_elements method should always be called before you have begun
   * iterating through the array: it is expected that you are pointing at
   * the beginning of the array.
   * The runtime complexity is linear in the size of the array. After
   * calling this function, if successful, the array is 'rewinded' at its
   * beginning as if it had never been accessed. If the JSON is malformed (e.g.,
   * there is a missing comma), then an error is returned and it is no longer
   * safe to continue.
   */
  simdjson_inline simdjson_result<size_t> count_elements() & noexcept;
   /**
   * This method scans the object and counts the number of key-value pairs.
   * The count_fields method should always be called before you have begun
   * iterating through the object: it is expected that you are pointing at
   * the beginning of the object.
   * The runtime complexity is linear in the size of the object. After
   * calling this function, if successful, the object is 'rewinded' at its
   * beginning as if it had never been accessed. If the JSON is malformed (e.g.,
   * there is a missing comma), then an error is returned and it is no longer
   * safe to continue.
   *
   * To check that an object is empty, it is more performant to use
   * the is_empty() method.
   */
  simdjson_inline simdjson_result<size_t> count_fields() & noexcept;
  /**
   * Get the value at the given index in the array. This function has linear-time complexity.
   * This function should only be called once on an array instance since the array iterator is not reset between each call.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  simdjson_inline simdjson_result<value> at(size_t index) & noexcept;
  /**
   * Begin array iteration.
   *
   * Part of the std::iterable interface.
   */
  simdjson_inline simdjson_result<array_iterator> begin() & noexcept;
  /**
   * Sentinel representing the end of the array.
   *
   * Part of the std::iterable interface.
   */
  simdjson_inline simdjson_result<array_iterator> end() & noexcept;

  /**
   * Look up a field by name on an object (order-sensitive).
   *
   * The following code reads z, then y, then x, and thus will not retrieve x or y if fed the
   * JSON `{ "x": 1, "y": 2, "z": 3 }`:
   *
   * ```c++
   * simdjson::ondemand::parser parser;
   * auto obj = parser.parse(R"( { "x": 1, "y": 2, "z": 3 } )"_padded);
   * double z = obj.find_field("z");
   * double y = obj.find_field("y");
   * double x = obj.find_field("x");
   * ```
   *
   * **Raw Keys:** The lookup will be done against the *raw* key, and will not unescape keys.
   * e.g. `object["a"]` will match `{ "a": 1 }`, but will *not* match `{ "\u0061": 1 }`.
   *
   *
   * You must consume the fields on an object one at a time. A request for a new key
   * invalidates previous field values: it makes them unsafe. E.g., the array
   * given by content["bids"].get_array() should not be accessed after you have called
   * content["asks"].get_array(). You can detect such mistakes by first compiling and running
   * the code in Debug mode (or with the macro `SIMDJSON_DEVELOPMENT_CHECKS` set to 1): an
   * OUT_OF_ORDER_ITERATION error is generated.
   *
   * You are expected to access keys only once. You should access the value corresponding to
   * a key a single time. Doing object["mykey"].to_string()and then again object["mykey"].to_string()
   * is an error.
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_inline simdjson_result<value> find_field(std::string_view key) & noexcept;
  /** @overload simdjson_inline simdjson_result<value> find_field(std::string_view key) & noexcept; */
  simdjson_inline simdjson_result<value> find_field(const char *key) & noexcept;

  /**
   * Look up a field by name on an object, without regard to key order.
   *
   * **Performance Notes:** This is a bit less performant than find_field(), though its effect varies
   * and often appears negligible. It starts out normally, starting out at the last field; but if
   * the field is not found, it scans from the beginning of the object to see if it missed it. That
   * missing case has a non-cache-friendly bump and lots of extra scanning, especially if the object
   * in question is large. The fact that the extra code is there also bumps the executable size.
   *
   * It is the default, however, because it would be highly surprising (and hard to debug) if the
   * default behavior failed to look up a field just because it was in the wrong order--and many
   * APIs assume this. Therefore, you must be explicit if you want to treat objects as out of order.
   *
   * Use find_field() if you are sure fields will be in order (or are willing to treat it as if the
   * field wasn't there when they aren't).
   *
   * You must consume the fields on an object one at a time. A request for a new key
   * invalidates previous field values: it makes them unsafe. E.g., the array
   * given by content["bids"].get_array() should not be accessed after you have called
   * content["asks"].get_array(). You can detect such mistakes by first compiling and running
   * the code in Debug mode (or with the macro `SIMDJSON_DEVELOPMENT_CHECKS` set to 1): an
   * OUT_OF_ORDER_ITERATION error is generated.
   *
   * You are expected to access keys only once. You should access the value corresponding to a key
   * a single time. Doing object["mykey"].to_string() and then again object["mykey"].to_string()
   * is an error.
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept;
  /** @overload simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_inline simdjson_result<value> find_field_unordered(const char *key) & noexcept;
  /** @overload simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_inline simdjson_result<value> operator[](std::string_view key) & noexcept;
  /** @overload simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_inline simdjson_result<value> operator[](const char *key) & noexcept;

  /**
   * Get the type of this JSON value. It does not validate or consume the value.
   * E.g., you must still call "is_null()" to check that a value is null even if
   * "type()" returns json_type::null.
   *
   * NOTE: If you're only expecting a value to be one type (a typical case), it's generally
   * better to just call .get_double, .get_string, etc. and check for INCORRECT_TYPE (or just
   * let it throw an exception).
   *
   * @error TAPE_ERROR when the JSON value is a bad token like "}" "," or "alse".
   */
  simdjson_inline simdjson_result<json_type> type() noexcept;

  /**
   * Checks whether the document is a scalar (string, number, null, Boolean).
   * Returns false when there it is an array or object.
   *
   * @returns true if the type is string, number, null, Boolean
   * @error TAPE_ERROR when the JSON value is a bad token like "}" "," or "alse".
   */
  simdjson_inline simdjson_result<bool> is_scalar() noexcept;

  /**
   * Checks whether the document is a negative number.
   *
   * @returns true if the number if negative.
   */
  simdjson_inline bool is_negative() noexcept;
  /**
   * Checks whether the document is an integer number. Note that
   * this requires to partially parse the number string. If
   * the value is determined to be an integer, it may still
   * not parse properly as an integer in subsequent steps
   * (e.g., it might overflow).
   *
   * @returns true if the number if negative.
   */
  simdjson_inline simdjson_result<bool> is_integer() noexcept;
  /**
   * Determine the number type (integer or floating-point number) as quickly
   * as possible. This function does not fully validate the input. It is
   * useful when you only need to classify the numbers, without parsing them.
   *
   * If you are planning to retrieve the value or you need full validation,
   * consider using the get_number() method instead: it will fully parse
   * and validate the input, and give you access to the type:
   * get_number().get_number_type().
   *
   * get_number_type() is number_type::unsigned_integer if we have
   * an integer greater or equal to 9223372036854775808
   * get_number_type() is number_type::signed_integer if we have an
   * integer that is less than 9223372036854775808
   * Otherwise, get_number_type() has value number_type::floating_point_number
   *
   * This function requires processing the number string, but it is expected
   * to be faster than get_number().get_number_type() because it is does not
   * parse the number value.
   *
   * @returns the type of the number
   */
  simdjson_inline simdjson_result<number_type> get_number_type() noexcept;

  /**
   * Attempt to parse an ondemand::number. An ondemand::number may
   * contain an integer value or a floating-point value, the simdjson
   * library will autodetect the type. Thus it is a dynamically typed
   * number. Before accessing the value, you must determine the detected
   * type.
   *
   * number.get_number_type() is number_type::signed_integer if we have
   * an integer in [-9223372036854775808,9223372036854775808)
   * You can recover the value by calling number.get_int64() and you
   * have that number.is_int64() is true.
   *
   * number.get_number_type() is number_type::unsigned_integer if we have
   * an integer in [9223372036854775808,18446744073709551616)
   * You can recover the value by calling number.get_uint64() and you
   * have that number.is_uint64() is true.
   *
   * Otherwise, number.get_number_type() has value number_type::floating_point_number
   * and we have a binary64 number.
   * You can recover the value by calling number.get_double() and you
   * have that number.is_double() is true.
   *
   * You must check the type before accessing the value: it is an error
   * to call "get_int64()" when number.get_number_type() is not
   * number_type::signed_integer and when number.is_int64() is false.
   */
  simdjson_warn_unused simdjson_inline simdjson_result<number> get_number() noexcept;

  /**
   * Get the raw JSON for this token.
   *
   * The string_view will always point into the input buffer.
   *
   * The string_view will start at the beginning of the token, and include the entire token
   * *as well as all spaces until the next token (or EOF).* This means, for example, that a
   * string token always begins with a " and is always terminated by the final ", possibly
   * followed by a number of spaces.
   *
   * The string_view is *not* null-terminated. If this is a scalar (string, number,
   * boolean, or null), the character after the end of the string_view may be the padded buffer.
   *
   * Tokens include:
   * - {
   * - [
   * - "a string (possibly with UTF-8 or backslashed characters like \\\")".
   * - -1.2e-100
   * - true
   * - false
   * - null
   */
  simdjson_inline simdjson_result<std::string_view> raw_json_token() noexcept;

  /**
   * Reset the iterator inside the document instance so we are pointing back at the
   * beginning of the document, as if it had just been created. It invalidates all
   * values, objects and arrays that you have created so far (including unescaped strings).
   */
  inline void rewind() noexcept;
  /**
   * Returns debugging information.
   */
  inline std::string to_debug_string() noexcept;
  /**
   * Some unrecoverable error conditions may render the document instance unusable.
   * The is_alive() method returns true when the document is still suitable.
   */
  inline bool is_alive() noexcept;

  /**
   * Returns the current location in the document if in bounds.
   */
  inline simdjson_result<const char *> current_location() const noexcept;

  /**
   * Returns true if this document has been fully parsed.
   * If you have consumed the whole document and at_end() returns
   * false, then there may be trailing content.
   */
  inline bool at_end() const noexcept;

  /**
   * Returns the current depth in the document if in bounds.
   *
   * E.g.,
   *  0 = finished with document
   *  1 = document root value (could be [ or {, not yet known)
   *  2 = , or } inside root array/object
   *  3 = key or value inside root array/object.
   */
  simdjson_inline int32_t current_depth() const noexcept;

  /**
   * Get the value associated with the given JSON pointer.  We use the RFC 6901
   * https://tools.ietf.org/html/rfc6901 standard.
   *
   *   ondemand::parser parser;
   *   auto json = R"({ "foo": { "a": [ 10, 20, 30 ] }})"_padded;
   *   auto doc = parser.iterate(json);
   *   doc.at_pointer("/foo/a/1") == 20
   *
   * It is allowed for a key to be the empty string:
   *
   *   ondemand::parser parser;
   *   auto json = R"({ "": { "a": [ 10, 20, 30 ] }})"_padded;
   *   auto doc = parser.iterate(json);
   *   doc.at_pointer("//a/1") == 20
   *
   * Note that at_pointer() automatically calls rewind between each call. Thus
   * all values, objects and arrays that you have created so far (including unescaped strings)
   * are invalidated. After calling at_pointer, you need to consume the result: string values
   * should be stored in your own variables, arrays should be decoded and stored in your own array-like
   * structures and so forth.
   *
   * Also note that at_pointer() relies on find_field() which implies that we do not unescape keys when matching
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   *         - SCALAR_DOCUMENT_AS_VALUE if the json_pointer is empty and the document is not a scalar (see is_scalar() function).
   */
  simdjson_inline simdjson_result<value> at_pointer(std::string_view json_pointer) noexcept;
  /**
   * Consumes the document and returns a string_view instance corresponding to the
   * document as represented in JSON. It points inside the original byte array containing
   * the JSON document.
   */
  simdjson_inline simdjson_result<std::string_view> raw_json() noexcept;
protected:
  /**
   * Consumes the document.
   */
  simdjson_inline error_code consume() noexcept;

  simdjson_inline document(ondemand::json_iterator &&iter) noexcept;
  simdjson_inline const uint8_t *text(uint32_t idx) const noexcept;

  simdjson_inline value_iterator resume_value_iterator() noexcept;
  simdjson_inline value_iterator get_root_value_iterator() noexcept;
  simdjson_inline simdjson_result<object> start_or_resume_object() noexcept;
  static simdjson_inline document start(ondemand::json_iterator &&iter) noexcept;

  //
  // Fields
  //
  json_iterator iter{}; ///< Current position in the document
  static constexpr depth_t DOCUMENT_DEPTH = 0; ///< document depth is always 0

  friend class array_iterator;
  friend class value;
  friend class ondemand::parser;
  friend class object;
  friend class array;
  friend class field;
  friend class token;
  friend class document_stream;
  friend class document_reference;
};


/**
 * A document_reference is a thin wrapper around a document reference instance.
 */
class document_reference {
public:
  simdjson_inline document_reference() noexcept;
  simdjson_inline document_reference(document &d) noexcept;
  simdjson_inline document_reference(const document_reference &other) noexcept = default;
  simdjson_inline document_reference& operator=(const document_reference &other) noexcept = default;
  simdjson_inline void rewind() noexcept;
  simdjson_inline simdjson_result<array> get_array() & noexcept;
  simdjson_inline simdjson_result<object> get_object() & noexcept;
  simdjson_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_inline simdjson_result<uint64_t> get_uint64_in_string() noexcept;
  simdjson_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_inline simdjson_result<int64_t> get_int64_in_string() noexcept;
  simdjson_inline simdjson_result<double> get_double() noexcept;
  simdjson_inline simdjson_result<double> get_double_in_string() noexcept;
  simdjson_inline simdjson_result<std::string_view> get_string(bool allow_replacement = false) noexcept;
  template <typename string_type>
  simdjson_inline error_code get_string(string_type& receiver, bool allow_replacement = false) noexcept;
  simdjson_inline simdjson_result<std::string_view> get_wobbly_string() noexcept;
  simdjson_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  simdjson_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_inline simdjson_result<value> get_value() noexcept;

  simdjson_inline simdjson_result<bool> is_null() noexcept;
  template<typename T> simdjson_inline simdjson_result<T> get() & noexcept;
  simdjson_inline simdjson_result<std::string_view> raw_json() noexcept;
  simdjson_inline operator document&() const noexcept;
#if SIMDJSON_EXCEPTIONS
  template <class T>
  explicit simdjson_inline operator T() noexcept(false);
  simdjson_inline operator array() & noexcept(false);
  simdjson_inline operator object() & noexcept(false);
  simdjson_inline operator uint64_t() noexcept(false);
  simdjson_inline operator int64_t() noexcept(false);
  simdjson_inline operator double() noexcept(false);
  simdjson_inline operator std::string_view() noexcept(false);
  simdjson_inline operator raw_json_string() noexcept(false);
  simdjson_inline operator bool() noexcept(false);
  simdjson_inline operator value() noexcept(false);
#endif
  simdjson_inline simdjson_result<size_t> count_elements() & noexcept;
  simdjson_inline simdjson_result<size_t> count_fields() & noexcept;
  simdjson_inline simdjson_result<value> at(size_t index) & noexcept;
  simdjson_inline simdjson_result<array_iterator> begin() & noexcept;
  simdjson_inline simdjson_result<array_iterator> end() & noexcept;
  simdjson_inline simdjson_result<value> find_field(std::string_view key) & noexcept;
  simdjson_inline simdjson_result<value> find_field(const char *key) & noexcept;
  simdjson_inline simdjson_result<value> operator[](std::string_view key) & noexcept;
  simdjson_inline simdjson_result<value> operator[](const char *key) & noexcept;
  simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept;
  simdjson_inline simdjson_result<value> find_field_unordered(const char *key) & noexcept;

  simdjson_inline simdjson_result<json_type> type() noexcept;
  simdjson_inline simdjson_result<bool> is_scalar() noexcept;

  simdjson_inline simdjson_result<const char *> current_location() noexcept;
  simdjson_inline int32_t current_depth() const noexcept;
  simdjson_inline bool is_negative() noexcept;
  simdjson_inline simdjson_result<bool> is_integer() noexcept;
  simdjson_inline simdjson_result<number_type> get_number_type() noexcept;
  simdjson_inline simdjson_result<number> get_number() noexcept;
  simdjson_inline simdjson_result<std::string_view> raw_json_token() noexcept;
  simdjson_inline simdjson_result<value> at_pointer(std::string_view json_pointer) noexcept;
private:
  document *doc{nullptr};
};
} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;
  simdjson_inline error_code rewind() noexcept;

  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> get_array() & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> get_object() & noexcept;
  simdjson_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_inline simdjson_result<uint64_t> get_uint64_in_string() noexcept;
  simdjson_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_inline simdjson_result<int64_t> get_int64_in_string() noexcept;
  simdjson_inline simdjson_result<double> get_double() noexcept;
  simdjson_inline simdjson_result<double> get_double_in_string() noexcept;
  simdjson_inline simdjson_result<std::string_view> get_string(bool allow_replacement = false) noexcept;
  template <typename string_type>
  simdjson_inline error_code get_string(string_type& receiver, bool allow_replacement = false) noexcept;
  simdjson_inline simdjson_result<std::string_view> get_wobbly_string() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> get_raw_json_string() noexcept;
  simdjson_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> get_value() noexcept;
  simdjson_inline simdjson_result<bool> is_null() noexcept;

  template<typename T> simdjson_inline simdjson_result<T> get() & noexcept;
  template<typename T> simdjson_inline simdjson_result<T> get() && noexcept;

  template<typename T> simdjson_inline error_code get(T &out) & noexcept;
  template<typename T> simdjson_inline error_code get(T &out) && noexcept;
#if SIMDJSON_EXCEPTIONS
  template <class T>
  explicit simdjson_inline operator T() noexcept(false);
  simdjson_inline operator SIMDJSON_IMPLEMENTATION::ondemand::array() & noexcept(false);
  simdjson_inline operator SIMDJSON_IMPLEMENTATION::ondemand::object() & noexcept(false);
  simdjson_inline operator uint64_t() noexcept(false);
  simdjson_inline operator int64_t() noexcept(false);
  simdjson_inline operator double() noexcept(false);
  simdjson_inline operator std::string_view() noexcept(false);
  simdjson_inline operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false);
  simdjson_inline operator bool() noexcept(false);
  simdjson_inline operator SIMDJSON_IMPLEMENTATION::ondemand::value() noexcept(false);
#endif
  simdjson_inline simdjson_result<size_t> count_elements() & noexcept;
  simdjson_inline simdjson_result<size_t> count_fields() & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> at(size_t index) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> begin() & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> end() & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field(std::string_view key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field(const char *key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](const char *key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field_unordered(const char *key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_type> type() noexcept;
  simdjson_inline simdjson_result<bool> is_scalar() noexcept;
  simdjson_inline simdjson_result<const char *> current_location() noexcept;
  simdjson_inline int32_t current_depth() const noexcept;
  simdjson_inline bool at_end() const noexcept;
  simdjson_inline bool is_negative() noexcept;
  simdjson_inline simdjson_result<bool> is_integer() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::number_type> get_number_type() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::number> get_number() noexcept;
  /** @copydoc simdjson_inline std::string_view document::raw_json_token() const noexcept */
  simdjson_inline simdjson_result<std::string_view> raw_json_token() noexcept;

  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> at_pointer(std::string_view json_pointer) noexcept;
};


} // namespace simdjson



namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_reference> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document_reference> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document_reference value, error_code error) noexcept;
  simdjson_inline simdjson_result() noexcept = default;
  simdjson_inline error_code rewind() noexcept;

  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> get_array() & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> get_object() & noexcept;
  simdjson_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_inline simdjson_result<uint64_t> get_uint64_in_string() noexcept;
  simdjson_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_inline simdjson_result<int64_t> get_int64_in_string() noexcept;
  simdjson_inline simdjson_result<double> get_double() noexcept;
  simdjson_inline simdjson_result<double> get_double_in_string() noexcept;
  simdjson_inline simdjson_result<std::string_view> get_string(bool allow_replacement = false) noexcept;
  template <typename string_type>
  simdjson_inline error_code get_string(string_type& receiver, bool allow_replacement = false) noexcept;
  simdjson_inline simdjson_result<std::string_view> get_wobbly_string() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> get_raw_json_string() noexcept;
  simdjson_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> get_value() noexcept;
  simdjson_inline simdjson_result<bool> is_null() noexcept;
#if SIMDJSON_EXCEPTIONS
  template <class T>
  explicit simdjson_inline operator T() noexcept(false);
  simdjson_inline operator SIMDJSON_IMPLEMENTATION::ondemand::array() & noexcept(false);
  simdjson_inline operator SIMDJSON_IMPLEMENTATION::ondemand::object() & noexcept(false);
  simdjson_inline operator uint64_t() noexcept(false);
  simdjson_inline operator int64_t() noexcept(false);
  simdjson_inline operator double() noexcept(false);
  simdjson_inline operator std::string_view() noexcept(false);
  simdjson_inline operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false);
  simdjson_inline operator bool() noexcept(false);
  simdjson_inline operator SIMDJSON_IMPLEMENTATION::ondemand::value() noexcept(false);
#endif
  simdjson_inline simdjson_result<size_t> count_elements() & noexcept;
  simdjson_inline simdjson_result<size_t> count_fields() & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> at(size_t index) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> begin() & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> end() & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field(std::string_view key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field(const char *key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](const char *key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field_unordered(const char *key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_type> type() noexcept;
  simdjson_inline simdjson_result<bool> is_scalar() noexcept;
  simdjson_inline simdjson_result<const char *> current_location() noexcept;
  simdjson_inline simdjson_result<int32_t> current_depth() const noexcept;
  simdjson_inline simdjson_result<bool> is_negative() noexcept;
  simdjson_inline simdjson_result<bool> is_integer() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::number_type> get_number_type() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::number> get_number() noexcept;
  /** @copydoc simdjson_inline std::string_view document_reference::raw_json_token() const noexcept */
  simdjson_inline simdjson_result<std::string_view> raw_json_token() noexcept;

  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> at_pointer(std::string_view json_pointer) noexcept;
};


} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_DOCUMENT_H