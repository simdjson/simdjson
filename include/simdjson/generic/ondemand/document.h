#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class parser;
class array;
class object;
class value;
class raw_json_string;

/**
 * A JSON document iteration.
 *
 * Used by tokens to get text, and string buffer location.
 *
 * You must keep the document around during iteration.
 */
class document {
public:
  simdjson_really_inline document(document &&other) noexcept = default;
  simdjson_really_inline document &operator=(document &&other) noexcept = default;

  simdjson_really_inline document() noexcept = default;
  simdjson_really_inline document(const document &other) = delete;
  simdjson_really_inline document &operator=(const document &other) = delete;
  simdjson_really_inline ~document() noexcept;

  simdjson_really_inline simdjson_result<array> get_array() & noexcept;
  simdjson_really_inline simdjson_result<object> get_object() & noexcept;
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_really_inline simdjson_result<double> get_double() noexcept;
  simdjson_really_inline simdjson_result<std::string_view> get_string() & noexcept;
  simdjson_really_inline simdjson_result<raw_json_string> get_raw_json_string() & noexcept;
  simdjson_really_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_really_inline bool is_null() noexcept;

#if SIMDJSON_EXCEPTIONS
  simdjson_really_inline operator array() & noexcept(false);
  simdjson_really_inline operator object() & noexcept(false);
  simdjson_really_inline operator uint64_t() noexcept(false);
  simdjson_really_inline operator int64_t() noexcept(false);
  simdjson_really_inline operator double() noexcept(false);
  simdjson_really_inline operator std::string_view() & noexcept(false);
  simdjson_really_inline operator raw_json_string() & noexcept(false);
  simdjson_really_inline operator bool() noexcept(false);
#endif

  // We don't have an array_iterator that can point at an owned json_iterator
  // simdjson_really_inline simdjson_result<array::iterator> begin() & noexcept;
  // simdjson_really_inline simdjson_result<array::iterator> end() & noexcept;
  simdjson_really_inline simdjson_result<value> operator[](std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<value> operator[](const char *key) & noexcept;

protected:
  simdjson_really_inline document(ondemand::json_iterator &&iter, const uint8_t *json) noexcept;
  simdjson_really_inline const uint8_t *text(uint32_t idx) const noexcept;

  simdjson_really_inline value as_value() noexcept;
  static simdjson_really_inline document start(ondemand::json_iterator &&iter) noexcept;
  /**
   * Set json to null if the result is successful.
   * 
   * Convenience function for value-getters.
   */
  template<typename T>
  simdjson_result<T> consume_if_success(simdjson_result<T> &&result) noexcept;

  json_iterator iter{}; ///< Current position in the document
  const uint8_t *json{}; ///< JSON for the value in the document (nullptr if value has been consumed)

  simdjson_really_inline void assert_at_start() const noexcept;

  friend struct simdjson_result<document>;
  friend class value;
  friend class ondemand::parser;
  friend class object;
  friend class array;
  friend class field;
  friend class token;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> get_array() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> get_object() & noexcept;
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_really_inline simdjson_result<double> get_double() noexcept;
  simdjson_really_inline simdjson_result<std::string_view> get_string() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> get_raw_json_string() & noexcept;
  simdjson_really_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_really_inline bool is_null() noexcept;

#if SIMDJSON_EXCEPTIONS
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::array() & noexcept(false);
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::object() & noexcept(false);
  simdjson_really_inline operator uint64_t() noexcept(false);
  simdjson_really_inline operator int64_t() noexcept(false);
  simdjson_really_inline operator double() noexcept(false);
  simdjson_really_inline operator std::string_view() & noexcept(false);
  simdjson_really_inline operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() & noexcept(false);
  simdjson_really_inline operator bool() noexcept(false);
#endif

  // simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> begin() & noexcept;
  // simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> end() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](const char *key) & noexcept;
};

} // namespace simdjson
