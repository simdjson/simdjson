#ifndef SIMDJSON_GENERIC_ONDEMAND_OBJECT_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_OBJECT_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/ondemand/value_iterator.h"
#include "simdjson/generic/ondemand/key_selector.h"
#include <vector>
#if SIMDJSON_STATIC_REFLECTION && SIMDJSON_SUPPORTS_CONCEPTS
#include "simdjson/generic/ondemand/json_string_builder.h"  // for constevalutil::fixed_string
#endif
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

#if SIMDJSON_SUPPORTS_CONCEPTS
/**
 * Result of object::for_each: the first error encountered (SUCCESS if none) and
 * the number of distinct selector keys that matched during the walk. A
 * matched_count equal to Selector::size() means every selected key was present
 * in the object. Implicitly converts to error_code so existing callers that only
 * care about the error (including SIMDJSON_TRY and the test ASSERT_* macros) keep
 * working unchanged.
 *
 * On error paths (a handler returns a non-SUCCESS error_code, or a direct-target
 * deserialization via value::get fails), matched_count reflects only the number
 * of distinct selected keys that were successfully handled *before* the failure;
 * the failing key is not counted. This is the current behavior.
 *
 * Marked [[nodiscard]]: for_each surfaces parse/type errors (from a matched
 * value, a returning handler, or the structural walk itself) only through this
 * result, so it must not be silently dropped. This matters most in builds
 * without exceptions, where it is the only channel for those errors. To
 * deliberately ignore it, assign to a variable or cast to void.
 */
struct [[nodiscard]] for_each_result {
  error_code error{SUCCESS};
  std::size_t matched_count{0};
  constexpr operator error_code() const noexcept { return error; }
};

namespace key_selector_for_each_detail {
/**
 * A per-key handler for the variadic object::for_each is either:
 *   - an invocable taking a value (run custom logic for that field), or
 *   - a deserialization target T, in which case the matched value is assigned
 *     directly via value::get(T&).
 * The target form lets callers bind fields straight to variables without a
 * lambda per key -- e.g. obj.for_each<"name","city","age">(name, city, age) --
 * while still allowing a lambda in any position when custom logic is needed
 * (for example to descend into a nested object).
 */
template <typename H>
concept field_handler =
    std::is_invocable_v<std::remove_reference_t<H>&, value> ||
    ::simdjson::deserializable<std::remove_cvref_t<H>, value>;

/**
 * noexcept-ness of handling a single handler: nothrow-invocability for the
 * callback form, or nothrow-deserializability for the direct-target form
 * (builtin scalar/string targets are noexcept; custom targets follow their
 * own tag_invoke noexcept specification).
 */
template <typename H>
inline constexpr bool nothrow_field_handler_v =
    std::is_invocable_v<std::remove_reference_t<H>&, value>
        ? std::is_nothrow_invocable_v<std::remove_reference_t<H>&, value>
        : ::simdjson::nothrow_deserializable<std::remove_cvref_t<H>, value>;

/**
 * nothrow_field_handler_v folded over a whole handler pack. The variadic
 * for_each overloads spell their conditional-noexcept specifier with this
 * single id-expression rather than an inline fold expression. MSVC (VS18)
 * mishandles a fold-expression that appears directly in the noexcept-specifier
 * of an out-of-line template definition: it fails to recognize the definition's
 * specifier as matching the in-class declaration's and reports a spurious
 * C2382 ("redefinition; different exception specifications"). Naming the fold
 * here keeps the specifier a plain identifier, which matches reliably.
 */
template <typename... Handlers>
inline constexpr bool nothrow_field_handlers_v =
    (nothrow_field_handler_v<Handlers> && ...);
} // namespace key_selector_for_each_detail
#endif

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

  /**
   * Get an iterator to the start of the object. We recommend using a range-based for loop.
   *
   * Using the iterator directly is also possible but error-prone and discouraged. In particular,
   * you must dereference the iterator exactly once per iteration (before calling '++').
   * Doing otherwise is unsafe and may lead to errors. You are responsible for ensuring
   */
  simdjson_inline simdjson_result<object_iterator> begin() noexcept;
  simdjson_inline simdjson_result<object_iterator> end() noexcept;
  /**
   * Look up a field by name on an object (order-sensitive). By order-sensitive, we mean that
   * fields must be accessed in the order they appear in the JSON text (although you can
   * skip fields). See find_field_unordered() and operator[] for an order-insensitive version.
   *
   * The following code reads z, then y, then x, and thus will not retrieve x or y if fed the
   * JSON `{ "x": 1, "y": 2, "z": 3 }`:
   *
   * ```cpp
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
   * We default operator[] on find_field_unordered() for convenience.
   * It is the default because it would be highly surprising (and hard to debug) if the
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

#if SIMDJSON_SUPPORTS_CONCEPTS
  /**
   * Walk this object once and invoke on_match(selector_index, value) for each
   * field whose key is in the compile-time key_selector Selector, in JSON order
   * (first occurrence of a duplicate key wins). Iteration stops once all
   * Selector::size() keys have matched or the object ends. The value is consumed
   * in place, so this is a low-overhead way to extract a known set of fields
   * regardless of their order in the JSON.
   *
   * Like other object iteration in simdjson, for_each consumes the object by
   * advancing the underlying iterator state; after the call the same object
   * instance should not be used for further field access or iteration.
   *
   * Usage:
   *   using sel_t = ondemand::key_selector<"id", "text", "user">;
   *   obj.for_each<sel_t>([&](std::size_t i, ondemand::value v) {
   *     switch (i) { case 0: ...; case 1: ...; }
   *   });
   *
   * Limitations (see key_selector): each key must be at most 63 characters long,
   * and the number of keys should be moderate (hard limit 255; a handful is
   * best, as the compile-time perfect hash may fail or slow compilation for
   * large key sets). The keys must be distinct, non-empty, and free of backslash, double-quote and
   * null bytes.
   *
   * The callback may return either void or an error_code. When it returns an
   * error_code, the walk stops at the first non-SUCCESS result and that error is
   * returned, which lets the callback surface value-parse errors.
   *
   * This function is conditionally noexcept: it is noexcept exactly when invoking
   * the callback is noexcept. The callback runs inside this frame, so a throwing
   * callback (e.g. one using the exception-throwing conversions like
   * std::string_view(value) or uint64_t(value)) makes for_each potentially
   * throwing too -- the exception propagates to the caller instead of crossing a
   * noexcept boundary and calling std::terminate.
   *
   * @returns a for_each_result holding the first error encountered while walking
   *          the object (including any error returned by the callback, SUCCESS if
   *          none) and the number of distinct selector keys that matched. The
   *          result converts implicitly to error_code, so callers that only need
   *          the error can ignore the count.
   */
  template <typename Selector, typename Func>
    requires key_selector_type<Selector> &&
             std::is_invocable_v<Func&, std::size_t, value>
  simdjson_inline for_each_result for_each(Func&& on_match)
      noexcept(std::is_nothrow_invocable_v<Func&, std::size_t, value>);

  /**
   * Variadic per-key form. Provide exactly one handler per key in the Selector
   * (compiler-enforced). Handlers are processed in JSON document order for the
   * matching keys. Each handler is either:
   *   - a deserialization target (a variable), in which case the matched value
   *     is assigned to it via value::get -- no lambda required; or
   *   - an invocable taking the ondemand::value (for custom logic such as
   *     descending into a nested object). It may return void or error_code;
   *     returning error_code lets you surface parse/type errors.
   * The two styles may be mixed freely, one handler per key.
   *
   * Example (bind fields straight to variables):
   *   using fields = ondemand::key_selector<"name", "city", "age">;
   *   obj.for_each<fields>(name, city, age);
   *
   * Example (mixing a target and a lambda):
   *   obj.for_each<ondemand::key_selector<"id", "user">>(
   *     id,                                          // assigned via value::get
   *     [&](ondemand::value v){ u = read_user(v); }  // custom logic
   *   );
   *
   * The index-based single-callback form (taking (size_t, value)) remains
   * available for shared-state or more complex per-key logic.
   */
  template <typename Selector, typename... Handlers>
    requires key_selector_type<Selector> &&
             (sizeof...(Handlers) == Selector::size()) &&
             (key_selector_for_each_detail::field_handler<Handlers> && ...)
  simdjson_inline for_each_result for_each(Handlers&&... on_match)
      noexcept(key_selector_for_each_detail::nothrow_field_handlers_v<Handlers...>);

  /**
   * Direct-key shorthand. Equivalent to for_each<key_selector<Keys...>>(...).
   * Lets you write the keys inline without a separate using/alias, binding each
   * field straight to a variable (or a lambda, see the Selector form above):
   *
   *   obj.for_each<"name", "city", "age">(name, city, age);
   */
  template <constevalutil::fixed_string... Keys, typename... Handlers>
    requires (sizeof...(Handlers) == sizeof...(Keys)) &&
             (sizeof...(Keys) >= 1) && (sizeof...(Keys) <= 255) &&
             (key_selector_for_each_detail::field_handler<Handlers> && ...)
  simdjson_inline for_each_result for_each(Handlers&&... on_match)
      noexcept(key_selector_for_each_detail::nothrow_field_handlers_v<Handlers...>);
#endif

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
   * Call the provided callback for each value matching the given JSONPath
   * expression with wildcard support.
   * Supports wildcard patterns like ".*" to match all object fields.
   *
   * @param json_path JSONPath expression with wildcards
   * @param callback Function called for each matching value
   * @return error_code indicating success or failure
  */
#if SIMDJSON_SUPPORTS_CONCEPTS
  template <typename Func>
    requires std::invocable<Func, value>
#else
  template <typename Func>
#endif
  inline error_code for_each_at_path_with_wildcard(std::string_view json_path, Func&& callback) noexcept;

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

#if SIMDJSON_SUPPORTS_CONCEPTS
  /**
   * Get this object as the given type.
   *
   * @param out This is set to a value of the given type, parsed from the JSON. If there is an error, this may not be initialized.
   * @returns INCORRECT_TYPE If the JSON object is not of the given type.
   * @returns SUCCESS If the parse succeeded and the out parameter was set to the value.
   */
  template <typename T>
  simdjson_warn_unused simdjson_inline error_code get(T &out)
     noexcept(custom_deserializable<T, object> ? nothrow_custom_deserializable<T, object> : true) {
    static_assert(custom_deserializable<T, object>);
    return deserialize(*this, out);
  }
  /**
   * Get this array as the given type.
   *
   * @returns A value of the given type, parsed from the JSON.
   * @returns INCORRECT_TYPE If the JSON value is not the given type.
   */
  template <typename T>
  simdjson_inline simdjson_result<T> get()
    noexcept(custom_deserializable<T, value> ? nothrow_custom_deserializable<T, value> : true)
  {
    static_assert(std::is_default_constructible<T>::value, "The specified type is not default constructible.");
    T out{};
    SIMDJSON_TRY(get<T>(out));
    return out;
  }

#if SIMDJSON_STATIC_REFLECTION
  /**
   * Extract only specific fields from the JSON object into a struct.
   *
   * This allows selective deserialization of only the fields you need,
   * potentially improving performance by skipping unwanted fields.
   *
   * Example:
   * ```cpp
   * struct Car {
   *   std::string make;
   *   std::string model;
   *   int year;
   *   double price;
   * };
   *
   * Car car;
   * object.extract_into<"make", "model">(car);
   * // Only 'make' and 'model' fields are extracted from JSON
   * ```
   *
   * @tparam FieldNames Compile-time string literals specifying which fields to extract
   * @param out The output struct to populate with selected fields
   * @returns SUCCESS on success, or an error code if a required field is missing or has wrong type
   */
  template<constevalutil::fixed_string... FieldNames, typename T>
    requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
  simdjson_warn_unused simdjson_inline error_code extract_into(T& out) & noexcept;
#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_SUPPORTS_CONCEPTS
protected:
  /**
   * Go to the end of the object, no matter where you are right now.
   */
  simdjson_warn_unused simdjson_inline error_code consume() noexcept;
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
#if SIMDJSON_SUPPORTS_CONCEPTS
  template <typename Func>
    requires std::invocable<Func, SIMDJSON_IMPLEMENTATION::ondemand::value>
#else
  template <typename Func>
#endif
  simdjson_inline error_code for_each_at_path_with_wildcard(std::string_view json_path, Func&& callback) noexcept;
  inline simdjson_result<bool> reset() noexcept;
  inline simdjson_result<bool> is_empty() noexcept;
  inline simdjson_result<size_t> count_fields() & noexcept;
  inline simdjson_result<std::string_view> raw_json() noexcept;
  #if SIMDJSON_SUPPORTS_CONCEPTS
  // TODO: move this code into object-inl.h

  template<typename T>
  simdjson_inline simdjson_result<T> get() noexcept {
    if (error()) { return error(); }
    if constexpr (std::is_same_v<T, SIMDJSON_IMPLEMENTATION::ondemand::object>) {
      return first;
    }
    return first.get<T>();
  }
  template<typename T>
  simdjson_warn_unused simdjson_inline error_code get(T& out) noexcept {
    if (error()) { return error(); }
    if constexpr (std::is_same_v<T, SIMDJSON_IMPLEMENTATION::ondemand::object>) {
      out = first;
    } else {
      SIMDJSON_TRY( first.get<T>(out) );
    }
    return SUCCESS;
  }

  /**
   * Forwards to object::for_each on the underlying object, so error-code-style
   * chains (e.g. doc["x"].get_object()) can call for_each without first
   * extracting the object. If this result holds an error, that error is returned
   * (with a zero match count) and the callback is not invoked. See
   * object::for_each for the semantics.
   */
  template <typename Selector, typename Func>
    requires SIMDJSON_IMPLEMENTATION::ondemand::key_selector_type<Selector> &&
             std::is_invocable_v<Func&, std::size_t, SIMDJSON_IMPLEMENTATION::ondemand::value>
  simdjson_inline SIMDJSON_IMPLEMENTATION::ondemand::for_each_result for_each(Func&& on_match)
      noexcept(std::is_nothrow_invocable_v<Func&, std::size_t, SIMDJSON_IMPLEMENTATION::ondemand::value>);

  /**
   * Forwarding overload for the variadic per-key form (targets and/or lambdas).
   */
  template <typename Selector, typename... Handlers>
    requires SIMDJSON_IMPLEMENTATION::ondemand::key_selector_type<Selector> &&
             (sizeof...(Handlers) == Selector::size()) &&
             (SIMDJSON_IMPLEMENTATION::ondemand::key_selector_for_each_detail::field_handler<Handlers> && ...)
  simdjson_inline SIMDJSON_IMPLEMENTATION::ondemand::for_each_result for_each(Handlers&&... on_match)
      noexcept(SIMDJSON_IMPLEMENTATION::ondemand::key_selector_for_each_detail::nothrow_field_handlers_v<Handlers...>);

  /**
   * Forwarding overload for the direct-key variadic form.
   */
  template <constevalutil::fixed_string... Keys, typename... Handlers>
    requires (sizeof...(Handlers) == sizeof...(Keys)) &&
             (sizeof...(Keys) >= 1) && (sizeof...(Keys) <= 255) &&
             (SIMDJSON_IMPLEMENTATION::ondemand::key_selector_for_each_detail::field_handler<Handlers> && ...)
  simdjson_inline SIMDJSON_IMPLEMENTATION::ondemand::for_each_result for_each(Handlers&&... on_match)
      noexcept(SIMDJSON_IMPLEMENTATION::ondemand::key_selector_for_each_detail::nothrow_field_handlers_v<Handlers...>);

#if SIMDJSON_STATIC_REFLECTION
  // TODO: move this code into object-inl.h
  template<constevalutil::fixed_string... FieldNames, typename T>
    requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
  simdjson_warn_unused simdjson_inline error_code extract_into(T& out) noexcept {
    if (error()) { return error(); }
    return first.extract_into<FieldNames...>(out);
  }
#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_SUPPORTS_CONCEPTS
};

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_OBJECT_H
