#if SIMDJSON_SUPPORTS_DESERIALIZATION

#ifndef SIMDJSON_ONDEMAND_DESERIALIZE_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_ONDEMAND_DESERIALIZE_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/array.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <concepts>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace simdjson {

//////////////////////////////
// Number deserialization
//////////////////////////////
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

//////////////////////////////
// STL list deserialization
//////////////////////////////

template <typename T, typename AllocT, typename ValT>
error_code tag_invoke(deserialize_tag, ValT &val,
                      std::list<T, AllocT> &out) noexcept(false) {

  // For better error messages, don't use these as constraints on
  // the tag_invoke CPO.
  static_assert(
      deserializable<T, ValT>,
      "The specified type inside the list must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<T>,
      "The specified type inside the list must default constructible.");

  SIMDJSON_IMPLEMENTATION::ondemand::array arr;
  SIMDJSON_TRY(val.get_array().get(arr));
  for (auto v : arr) {
    if (auto const err = v.get<T>().get(out.emplace_back()); err) {
      // If an error occurs, the empty element that we just inserted gets
      // removed. We're not using a temp variable because if T is a heavy type,
      // we want the valid path to be the fast path and the slow path be the
      // path that has errors in it.
      static_cast<void>(out.pop_back());
      return err;
    }
  }
  return SUCCESS;
}

//////////////////////////////
// std::string deserialization
//////////////////////////////

template <typename CharT,
          typename TraitsT,
          typename AllocT,
          typename ValT>
error_code tag_invoke(deserialize_tag, ValT &val, std::basic_string<CharT, TraitsT, AllocT> &out) noexcept(false) {
  using string_type = std::basic_string<CharT, TraitsT, AllocT>;

  if constexpr (std::same_as<string_type, string_type>) {
    SIMDJSON_TRY(val.get_string(out));
  } else {
    // todo: optimize performance
    std::string tmp;
    SIMDJSON_TRY(val.get_string(tmp));
    for (auto const ch : tmp) {
      out.push_back(ch);
    }
  }
  return SUCCESS;
}

//////////////////////////////
// STL Vector deserialization
//////////////////////////////

/**
 * STL containers have several constructors including one that takes a single
 * size argument. Thus, some compilers (Visual Studio) will not be able to
 * disambiguate between the size and container constructor. Users should
 * explicitly specify the type of the container as needed: e.g.,
 * doc.get<std::vector<int>>().
 */
template <typename T, typename AllocT, typename ValT>
error_code tag_invoke(deserialize_tag, ValT &val,
                      std::vector<T, AllocT> &out) noexcept(false) {

  // For better error messages, don't use these as constraints on
  // the tag_invoke CPO.
  static_assert(
      deserializable<T, ValT>,
      "The specified type inside the vector must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<T>,
      "The specified type inside the vector must default constructible.");

  SIMDJSON_IMPLEMENTATION::ondemand::array arr;
  SIMDJSON_TRY(val.get_array().get(arr));
  for (auto v : arr) {
    if (auto const err = v.get<T>().get(out.emplace_back()); err) {
      // If an error occurs, the empty element that we just inserted gets
      // removed. We're not using a temp variable because if T is a heavy type,
      // we want the valid path to be the fast path and the slow path be the
      // path that has errors in it.
      static_cast<void>(out.pop_back());
      return err;
    }
  }
  return SUCCESS;
}

//////////////////////////////
// std::unique_ptr deserialization
//////////////////////////////
/**
 * This CPO (Customization Point Object) will help deserialize into
 * `unique_ptr`s.
 *
 * If constructing T is nothrow, this conversion should be nothrow as well since
 * we return MEMALLOC if we're not able to allocate memory instead of throwing
 * the the error message.
 *
 * @tparam T The type inside the unique_ptr
 * @tparam Deleter The Deleter of the unique_ptr
 * @tparam ValT document/value type
 * @param val document/value
 * @param out output unique_ptr
 * @return status of the conversion
 */
template <typename T, typename Deleter, typename ValT>
error_code tag_invoke(deserialize_tag, ValT &val,
                      std::unique_ptr<T, Deleter>
                          &out) noexcept(nothrow_deserializable<T, ValT>) {

  // For better error messages, don't use these as constraints on
  // the tag_invoke CPO.
  static_assert(
      deserializable<T, ValT>,
      "The specified type inside the unique_ptr must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<T>,
      "The specified type inside the unique_ptr must default constructible.");

  auto ptr = new (std::nothrow) T();
  if (ptr == nullptr) {
    return MEMALLOC;
  }
  SIMDJSON_TRY(val.template get<T>(*ptr));
  out.reset(ptr);
  return SUCCESS;
}
} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_DESERIALIZE_H
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION