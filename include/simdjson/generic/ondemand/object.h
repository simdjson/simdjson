#ifndef SIMDJSON_GENERIC_ONDEMAND_OBJECT_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_OBJECT_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/ondemand/value_iterator.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A forward-only JSON object field iterator.
 */
class object {
public:
  /**
   * Create a new invalid object.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_inline object() noexcept = default;

  simdjson_inline simdjson_result<object_iterator> begin() noexcept;
  simdjson_inline simdjson_result<object_iterator> end() noexcept;
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
   * If you have multiple fields with a matching key ({"x": 1,  "x": 1}) be mindful
   * that only one field is returned.
   *
   * **Raw Keys:** The lookup will be done against the *raw* key, and will not unescape keys.
   * e.g. `object["a"]` will match `{ "a": 1 }`, but will *not* match `{ "\u0061": 1 }`.
   *
   * You must consume the fields on an object one at a time. A request for a new key
   * invalidates previous field values: it makes them unsafe. The value instance you get
   * from  `content["bids"]` becomes invalid when you call `content["asks"]`. The array
   * given by content["bids"].get_array() should not be accessed after you have called
   * content["asks"].get_array(). You can detect such mistakes by first compiling and running
   * the code in Debug mode (or with the macro `SIMDJSON_DEVELOPMENT_CHECKS` set to 1): an
   * OUT_OF_ORDER_ITERATION error is generated.
   *
   * You are expected to access keys only once. You should access the value corresponding to a
   * key a single time. Doing object["mykey"].to_string() and then again object["mykey"].to_string()
   * is an error.
   *
   * If you expect to have keys with escape characters, please review our documentation.
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_inline simdjson_result<value> find_field(std::string_view key) & noexcept;
  /** @overload simdjson_inline simdjson_result<value> find_field(std::string_view key) & noexcept; */
  simdjson_inline simdjson_result<value> find_field(std::string_view key) && noexcept;

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
   * field was not there when they are not in order).
   *
   * If you have multiple fields with a matching key ({"x": 1,  "x": 1}) be mindful
   * that only one field is returned.
   *
   * You must consume the fields on an object one at a time. A request for a new key
   * invalidates previous field values: it makes them unsafe. The value instance you get
   * from  `content["bids"]` becomes invalid when you call `content["asks"]`. The array
   * given by content["bids"].get_array() should not be accessed after you have called
   * content["asks"].get_array(). You can detect such mistakes by first compiling and running
   * the code in Debug mode (or with the macro `SIMDJSON_DEVELOPMENT_CHECKS` set to 1): an
   * OUT_OF_ORDER_ITERATION error is generated.
   *
   * You are expected to access keys only once. You should access the value corresponding to a key
   * a single time. Doing object["mykey"].to_string() and then again object["mykey"].to_string() is an error.
   *
   * If you expect to have keys with escape characters, please review our documentation.
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept;
  /** @overload simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) && noexcept;
  /** @overload simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_inline simdjson_result<value> operator[](std::string_view key) & noexcept;
  /** @overload simdjson_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_inline simdjson_result<value> operator[](std::string_view key) && noexcept;

  /**
   * Get the value associated with the given JSON pointer. We use the RFC 6901
   * https://tools.ietf.org/html/rfc6901 standard, interpreting the current node
   * as the root of its own JSON document.
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
   * Note that at_pointer() called on the document automatically calls the document's rewind
   * method between each call. It invalidates all previously accessed arrays, objects and values
   * that have not been consumed. Yet it is not the case when calling at_pointer on an object
   * instance: there is no rewind and no invalidation.
   *
   * You may call at_pointer more than once on an object, but each time the pointer is advanced
   * to be within the value matched by the key indicated by the JSON pointer query. Thus any preceding
   * key (as well as the current key) can no longer be used with following JSON pointer calls.
   *
   * Also note that at_pointer() relies on find_field() which implies that we do not unescape keys when matching.
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<value> at_pointer(std::string_view json_pointer) noexcept;

  /**
   * Get the value associated with the given JSONPath expression. We only support
   * JSONPath queries that trivially convertible to JSON Pointer queries: key
   * names and array indices.
   *
   * @return The value associated with the given JSONPath expression, or:
   *         - INVALID_JSON_POINTER if the JSONPath to JSON Pointer conversion fails
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   */
  inline simdjson_result<value> at_path(std::string_view json_path) noexcept;

  /**
   * Reset the iterator so that we are pointing back at the
   * beginning of the object. You should still consume values only once even if you
   * can iterate through the object more than once. If you unescape a string or a key
   * within the object more than once, you have unsafe code. Note that rewinding an object
   * means that you may need to reparse it anew: it is not a free operation.
   *
   * @returns true if the object contains some elements (not empty)
   */
  inline simdjson_result<bool> reset() & noexcept;
  /**
   * This method scans the beginning of the object and checks whether the
   * object is empty.
   * The runtime complexity is constant time. After
   * calling this function, if successful, the object is 'rewinded' at its
   * beginning as if it had never been accessed. If the JSON is malformed (e.g.,
   * there is a missing comma), then an error is returned and it is no longer
   * safe to continue.
   */
  inline simdjson_result<bool> is_empty() & noexcept;
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
   *
   * Performance hint: You should only call count_fields() as a last
   * resort as it may require scanning the document twice or more.
   */
  simdjson_inline simdjson_result<size_t> count_fields() & noexcept;
  /**
   * Consumes the object and returns a string_view instance corresponding to the
   * object as represented in JSON. It points inside the original byte array containing
   * the JSON document.
   */
  simdjson_inline simdjson_result<std::string_view> raw_json() noexcept;

protected:
  /**
   * Go to the end of the object, no matter where you are right now.
   */
  simdjson_inline error_code consume() noexcept;
  static simdjson_inline simdjson_result<object> start(value_iterator &iter) noexcept;
  static simdjson_inline simdjson_result<object> start_root(value_iterator &iter) noexcept;
  static simdjson_inline simdjson_result<object> started(value_iterator &iter) noexcept;
  static simdjson_inline object resume(const value_iterator &iter) noexcept;
  simdjson_inline object(const value_iterator &iter) noexcept;

  simdjson_warn_unused simdjson_inline error_code find_field_raw(const std::string_view key) noexcept;

  value_iterator iter{};

  friend class value;
  friend class document;
  friend struct simdjson_result<object>;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::object &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;

  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> begin() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> end() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field(std::string_view key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field(std::string_view key) && noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) && noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) & noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) && noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> at_pointer(std::string_view json_pointer) noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> at_path(std::string_view json_path) noexcept;

  inline simdjson_result<bool> reset() noexcept;
  inline simdjson_result<bool> is_empty() noexcept;
  inline simdjson_result<size_t> count_fields() & noexcept;
  inline simdjson_result<std::string_view> raw_json() noexcept;

};

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_OBJECT_H
