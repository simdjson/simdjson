#ifndef SIMDJSON_GENERIC_ONDEMAND_FIELD_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_FIELD_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#include "simdjson/generic/ondemand/value.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

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
  simdjson_inline field() noexcept;

  /**
   * Get the key as a string_view (for higher speed, consider raw_key).
   * We deliberately use a more cumbersome name (unescaped_key) to force users
   * to think twice about using it.
   *
   * This consumes the key: once you have called unescaped_key(), you cannot
   * call it again nor can you call key().
   */
  simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> unescaped_key(bool allow_replacement) noexcept;
  /**
   * Get the key as a raw_json_string. Can be used for direct comparison with
   * an unescaped C string: e.g., key() == "test".
   */
  simdjson_inline raw_json_string key() const noexcept;
  /**
   * Get the field value.
   */
  simdjson_inline ondemand::value &value() & noexcept;
  /**
   * @overload ondemand::value &ondemand::value() & noexcept
   */
  simdjson_inline ondemand::value value() && noexcept;

protected:
  simdjson_inline field(raw_json_string key, ondemand::value &&value) noexcept;
  static simdjson_inline simdjson_result<field> start(value_iterator &parent_iter) noexcept;
  static simdjson_inline simdjson_result<field> start(const value_iterator &parent_iter, raw_json_string key) noexcept;
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
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::field &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;

  simdjson_inline simdjson_result<std::string_view> unescaped_key(bool allow_replacement = false) noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> key() noexcept;
  simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> value() noexcept;
};

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_FIELD_H