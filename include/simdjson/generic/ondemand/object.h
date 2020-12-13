#include "simdjson/error.h"

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
  simdjson_really_inline object() noexcept = default;

  simdjson_really_inline object_iterator begin() noexcept;
  simdjson_really_inline object_iterator end() noexcept;

  /**
   * Look up a field by name on an object.
   *
   * Important notes:
   *
   * * **Raw Keys:** The lookup will be done against the *raw* key, and will not unescape keys.
   *   e.g. `object["a"]` will match `{ "a": 1 }`, but will *not* match `{ "\u0061": 1 }`.
   * * **Order Sensitive:** Each field lookup will only move forward in the object. In particular,
   *   the following code reads z, then y, then x, and thus will not retrieve x or y if fed the
   *   JSON `{ "x": 1, "y": 2, "z": 3 }`:
   *
   *   ```c++
   *   simdjson::builtin::ondemand::parser parser;
   *   auto obj = parser.parse(R"( { "x": 1, "y": 2, "z": 3 } )"_padded);
   *   double z = obj["z"];
   *   double y = obj["y"];
   *   double x = obj["x"];
   *   ```
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_really_inline simdjson_result<value> operator[](std::string_view key) & noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> operator[](std::string_view key) & noexcept; */
  simdjson_really_inline simdjson_result<value> operator[](std::string_view key) && noexcept;

protected:
  static simdjson_really_inline simdjson_result<object> start(value_iterator &iter) noexcept;
  static simdjson_really_inline simdjson_result<object> try_start(value_iterator &iter) noexcept;
  static simdjson_really_inline object started(value_iterator &iter) noexcept;
  static simdjson_really_inline object resume(const value_iterator &iter) noexcept;
  simdjson_really_inline object(const value_iterator &iter) noexcept;

  simdjson_warn_unused simdjson_really_inline error_code find_field_raw(const std::string_view key) noexcept;

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
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::object &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> begin() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object_iterator> end() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) && noexcept;
};

} // namespace simdjson
