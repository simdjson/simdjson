#if SIMDJSON_SUPPORTS_CONCEPTS

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
#include <meta>
// #include <static_reflection> // for std::define_static_string - header not available yet
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

// just a character!
error_code tag_invoke(deserialize_tag, auto &val, char &out) noexcept {
  std::string_view x;
  SIMDJSON_TRY(val.get_string().get(x));
  if(x.size() != 1) {
    return INCORRECT_TYPE;
  }
  out = x[0];
  return SUCCESS;
}

// any string-like type (can be constructed from std::string_view)
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

template <concepts::string_view_keyed_map T>
error_code tag_invoke(deserialize_tag, SIMDJSON_IMPLEMENTATION::ondemand::object &obj, T &out) noexcept {
  using value_type = typename std::remove_cvref_t<T>::mapped_type;

  out.clear();
  for (auto field : obj) {
    std::string_view key;
    SIMDJSON_TRY(field.unescaped_key().get(key));

    SIMDJSON_IMPLEMENTATION::ondemand::value value_obj;
    SIMDJSON_TRY(field.value().get(value_obj));

    value_type this_value;
    SIMDJSON_TRY(value_obj.get(this_value));
    out.emplace(typename T::key_type(key), std::move(this_value));
  }
  return SUCCESS;
}

template <concepts::string_view_keyed_map T>
error_code tag_invoke(deserialize_tag, SIMDJSON_IMPLEMENTATION::ondemand::value &val, T &out) noexcept {
  SIMDJSON_IMPLEMENTATION::ondemand::object obj;
  SIMDJSON_TRY(val.get_object().get(obj));
  return simdjson::deserialize(obj, out);
}

template <concepts::string_view_keyed_map T>
error_code tag_invoke(deserialize_tag, SIMDJSON_IMPLEMENTATION::ondemand::document &doc, T &out) noexcept {
  SIMDJSON_IMPLEMENTATION::ondemand::object obj;
  SIMDJSON_TRY(doc.get_object().get(obj));
  return simdjson::deserialize(obj, out);
}

template <concepts::string_view_keyed_map T>
error_code tag_invoke(deserialize_tag, SIMDJSON_IMPLEMENTATION::ondemand::document_reference &doc, T &out) noexcept {
  SIMDJSON_IMPLEMENTATION::ondemand::object obj;
  SIMDJSON_TRY(doc.get_object().get(obj));
  return simdjson::deserialize(obj, out);
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
template <concepts::optional_type T>
  requires(!require_custom_serialization<T>)
error_code tag_invoke(deserialize_tag, auto &val, T &out) noexcept(nothrow_deserializable<typename std::remove_cvref_t<T>::value_type, decltype(val)>) {
  using value_type = typename std::remove_cvref_t<T>::value_type;

  // Check if the value is null
  if (val.is_null()) {
    out.reset(); // Set to nullopt
    return SUCCESS;
  }

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
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r10.html#back-and-forth
// for missing expansion statements
namespace __impl {
  template<auto... vals>
  struct replicator_type {
    template<typename F>
      constexpr void operator>>(F body) const {
        (body.template operator()<vals>(), ...);
      }
  };

  template<auto... vals>
  replicator_type<vals...> replicator = {};
}

template<typename R>
consteval auto expand(R range) {
  std::vector<std::meta::info> args;
  for (auto r : range) {
    args.push_back(reflect_constant(r));
  }
  return substitute(^^__impl::replicator, args);
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

  [:expand(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())):] >> [&]<auto mem>() {
    if constexpr (!std::meta::is_const(mem) && std::meta::is_public(mem)) {
      constexpr std::string_view key = std::define_static_string(std::meta::identifier_of(mem));
      // Note: removed static assert as optional types are now handled generically
      // as long we are succesful or the field is not found, we continue
      if(e == simdjson::SUCCESS || e == simdjson::NO_SUCH_FIELD) {
        e = obj[key].get(out.[:mem:]);
      }
    }
  };
  return e;
}

// Support for enum deserialization - deserialize from string representation using expand approach from P2996R12
template <typename T, typename ValT>
  requires(std::is_enum_v<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept {
#if SIMDJSON_STATIC_REFLECTION
  std::string_view str;
  SIMDJSON_TRY(val.get_string().get(str));

  bool found = false;
  [:expand(std::meta::enumerators_of(^^T)):] >> [&]<auto enum_val>{
    if (!found && str == std::meta::identifier_of(enum_val)) {
      out = [:enum_val:];
      found = true;
    }
  };

  return found ? SUCCESS : INCORRECT_TYPE;
#else
  // Fallback: deserialize as integer if reflection not available
  std::underlying_type_t<T> int_val;
  SIMDJSON_TRY(val.get(int_val));
  out = static_cast<T>(int_val);
  return SUCCESS;
#endif
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
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
  if (!out) {
    out = std::make_unique<bool>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_bool().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<int64_t> &out) noexcept {
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
  if (!out) {
    out = std::make_unique<int64_t>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_int64().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<uint64_t> &out) noexcept {
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
  if (!out) {
    out = std::make_unique<uint64_t>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_uint64().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<double> &out) noexcept {
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
  if (!out) {
    out = std::make_unique<double>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_double().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<std::string_view> &out) noexcept {
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
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
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
  if (!out) {
    out = std::make_shared<bool>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_bool().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<int64_t> &out) noexcept {
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
  if (!out) {
    out = std::make_shared<int64_t>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_int64().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<uint64_t> &out) noexcept {
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
  if (!out) {
    out = std::make_shared<uint64_t>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_uint64().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<double> &out) noexcept {
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
  if (!out) {
    out = std::make_shared<double>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_double().get(*out));
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<std::string_view> &out) noexcept {
  if (val.is_null()) {
    out.reset();
    return SUCCESS;
  }
  if (!out) {
    out = std::make_shared<std::string_view>();
    if (!out) { return MEMALLOC; }
  }
  SIMDJSON_TRY(val.get_string().get(*out));
  return SUCCESS;
}


////////////////////////////////////////
// Explicit optional specializations
////////////////////////////////////////

////////////////////////////////////////
// Explicit smart pointer specializations for string and int types
////////////////////////////////////////
error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<std::string> &out) noexcept {
  // Check if the value is null
  if (val.is_null()) {
    out.reset(); // Set to nullptr
    return SUCCESS;
  }

  if (!out) {
    out = std::make_unique<std::string>();
  }
  std::string_view str;
  SIMDJSON_TRY(val.get_string().get(str));
  *out = std::string{str};
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::shared_ptr<std::string> &out) noexcept {
  // Check if the value is null
  if (val.is_null()) {
    out.reset(); // Set to nullptr
    return SUCCESS;
  }

  if (!out) {
    out = std::make_shared<std::string>();
  }
  std::string_view str;
  SIMDJSON_TRY(val.get_string().get(str));
  *out = std::string{str};
  return SUCCESS;
}

error_code tag_invoke(deserialize_tag, auto &val, std::unique_ptr<int> &out) noexcept {
  // Check if the value is null
  if (val.is_null()) {
    out.reset(); // Set to nullptr
    return SUCCESS;
  }

  if (!out) {
    out = std::make_unique<int>();
  }
  int64_t temp;
  SIMDJSON_TRY(val.get_int64().get(temp));
  *out = static_cast<int>(temp);
  return SUCCESS;
}

} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_DESERIALIZE_H
#endif // SIMDJSON_SUPPORTS_CONCEPTS
