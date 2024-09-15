#ifndef SIMDJSON_TAG_INVOKE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_TAG_INVOKE_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/deserialize.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE
#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#ifdef __cpp_concepts
#include <utility>
#include <vector>
#include <list>

/**
 * Provides convenience functions for deserialization of common types.
 */
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

template <std::unsigned_integral T>
error_code tag_invoke(deserialize_tag, auto &val, T& out) noexcept {
  uint64_t x;
  SIMDJSON_TRY(val.get_uint64().get(x));
  if(x > (std::numeric_limits<T>::max)() || x < (std::numeric_limits<T>::min)()) {
    return NUMBER_OUT_OF_RANGE;
  }
  out = static_cast<T>(x);
  return SUCCESS;
}

template <std::floating_point T>
error_code tag_invoke(deserialize_tag, auto &val, T& out) noexcept {
  double x;
  SIMDJSON_TRY(val.get_double().get(x));
  out = static_cast<T>(x);
  return SUCCESS;
}

template <std::signed_integral T>
error_code tag_invoke(deserialize_tag, auto &val, T& out) noexcept {
  int64_t x;
  SIMDJSON_TRY(val.get_int64().get(x));
  if(x > (std::numeric_limits<T>::max)() || x < (std::numeric_limits<T>::min)()) {
    return NUMBER_OUT_OF_RANGE;
  }
  out = static_cast<T>(x);
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::string& out) noexcept {
  SIMDJSON_TRY(val.get_string(out));
  return SUCCESS;
}

/**
 * STL containers have several constructors including one that takes a single
 * size argument. Thus some compilers (Visual Studio) will not be able to
 * disambiguate between the size and container constructor. Users should
 * explicitly specify the type of the container as needed: e.g.,
 * doc.get<std::vector<int>>().
 */

template <typename T, typename AllocT = std::allocator<T>>
error_code tag_invoke(deserialize_tag, auto &val, std::vector<T, AllocT>& out) noexcept {
  array array;
  SIMDJSON_TRY(val.get_array().get(array));
  for (auto v : array) {
    T value;
    SIMDJSON_TRY(v.get<T>().get(value));
    out.push_back(value);
  }
  return SUCCESS;
}

template <typename T, typename AllocT = std::allocator<T>>
error_code tag_invoke(deserialize_tag, auto &val, std::list<T, AllocT>& out) noexcept {
  array array;
  SIMDJSON_TRY(val.get_array().get(array));
  for (auto v : array) {
    T value;
    SIMDJSON_TRY(v.get<T>().get(value));
    out.push_back(value);
  }
  return SUCCESS;
}
}
}
}
#endif // __cpp_concepts
#endif // SIMDJSON_TAG_INVOKE_H
