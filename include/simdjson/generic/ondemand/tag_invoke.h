#ifndef SIMDJSON_TAG_INVOKE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_TAG_INVOKE_H
#include "simdjson/generic/ondemand/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE
#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#ifdef __cpp_concepts
#include <utility>
/**
 * Provides convenience functions for deserialization of common types.
 */
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

template <typename T>
  requires std::unsigned_integral<T>
simdjson_result<T> tag_invoke(deserialize_tag, std::type_identity<T>, auto &val) noexcept {
  uint64_t x;
  SIMDJSON_TRY(val.get_uint64().get(x));
  if(x > std::numeric_limits<T>::max() || x < std::numeric_limits<T>::min()) {
    return NUMBER_OUT_OF_RANGE;
  }
  return static_cast<T>(x);
}

template <typename T>
  requires std::floating_point<T>
simdjson_result<T> tag_invoke(deserialize_tag, std::type_identity<T>, auto &val) noexcept{
  double x;
  SIMDJSON_TRY(val.get_double().get(x));
  return static_cast<T>(x);
}

template <typename T>
  requires std::signed_integral<T>
simdjson_result<T> tag_invoke(simdjson::deserialize_tag, std::type_identity<T>, auto &val) noexcept {
  int64_t x;
  SIMDJSON_TRY(val.get_int64().get(x));
  if(x > std::numeric_limits<T>::max() || x < std::numeric_limits<T>::min()) {
    return NUMBER_OUT_OF_RANGE;
  }
  return static_cast<T>(x);
}


template <>
simdjson_result<std::string> tag_invoke(deserialize_tag, std::type_identity<std::string>, auto &val) noexcept{
  std::string s;
  SIMDJSON_TRY(val.get_string(s));
  return s;
}

template <typename T>
  requires concepts::std_vector<T>
simdjson_result<T> tag_invoke(deserialize_tag, std::type_identity<T>, auto &val) {
  T vec;
  array array;
  SIMDJSON_TRY(val.get_array().get(array));
  for (auto v : array) {
    typename T::value_type value;
    SIMDJSON_TRY(v.get<typename T::value_type>().get(value));
    vec.push_back(value);
  }
  return vec;
}
}
}
}
#endif // __cpp_concepts
#endif // SIMDJSON_TAG_INVOKE_H