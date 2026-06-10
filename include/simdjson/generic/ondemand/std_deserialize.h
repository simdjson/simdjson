#if SIMDJSON_SUPPORTS_CONCEPTS

#ifndef SIMDJSON_ONDEMAND_DESERIALIZE_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_ONDEMAND_DESERIALIZE_H
#include "simdjson/generic/ondemand/object.h"
#include "simdjson/generic/ondemand/array.h"
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/annotations.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <concepts>
#include <limits>
#if SIMDJSON_STATIC_REFLECTION
#include <meta>
#include <vector>
// #include <static_reflection> // for std::define_static_string - header not available yet
#endif

namespace simdjson {

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
error_code tag_invoke(deserialize_tag, auto &val, T &out) noexcept(nothrow_deserializable<typename std::remove_cvref_t<T>::value_type, decltype(val)>) {
  using value_type = typename std::remove_cvref_t<T>::value_type;

  // Check if the value is null
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
!concepts::appendable_containers<T>);


// key_selector_reflection_detail is defined unconditionally (it only requires
// static reflection). It provides both the compile-time machinery for building a
// key_selector from a struct's members and the ordered per-member fallback used
// by the opt-out build and as an automatic fallback (see deserialize_struct_ordered
// and keys_fit_selector below).
namespace key_selector_reflection_detail {

// A member participates if it is public, non-const, and not annotated to skip.
consteval bool is_eligible_member(std::meta::info mem) {
  return !std::meta::is_const(mem) && std::meta::is_public(mem)
      && std::meta::annotations_of_with_type(mem, ^^simdjson::detail::skip_tag).empty();
}

// JSON key name for `mem`, as a constevalutil::fixed_string usable as an NTTP.
template <auto mem>
consteval auto member_key_fixed_string() {
  constexpr std::string_view key{ simdjson::get_json_key_name<mem>() };
  char buffer[key.size() + 1] = {};
  for (std::size_t i = 0; i < key.size(); ++i) { buffer[i] = key[i]; }
  return constevalutil::fixed_string<key.size() + 1>(buffer);
}

// key_selector template arguments (one fixed_string per eligible member), in
// declaration order.
template <typename T>
consteval std::vector<std::meta::info> selector_key_args() {
  std::vector<std::meta::info> args;
  template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (is_eligible_member(mem)) {
      args.push_back(std::meta::reflect_constant(member_key_fixed_string<mem>()));
    }
  }
  return args;
}

// key_selector whose keys are exactly T's eligible members (index i <-> i-th).
template <typename T>
using selector_for = typename [: std::meta::substitute(
    ^^SIMDJSON_IMPLEMENTATION::ondemand::key_selector, selector_key_args<T>()) :];

// Number of members that participate in deserialization. A class can have zero
// eligible members (e.g. std::chrono::time_point, whose only data member is
// private): an empty key_selector cannot be built, so the tag_invoke below
// special-cases this count.
template <typename T>
consteval std::size_t eligible_member_count() {
  std::size_t count = 0;
  template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (is_eligible_member(mem)) {
      ++count;
    }
  }
  return count;
}

// True when none of T's eligible members is an optional type, i.e. every member
// is required. In that case presence can be checked with a single match count
// instead of a per-member "seen" array.
template <typename T>
consteval bool all_eligible_members_required() {
  bool all_required = true;
  template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (is_eligible_member(mem)) {
      if constexpr (concepts::optional_type<typename [: std::meta::type_of(mem) :]>) {
        all_required = false;
      }
    }
  }
  return all_required;
}

// True when T's eligible member keys satisfy every key_selector requirement, so
// a key_selector can be built for T without a compile-time error. This mirrors
// the key_selector limits (see key_selector.h): at most 255 keys, each key
// non-empty and at most 63 characters, no backslash / double-quote / null byte,
// and all keys distinct. When this returns false the deserializer falls back to
// the ordered per-member path instead of failing to compile.
template <typename T>
consteval bool keys_fit_selector() {
  std::vector<std::string_view> keys;
  template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (is_eligible_member(mem)) {
      keys.push_back(std::string_view{ simdjson::get_json_key_name<mem>() });
    }
  }
  if (keys.size() > 255) { return false; }
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (keys[i].empty() || keys[i].size() > 63) { return false; }
    for (char c : keys[i]) {
      if (c == '\\' || c == '"' || c == '\0') { return false; }
    }
    for (std::size_t j = i + 1; j < keys.size(); ++j) {
      if (keys[i] == keys[j]) { return false; }
    }
  }
  return true;
}

// Ordered, per-member deserialization: one obj[key] lookup per eligible member.
// This is the opt-out path (-DSIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION=1) and the
// automatic fallback for structs whose keys do not fit the key_selector limits
// (see keys_fit_selector).
template <typename T>
simdjson_warn_unused error_code deserialize_struct_ordered(
    SIMDJSON_IMPLEMENTATION::ondemand::object &obj, T &out) noexcept {
  template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (is_eligible_member(mem)) {
      constexpr std::string_view key = simdjson::get_json_key_name<mem>();
      if constexpr (concepts::optional_type<decltype(out.[:mem:])>) {
        // For optional members, a missing key is not an error: leave the member
        // at its current (default) value.
        auto error = obj[key].get(out.[:mem:]);
        if (error && error != NO_SUCH_FIELD) { return error; }
      } else {
        // For non-optional members, the key must be present.
        SIMDJSON_TRY(obj[key].get(out.[:mem:]));
      }
    }
  }
  return SUCCESS;
}

} // namespace key_selector_reflection_detail

// Deserialize a reflected struct. By default this builds a compile-time
// key_selector from the struct's members and walks each object once with
// object::for_each (perfect-hash key matching), instead of one obj[key] lookup
// per member. There are two ways the ordered per-member path is used instead:
//   - globally, by defining -DSIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION=1;
//   - automatically and per-type, when the struct's member keys do not fit the
//     key_selector limits (see keys_fit_selector), so that long member names and
//     the like keep compiling rather than tripping a static_assert.
template <typename T, typename ValT>
  requires(user_defined_type<T> && std::is_class_v<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept {
  SIMDJSON_IMPLEMENTATION::ondemand::object obj;
  if constexpr (std::is_same_v<std::remove_cvref_t<ValT>, SIMDJSON_IMPLEMENTATION::ondemand::object>) {
    obj = val;
  } else {
    SIMDJSON_TRY(val.get_object().get(obj));
  }
#if defined(SIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION) && SIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION
  // Opt-out build: always use the ordered per-member path.
  return key_selector_reflection_detail::deserialize_struct_ordered(obj, out);
#else
  if constexpr (key_selector_reflection_detail::eligible_member_count<T>() == 0) {
    // No members to deserialize: an empty key_selector cannot be built, so just
    // validate that the input is an object (done above) and succeed. Mirrors the
    // ordered per-member path, which iterates over zero members.
    (void)out;
    (void)obj;
    return SUCCESS;
  } else if constexpr (!key_selector_reflection_detail::keys_fit_selector<T>()) {
    // Automatic fallback: T's eligible member keys do not fit the key_selector
    // limits (e.g. a member name longer than 63 characters), so building a
    // selector would be a compile error. Use the ordered per-member path instead,
    // so the default never breaks a struct that the opt-out path would accept.
    return key_selector_reflection_detail::deserialize_struct_ordered(obj, out);
  } else {
  using selector = key_selector_reflection_detail::selector_for<T>;
  if constexpr (key_selector_reflection_detail::all_eligible_members_required<T>()) {
    // Fast path: every member is required. A single for_each pass parses each
    // matched field; the returned match count then tells us whether every member
    // was present (matched_count == selector::size()) without a per-member "seen"
    // array. A value-parse error (e.g. a type mismatch) is propagated by for_each.
    auto walk = obj.template for_each<selector>(
        [&](std::size_t matched_index, SIMDJSON_IMPLEMENTATION::ondemand::value field_value) -> error_code {
      std::size_t counter = 0;
      template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
        if constexpr (key_selector_reflection_detail::is_eligible_member(mem)) {
          if (matched_index == counter) { return field_value.get(out.[:mem:]); }
          ++counter;
        }
      }
      return SUCCESS;
    });
    if (walk.error) { return walk.error; }
    // A missing required member shows up as a short match count and is reported as
    // NO_SUCH_FIELD, mirroring the ordered obj[key] path.
    if (walk.matched_count != selector::size()) { return NO_SUCH_FIELD; }
    return SUCCESS;
  } else {
    std::array<bool, selector::size()> seen_member{};
    // Single pass over the object: each field whose key matches a member yields its
    // selector index, which we map back to the corresponding member. The callback
    // returns an error_code so that a value-parse error (e.g. a type mismatch on a
    // matched field) is propagated by for_each instead of being silently dropped.
    error_code walk_error = obj.template for_each<selector>(
        [&](std::size_t matched_index, SIMDJSON_IMPLEMENTATION::ondemand::value field_value) -> error_code {
      std::size_t counter = 0;
      error_code field_error = SUCCESS;
      template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
        if constexpr (key_selector_reflection_detail::is_eligible_member(mem)) {
          if (matched_index == counter) {
            seen_member[counter] = true;
            field_error = field_value.get(out.[:mem:]);
          }
          ++counter;
        }
      }
      return field_error;
    });
    if (walk_error) { return walk_error; }
    // Required (non-optional) members must be present: a missing one is reported as
    // NO_SUCH_FIELD, mirroring the ordered obj[key] path. Optional members may be
    // absent.
    std::size_t check_counter = 0;
    template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
      if constexpr (key_selector_reflection_detail::is_eligible_member(mem)) {
        if constexpr (!concepts::optional_type<decltype(out.[:mem:])>) {
          if (!seen_member[check_counter]) { return NO_SUCH_FIELD; }
        }
        ++check_counter;
      }
    }
    return SUCCESS;
  }
  }
#endif // SIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION
}

// Support for enum deserialization - deserialize from string representation using expand approach from P2996R12
template <typename T, typename ValT>
  requires(std::is_enum_v<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept {
#if SIMDJSON_STATIC_REFLECTION
  std::string_view str;
  SIMDJSON_TRY(val.get_string().get(str));
  static constexpr auto enumerators = std::define_static_array(std::meta::enumerators_of(^^T));
  template for (constexpr auto enum_val : enumerators) {
    if (str == std::meta::identifier_of(enum_val)) {
      out = [:enum_val:];
      return SUCCESS;
    }
  };

  return INCORRECT_TYPE;
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
  bool is_null_value;
  SIMDJSON_TRY( val.is_null().get(is_null_value) );
  if (is_null_value) {
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
