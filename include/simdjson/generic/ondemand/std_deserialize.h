#if SIMDJSON_SUPPORTS_DESERIALIZATION

#ifndef SIMDJSON_ONDEMAND_DESERIALIZE_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_ONDEMAND_DESERIALIZE_H
#include "simdjson/generic/ondemand/object.h"
#include "simdjson/generic/ondemand/array.h"
#include "simdjson/generic/ondemand/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <concepts>
#include <limits>
#if SIMDJSON_STATIC_REFLECTION
#include <experimental/meta>
#endif

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

//////////////////////////////
// String deserialization
//////////////////////////////

error_code tag_invoke(deserialize_tag, auto &val, char &out) noexcept {
  std::string_view x;
  SIMDJSON_TRY(val.get_string().get(x));
  if(x.size() != 1) {
    return INCORRECT_TYPE;
  }
  out = x[0];
  return SUCCESS;
}


error_code tag_invoke(deserialize_tag, auto &val, std::string &out) noexcept {
  std::string_view x;
  SIMDJSON_TRY(val.get_string().get(x));
  out.assign(x.data(), x.size());
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
  /*static_assert(
      deserializable<value_type, ValT>,
      "The specified type inside the container must itself be deserializable");*/
  static_assert(
      std::is_default_constructible_v<value_type>,
      "The specified type inside the container must default constructible.");

  SIMDJSON_IMPLEMENTATION::ondemand::array arr;
  if constexpr (std::is_same_v<std::remove_cvref_t<ValT>, SIMDJSON_IMPLEMENTATION::ondemand::array>) {
    arr = val;
  } else {
    SIMDJSON_TRY(val.get_array().get(arr));
  }

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


#if SIMDJSON_STATIC_REFLECTION


template <typename T>
constexpr bool user_defined_type = (std::is_class_v<T>
&& !std::is_same_v<T, std::string> && !std::is_same_v<T, std::string_view> && !concepts::optional_type<T> &&
!concepts::appendable_containers<T> && !require_custom_serialization<T>);


// workaround from
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r3.html#back-and-forth
// for missing expansion statements
namespace __impl {
template <auto... vals> struct replicator_type {
  template <typename F> constexpr void operator>>(F body) const {
    (body.template operator()<vals>(), ...);
  }
};

template <auto... vals> replicator_type<vals...> replicator = {};
} // namespace __impl

template <typename R> consteval auto expand(R range) {
  std::vector<std::meta::info> args;
  for (auto r : range) {
    args.push_back(std::meta::reflect_value(r));
  }
  return substitute(^__impl::replicator, args);
}
// end of workaround

template <typename T, typename ValT>
  requires(user_defined_type<T> && std::is_class_v<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept {
  SIMDJSON_IMPLEMENTATION::ondemand::object obj;
  if constexpr (std::is_same_v<std::remove_cvref_t<ValT>, SIMDJSON_IMPLEMENTATION::ondemand::object>) {
    obj = val;
  } else {
    SIMDJSON_TRY(val.get_object().get(obj));
  }
  error_code e = simdjson::SUCCESS;

  [:expand(std::meta::nonstatic_data_members_of(^T)):] >> [&]<auto mem> {
    constexpr std::string_view key = std::string_view(std::meta::identifier_of(mem));
    static_assert(
      deserializable<decltype(out.[:mem:]), SIMDJSON_IMPLEMENTATION::ondemand::object>,
      "The specified type inside the class must itself be deserializable");
    // as long we are succesful or the field is not found, we continue
    if(e == simdjson::SUCCESS || e == simdjson::NO_SUCH_FIELD) {
      obj[key].get(out.[:mem:]);
    }
  };
  /**  TODO: we need to migrate to the following code once the compiler supports it:
  template for (constexpr auto mem : std::meta::nonstatic_data_members_of(^T)) {
    constexpr std::string_view key = std::string_view(std::meta::identifier_of(mem));

  }
  */
  return e;
}
template <typename simdjson_value, typename T>
  requires(user_defined_type<std::remove_cvref_t<T>>)
error_code tag_invoke(deserialize_tag, simdjson_value &val, std::unique_ptr<T> &out) noexcept {
  if (!out) {
    out = std::make_unique<T>();
    if (!out) {
      return MEMALLOC;
    }
  }
  if (auto err = val.get(*out)) {
    out.reset();
    return err;
  }
  return SUCCESS;
}

template <typename simdjson_value, typename T>
  requires(user_defined_type<std::remove_cvref_t<T>>)
error_code tag_invoke(deserialize_tag, simdjson_value &val, std::shared_ptr<T> &out) noexcept {
  if (!out) {
    out = std::make_shared<T>();
    if (!out) {
      return MEMALLOC;
    }
  }
  if (auto err = val.get(*out)) {
    out.reset();
    return err;
  }
  return SUCCESS;
}

#endif // SIMDJSON_STATIC_REFLECTION

////////////////////////////////////////
// Unique pointers
////////////////////////////////////////
error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<bool> &out) noexcept {
  if (!out) {
    out = std::make_unique<bool>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_bool().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<int64_t> &out) noexcept {
  if (!out) {
    out = std::make_unique<int64_t>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_int64().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<uint64_t> &out) noexcept {
  if (!out) {
    out = std::make_unique<uint64_t>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_uint64().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<double> &out) noexcept {
  if (!out) {
    out = std::make_unique<double>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_double().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<std::string_view> &out) noexcept {
  if (!out) {
    out = std::make_unique<std::string_view>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_string().get(*out));
  return SUCCESS;
}


////////////////////////////////////////
// Shared pointers
////////////////////////////////////////
error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<bool> &out) noexcept {
  if (!out) {
    out = std::make_shared<bool>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_bool().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<int64_t> &out) noexcept {
  if (!out) {
    out = std::make_shared<int64_t>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_int64().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<uint64_t> &out) noexcept {
  if (!out) {
    out = std::make_shared<uint64_t>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_uint64().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<double> &out) noexcept {
  if (!out) {
    out = std::make_shared<double>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_double().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<std::string_view> &out) noexcept {
  if (!out) {
    out = std::make_shared<std::string_view>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_string().get(*out));
  return SUCCESS;
}


} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_DESERIALIZE_H
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION
