#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class array;
class document;
class field;
class object;
class raw_json_string;

/**
 * An ephemeral JSON value returned during iteration.
 */
class value {
public:
  /**
   * Create a new invalid value.
   * 
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline value() noexcept = default;

  simdjson_really_inline value(value &&other) noexcept = default;
  simdjson_really_inline value &operator=(value && other) noexcept = default;
  simdjson_really_inline value(const value &) noexcept = delete;
  simdjson_really_inline value &operator=(const value &) noexcept = delete;

  /**
   * Skips the value if the value was not successfully parsed or used.
   */
  simdjson_really_inline ~value() noexcept;

  /**
   * Get this value as the given type.
   *
   * Supported types: object, array, raw_json_string, string_view, uint64_t, int64_t, double, bool
   * 
   * @returns A value of the given type, parsed from the JSON.
   * @returns INCORRECT_TYPE If the JSON value is not the given type.
   */
  template<typename T> simdjson_really_inline simdjson_result<T> get() & noexcept;
  /** @overload template<typename T> simdjson_result<T> get() & noexcept */
  template<typename T> simdjson_really_inline simdjson_result<T> get() && noexcept;

  /**
   * Get this value as the given type.
   *
   * Supported types: object, array, raw_json_string, string_view, uint64_t, int64_t, double, bool
   * 
   * @param out This is set to a value of the given type, parsed from the JSON. If there is an error, this may not be initialized.
   * @returns INCORRECT_TYPE If the JSON value is not an object.
   * @returns SUCCESS If the parse succeeded and the out parameter was set to the value.
   */
  template<typename T> simdjson_really_inline error_code get(T &out) & noexcept;
  /** @overload template<typename T> error_code get(T &out) & noexcept */
  template<typename T> simdjson_really_inline error_code get(T &out) && noexcept;

  /**
   * Cast this JSON value to an array.
   *
   * @returns An object that can be used to iterate the array.
   * @returns INCORRECT_TYPE If the JSON value is not an array.
   */
  simdjson_really_inline simdjson_result<array> get_array() noexcept;

  /**
   * Cast this JSON value to an object.
   *
   * @returns An object that can be used to look up or iterate fields.
   * @returns INCORRECT_TYPE If the JSON value is not an object.
   */
  simdjson_really_inline simdjson_result<object> get_object() noexcept;

  // PERF NOTE: get_XXX() methods generally have both && and & variants because performance is demonstrably better on clang.
  // Specifically, in typical cases where you use a temporary value (like doc["x"].get_double()) the && version is faster
  // because the & version has to branch to check whether the parse failed or not before deciding whether the value was consumed.

  /**
   * Cast this JSON value to an unsigned integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit unsigned integer.
   */
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() && noexcept;
  /** @overload simdjson_really_inline simdjson_result<uint64_t> get_uint64() && noexcept */
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() & noexcept;

  /**
   * Cast this JSON value to a signed integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit integer.
   */
  simdjson_really_inline simdjson_result<int64_t> get_int64() && noexcept;
  /** @overload simdjson_really_inline simdjson_result<int64_t> get_int64() && noexcept */
  simdjson_really_inline simdjson_result<int64_t> get_int64() & noexcept;

  /**
   * Cast this JSON value to a double.
   *
   * @returns A double.
   * @returns INCORRECT_TYPE If the JSON value is not a valid floating-point number.
   */
  simdjson_really_inline simdjson_result<double> get_double() && noexcept;
  /** @overload simdjson_really_inline simdjson_result<double> get_double() && noexcept */
  simdjson_really_inline simdjson_result<double> get_double() & noexcept;

  /**
   * Cast this JSON value to a string.
   * 
   * The string is guaranteed to be valid UTF-8.
   *
   * Equivalent to get<std::string_view>().
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next
   *          time it parses a document or when it is destroyed.
   * @returns INCORRECT_TYPE if the JSON value is not a string.
   */
  simdjson_really_inline simdjson_result<std::string_view> get_string() && noexcept;
  /** @overload simdjson_really_inline simdjson_result<std::string_view> get_string() && noexcept */
  simdjson_really_inline simdjson_result<std::string_view> get_string() & noexcept;

  /**
   * Cast this JSON value to a raw_json_string.
   * 
   * The string is guaranteed to be valid UTF-8, and may have escapes in it (e.g. \\ or \n).
   *
   * @returns A pointer to the raw JSON for the given string.
   * @returns INCORRECT_TYPE if the JSON value is not a string.
   */
  simdjson_really_inline simdjson_result<raw_json_string> get_raw_json_string() && noexcept;
  /** @overload simdjson_really_inline simdjson_result<raw_json_string> get_raw_json_string() && noexcept */
  simdjson_really_inline simdjson_result<raw_json_string> get_raw_json_string() & noexcept;

  /**
   * Cast this JSON value to a bool.
   *
   * @returns A bool value.
   * @returns INCORRECT_TYPE if the JSON value is not true or false.
   */
  simdjson_really_inline simdjson_result<bool> get_bool() && noexcept;
  /** @overload simdjson_really_inline simdjson_result<bool> get_bool() && noexcept */
  simdjson_really_inline simdjson_result<bool> get_bool() & noexcept;

  /**
   * Checks if this JSON value is null.
   * 
   * @returns Whether the value is null.
   */
  simdjson_really_inline bool is_null() && noexcept;
  /** @overload simdjson_really_inline bool is_null() && noexcept */
  simdjson_really_inline bool is_null() & noexcept;

#if SIMDJSON_EXCEPTIONS
  /**
   * Cast this JSON value to an array.
   *
   * @returns An object that can be used to iterate the array.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not an array.
   */
  simdjson_really_inline operator array() noexcept(false);
  /**
   * Cast this JSON value to an object.
   *
   * @returns An object that can be used to look up or iterate fields.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not an object.
   */
  simdjson_really_inline operator object() noexcept(false);
  /**
   * Cast this JSON value to an unsigned integer.
   *
   * @returns A signed 64-bit integer.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a 64-bit unsigned integer.
   */
  simdjson_really_inline operator uint64_t() && noexcept(false);
  /** @overload simdjson_really_inline operator uint64_t() && noexcept(false); */
  simdjson_really_inline operator uint64_t() & noexcept(false);
  /**
   * Cast this JSON value to a signed integer.
   *
   * @returns A signed 64-bit integer.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a 64-bit integer.
   */
  simdjson_really_inline operator int64_t() && noexcept(false);
  /** @overload simdjson_really_inline operator int64_t() && noexcept(false); */
  simdjson_really_inline operator int64_t() & noexcept(false);
  /**
   * Cast this JSON value to a double.
   *
   * @returns A double.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a valid floating-point number.
   */
  simdjson_really_inline operator double() && noexcept(false);
  /** @overload simdjson_really_inline operator double() && noexcept(false); */
  simdjson_really_inline operator double() & noexcept(false);
  /**
   * Cast this JSON value to a string.
   * 
   * The string is guaranteed to be valid UTF-8.
   *
   * Equivalent to get<std::string_view>().
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next
   *          time it parses a document or when it is destroyed.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not a string.
   */
  simdjson_really_inline operator std::string_view() && noexcept(false);
  /** @overload simdjson_really_inline operator std::string_view() && noexcept(false); */
  simdjson_really_inline operator std::string_view() & noexcept(false);
  /**
   * Cast this JSON value to a raw_json_string.
   * 
   * The string is guaranteed to be valid UTF-8, and may have escapes in it (e.g. \\ or \n).
   *
   * @returns A pointer to the raw JSON for the given string.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not a string.
   */
  simdjson_really_inline operator raw_json_string() && noexcept(false);
  /** @overload simdjson_really_inline operator raw_json_string() && noexcept(false); */
  simdjson_really_inline operator raw_json_string() & noexcept(false);
  /**
   * Cast this JSON value to a bool.
   *
   * @returns A bool value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not true or false.
   */
  simdjson_really_inline operator bool() && noexcept(false);
  /** @overload simdjson_really_inline operator bool() && noexcept(false); */
  simdjson_really_inline operator bool() & noexcept(false);
#endif

  /**
   * Begin array iteration.
   *
   * Part of the std::iterable interface.
   * 
   * @returns INCORRECT_TYPE If the JSON value is not an array.
   */
  simdjson_really_inline simdjson_result<array_iterator<value>> begin() & noexcept;
  /**
   * Sentinel representing the end of the array.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline simdjson_result<array_iterator<value>> end() & noexcept;

protected:
  /**
   * Create a value.
   *
   * Use value::read() instead of this.
   */
  simdjson_really_inline value(json_iterator_ref &&iter, const uint8_t *json) noexcept;

  /**
   * Read a value.
   *
   * If the value is an array or object, only the opening brace will be consumed.
   *
   * @param doc The document containing the value. Iterator must be at the value start position.
   */
  static simdjson_really_inline value start(json_iterator_ref &&iter) noexcept;

  /**
   * Skip this value, allowing iteration to continue.
   */
  simdjson_really_inline void skip() noexcept;

  simdjson_really_inline void log_value(const char *type) const noexcept;
  simdjson_really_inline void log_error(const char *message) const noexcept;

  //
  // For array_iterator
  //
  simdjson_really_inline json_iterator &get_iterator() noexcept;
  simdjson_really_inline json_iterator_ref borrow_iterator() noexcept;
  simdjson_really_inline bool is_iterator_alive() const noexcept;
  simdjson_really_inline void iteration_finished() noexcept;
  simdjson_really_inline const uint8_t *consume() noexcept;
  template<typename T>
  simdjson_really_inline simdjson_result<T> consume_if_success(simdjson_result<T> &&result) noexcept;

  json_iterator_ref iter{};
  const uint8_t *json{}; // The JSON text of the value

  friend class document;
  template<typename T> friend class array_iterator;
  friend class field;
  friend class object;
  friend struct simdjson_result<value>;
  friend struct simdjson_result<document>;
  friend struct simdjson_result<field>;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::value &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> get_array() noexcept;

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> get_object() noexcept;

  simdjson_really_inline simdjson_result<uint64_t> get_uint64() && noexcept;
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() & noexcept;

  simdjson_really_inline simdjson_result<int64_t> get_int64() && noexcept;
  simdjson_really_inline simdjson_result<int64_t> get_int64() & noexcept;

  simdjson_really_inline simdjson_result<double> get_double() && noexcept;
  simdjson_really_inline simdjson_result<double> get_double() & noexcept;

  simdjson_really_inline simdjson_result<std::string_view> get_string() && noexcept;
  simdjson_really_inline simdjson_result<std::string_view> get_string() & noexcept;

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> get_raw_json_string() && noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> get_raw_json_string() & noexcept;

  simdjson_really_inline simdjson_result<bool> get_bool() && noexcept;
  simdjson_really_inline simdjson_result<bool> get_bool() & noexcept;

  simdjson_really_inline bool is_null() && noexcept;
  simdjson_really_inline bool is_null() & noexcept;

  template<typename T> simdjson_really_inline simdjson_result<T> get() & noexcept;
  template<typename T> simdjson_really_inline simdjson_result<T> get() && noexcept;

  template<typename T> simdjson_really_inline error_code get(T &out) & noexcept;
  template<typename T> simdjson_really_inline error_code get(T &out) && noexcept;

#if SIMDJSON_EXCEPTIONS
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::array() noexcept(false);
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::object() noexcept(false);
  simdjson_really_inline operator uint64_t() && noexcept(false);
  simdjson_really_inline operator uint64_t() & noexcept(false);
  simdjson_really_inline operator int64_t() && noexcept(false);
  simdjson_really_inline operator int64_t() & noexcept(false);
  simdjson_really_inline operator double() && noexcept(false);
  simdjson_really_inline operator double() & noexcept(false);
  simdjson_really_inline operator std::string_view() && noexcept(false);
  simdjson_really_inline operator std::string_view() & noexcept(false);
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() && noexcept(false);
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() & noexcept(false);
  simdjson_really_inline operator bool() && noexcept(false);
  simdjson_really_inline operator bool() & noexcept(false);
#endif

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<SIMDJSON_IMPLEMENTATION::ondemand::value>> begin() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator<SIMDJSON_IMPLEMENTATION::ondemand::value>> end() & noexcept;
};

} // namespace simdjson
