#if SIMDJSON_SUPPORTS_DESERIALIZATION

#ifndef SIMDJSON_ONDEMAND_DESERIALIZE_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_ONDEMAND_DESERIALIZE_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/array.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <concepts>
#include <limits>
#include <string>

namespace simdjson {
template <typename T>
constexpr bool require_custom_serialization = false;

//////////////////////////////
// Number deserialization
//////////////////////////////

template <std::unsigned_integral T>
error_code tag_invoke(deserialize_tag, auto &val, T &out) noexcept {
  using limits = std::numeric_limits<T>;

  uint64_t x;
  SIMDJSON_TRY(val.get_uint64().get(x));
  if (x > (limits::max)()) {
    return NUMBER_OUT_OF_RANGE;
  }
  out = static_cast<T>(x);
  return SUCCESS;
}

template <std::floating_point T>
error_code tag_invoke(deserialize_tag, auto &val, T &out) noexcept {
  double x;
  SIMDJSON_TRY(val.get_double().get(x));
  out = static_cast<T>(x);
  return SUCCESS;
}

template <std::signed_integral T>
error_code tag_invoke(deserialize_tag, auto &val, T &out) noexcept {
  using limits = std::numeric_limits<T>;

  int64_t x;
  SIMDJSON_TRY(val.get_int64().get(x));
  if (x > (limits::max)() || x < (limits::min)()) {
    return NUMBER_OUT_OF_RANGE;
  }
  out = static_cast<T>(x);
  return SUCCESS;
}
//template <typename>
//struct is_deserialization_enabled : std::true_type {};
//template <typename>
//consteval bool allows_automated_serialization() { return true; }

/**
 * STL containers have several constructors including one that takes a single
 * size argument. Thus, some compilers (Visual Studio) will not be able to
 * disambiguate between the size and container constructor. Users should
 * explicitly specify the type of the container as needed: e.g.,
 * doc.get<std::vector<int>>().
 */
template <typename T, typename ValT>
  requires simdjson::concepts::pushable_container<T>
error_code tag_invoke(deserialize_tag, ValT &val,
                      T &out) noexcept(false) {
  using value_type = typename T::value_type;
  static_assert(
      !require_custom_serialization<T>,
      "You have overridden require_custom_serialization<T> and it returns false. ");
  static_assert(
      deserializable<value_type, ValT>,
      "The specified type inside the container must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<value_type>,
      "The specified type inside the container must default constructible.");

  SIMDJSON_IMPLEMENTATION::ondemand::array arr;
  SIMDJSON_TRY(val.get_array().get(arr));
  for (auto v : arr) {
    value_type temp;
    if (auto const err = v.get<value_type>().get(temp); err) {
      return err;
    }
    out.push_back(temp); // note that this can be done with emplace_back for better performance
  }
  return SUCCESS;
}
} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_DESERIALIZE_H
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION
