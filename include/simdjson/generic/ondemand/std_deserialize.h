#if SIMDJSON_SUPPORTS_DESERIALIZATION

#ifndef SIMDJSON_ONDEMAND_DESERIALIZE_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_ONDEMAND_DESERIALIZE_H
#include "simdjson/generic/ondemand/array.h"
#include "simdjson/generic/ondemand/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <concepts>
#include <limits>

namespace simdjson {
template <typename T>
constexpr bool require_custom_serialization = false;

//////////////////////////////
// Number deserialization
//////////////////////////////

template <std::unsigned_integral T>
  requires(!require_custom_serialization<T>)
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
  requires(!require_custom_serialization<T>)
error_code tag_invoke(deserialize_tag, auto &val, T &out) noexcept {
  double x;
  SIMDJSON_TRY(val.get_double().get(x));
  out = static_cast<T>(x);
  return SUCCESS;
}

template <std::signed_integral T>
  requires(!require_custom_serialization<T>)
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

template <concepts::constructible_from_string_view T, typename ValT>
  requires(!require_custom_serialization<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept(std::is_nothrow_constructible_v<T, std::string_view>) {
  std::string_view str;
  SIMDJSON_TRY(val.get_string().get(str));
  out = T{str};
  return SUCCESS;
}


/**
 * STL containers have several constructors including one that takes a single
 * size argument. Thus, some compilers (Visual Studio) will not be able to
 * disambiguate between the size and container constructor. Users should
 * explicitly specify the type of the container as needed: e.g.,
 * doc.get<std::vector<int>>().
 */
template <concepts::appendable_containers T, typename ValT>
  requires(!require_custom_serialization<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept(false) {
  using value_type = typename std::remove_cvref_t<T>::value_type;
  static_assert(
      deserializable<value_type, ValT>,
      "The specified type inside the container must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<value_type>,
      "The specified type inside the container must default constructible.");

  SIMDJSON_IMPLEMENTATION::ondemand::array arr;
  SIMDJSON_TRY(val.get_array().get(arr));
  for (auto v : arr) {
    if constexpr (concepts::returns_reference<T>) {
      if (auto const err = v.get<value_type>().get(concepts::emplace_one(out));
          err) {
        // If an error occurs, the empty element that we just inserted gets
        // removed. We're not using a temp variable because if T is a heavy
        // type, we want the valid path to be the fast path and the slow path be
        // the path that has errors in it.
        if constexpr (requires { out.pop_back(); }) {
          static_cast<void>(out.pop_back());
        }
        return err;
      }
    } else {
      value_type temp;
      if (auto const err = v.get<value_type>().get(temp); err) {
        return err;
      }
      concepts::emplace_one(out, std::move(temp));
    }
  }
  return SUCCESS;
}


/**
 * We want to support std::map and std::unordered_map but only for
 * string-keyed types.
 */
 template <concepts::string_view_keyed_map T, typename ValT>
 requires(!require_custom_serialization<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept(false) {
  using value_type = typename std::remove_cvref_t<T>::mapped_type;
  static_assert(
     deserializable<value_type, ValT>,
     "The specified value type inside the container must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<value_type>,
      "The specified value type inside the container must default constructible.");
 SIMDJSON_IMPLEMENTATION::ondemand::object obj;
 SIMDJSON_TRY(val.get_object().get(obj));
 for (auto field : obj) {
    std::string_view key;
    SIMDJSON_TRY(field.unescaped_key().get(key));
    value_type this_value;
    SIMDJSON_TRY(field.value().get<value_type>().get(this_value));
    [[maybe_unused]] std::pair<typename T::iterator, bool> result = out.emplace(key, this_value);
    // unclear what to do if the key already exists
    // if (result.second == false) {
    //   // key already exists
    // }
 }
 (void)out;
 return SUCCESS;
}




/**
 * This CPO (Customization Point Object) will help deserialize into
 * smart pointers.
 *
 * If constructing T is nothrow, this conversion should be nothrow as well since
 * we return MEMALLOC if we're not able to allocate memory instead of throwing
 * the error message.
 *
 * @tparam T The type inside the smart pointer
 * @tparam ValT document/value type
 * @param val document/value
 * @param out a reference to the smart pointer
 * @return status of the conversion
 */
template <concepts::smart_pointer T, typename ValT>
  requires(!require_custom_serialization<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept(nothrow_deserializable<typename std::remove_cvref_t<T>::element_type, ValT>) {
  using element_type = typename std::remove_cvref_t<T>::element_type;

  // For better error messages, don't use these as constraints on
  // the tag_invoke CPO.
  static_assert(
      deserializable<element_type, ValT>,
      "The specified type inside the unique_ptr must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<element_type>,
      "The specified type inside the unique_ptr must default constructible.");

  auto ptr = new (std::nothrow) element_type();
  if (ptr == nullptr) {
    return MEMALLOC;
  }
  SIMDJSON_TRY(val.template get<element_type>(*ptr));
  out.reset(ptr);
  return SUCCESS;
}

/**
 * This CPO (Customization Point Object) will help deserialize into optional types.
 */
template <concepts::optional_type T, typename ValT>
  requires(!require_custom_serialization<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept(nothrow_deserializable<typename std::remove_cvref_t<T>::value_type, ValT>) {
  using value_type = typename std::remove_cvref_t<T>::value_type;

  static_assert(
      deserializable<value_type, ValT>,
      "The specified type inside the unique_ptr must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<value_type>,
      "The specified type inside the unique_ptr must default constructible.");

  if (!out) {
    out.emplace();
  }
  SIMDJSON_TRY(val.template get<value_type>(out.value()));
  return SUCCESS;
}

} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_DESERIALIZE_H
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION
