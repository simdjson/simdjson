#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class array;
class document;
class field;
class object;
class raw_json_string;

/**
 * An ephemeral JSON value returned during iteration.
 *
 * This object must be destroyed before any other iteration occurs.
 */
class value {
public:
  simdjson_really_inline value() noexcept;
  simdjson_really_inline value(value &&other) noexcept;
  simdjson_really_inline value &operator=(value && other) noexcept;
  simdjson_really_inline value(const value &) noexcept = delete;
  simdjson_really_inline value &operator=(const value &) noexcept = delete;

  // Uses RAII to ensure we skip the value if it is unused.
  // TODO assert if two values are ever alive at the same time, to ensure they get destroyed
  simdjson_really_inline ~value() noexcept;
  simdjson_really_inline void skip() noexcept;
  simdjson_really_inline simdjson_result<array> get_array() noexcept;
  simdjson_really_inline simdjson_result<object> get_object() noexcept;
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_really_inline simdjson_result<double> get_double() noexcept;
  simdjson_really_inline simdjson_result<std::string_view> get_string() noexcept;
  simdjson_really_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  simdjson_really_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_really_inline bool is_null() noexcept;

#if SIMDJSON_EXCEPTIONS
  simdjson_really_inline operator array() noexcept(false);
  simdjson_really_inline operator object() noexcept(false);
  simdjson_really_inline operator uint64_t() noexcept(false);
  simdjson_really_inline operator int64_t() noexcept(false);
  simdjson_really_inline operator double() noexcept(false);
  simdjson_really_inline operator std::string_view() noexcept(false);
  simdjson_really_inline operator raw_json_string() noexcept(false);
  simdjson_really_inline operator bool() noexcept(false);
#endif

  simdjson_really_inline array begin() noexcept;
  simdjson_really_inline array end() noexcept;
  simdjson_really_inline simdjson_result<value> operator[](std::string_view key) noexcept;
  simdjson_really_inline simdjson_result<value> operator[](const char *key) noexcept;

protected:
  /**
   * Create a value.
   *
   * Use value::read() instead of this.
   */
  simdjson_really_inline value(document *doc, const uint8_t *json) noexcept;

  /**
   * Read a value.
   *
   * If the value is an array or object, only the opening brace will be consumed.
   *
   * @param doc The document containing the value. Iterator must be at the value start position.
   */
  static simdjson_really_inline value start(document *doc) noexcept;

  simdjson_really_inline void log_value(const char *type) const noexcept;
  simdjson_really_inline void log_error(const char *message) const noexcept;

  document *doc{}; // For the string buffer (if we need it)
  const uint8_t *json{}; // The JSON text of the value

  friend class document;
  friend class array;
  friend class field;
  friend class object;
  friend struct simdjson_result<value>;
  friend struct simdjson_result<document>;
  friend struct simdjson_result<field>;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> : public internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::value &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::value &&value, error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document *doc, error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> get_array() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> get_object() noexcept;
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_really_inline simdjson_result<double> get_double() noexcept;
  simdjson_really_inline simdjson_result<std::string_view> get_string() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> get_raw_json_string() noexcept;
  simdjson_really_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_really_inline bool is_null() noexcept;

#if SIMDJSON_EXCEPTIONS
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::array() noexcept(false);
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::object() noexcept(false);
  simdjson_really_inline operator uint64_t() noexcept(false);
  simdjson_really_inline operator int64_t() noexcept(false);
  simdjson_really_inline operator double() noexcept(false);
  simdjson_really_inline operator std::string_view() noexcept(false);
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false);
  simdjson_really_inline operator bool() noexcept(false);
#endif

  simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::array begin() noexcept;
  simdjson_really_inline SIMDJSON_IMPLEMENTATION::ondemand::array end() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](const char *key) noexcept;
};

} // namespace simdjson
