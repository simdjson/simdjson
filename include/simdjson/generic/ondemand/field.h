#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A JSON field (key/value pair) in an object.
 * 
 * Returned from object iteration.
 * 
 * Extends from std::pair<raw_json_string, value> so you can use C++ algorithms that rely on pairs.
 */
class field : public std::pair<raw_json_string, value> {
public:
  /**
   * Create a new invalid field.
   * 
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline field() noexcept;

  simdjson_really_inline field(field &&other) noexcept = default;
  simdjson_really_inline field &operator=(field &&other) noexcept = default;
  simdjson_really_inline field(const field &other) noexcept = delete;
  simdjson_really_inline field &operator=(const field &other) noexcept = delete;

  /**
   * Get the key.
   */
  simdjson_really_inline raw_json_string key() const noexcept;
  /**
   * Get the field value.
   */
  simdjson_really_inline ondemand::value &value() & noexcept;
  /**
   * @overload ondemand::value &ondemand::value() & noexcept
   */
  simdjson_really_inline ondemand::value value() && noexcept;

protected:
  simdjson_really_inline field(raw_json_string key, ondemand::value &&value) noexcept;
  static simdjson_really_inline simdjson_result<field> start(json_iterator_ref &&iter) noexcept;
  static simdjson_really_inline simdjson_result<field> start(json_iterator_ref &&iter, raw_json_string key) noexcept;
  friend struct simdjson_result<field>;
  friend class object_iterator;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::field> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::field &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> key() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> value() noexcept;
};

} // namespace simdjson
