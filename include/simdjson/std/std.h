#ifndef SIMDJSON_STD_STD_H
#define SIMDJSON_STD_STD_H

#ifndef SIMDJSON_SUPPORTS_DESERIALIZATION
#include "simdjson/generic/ondemand/deserialize.h"
#endif

#ifdef SIMDJSON_SUPPORTS_DESERIALIZATION
#include <concepts>
#include <limits>

namespace simdjson {

template <std::unsigned_integral T>
error_code tag_invoke(deserialize_tag, auto &val, T &out) noexcept {
  using limits = std::numeric_limits<T>;

  uint64_t x;
  SIMDJSON_TRY(val.get_uint64().get(x));
  if (x > limits::max()) {
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
  if (x > limits::max() || x < limits::min()) {
    return NUMBER_OUT_OF_RANGE;
  }
  out = static_cast<T>(x);
  return SUCCESS;
}

} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_DESERIALIZATION

#endif // SIMDJSON_STD_STD_H
