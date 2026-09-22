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
  if constexpr (std::is_same_v<T, float>) {
    // Going through binary64 and then rounding to binary32 would round twice
    // and could produce a value that is not the float nearest to the JSON
    // number, so we parse to binary32 directly.
    return val.get_float().get(out);
  } else {
    double x;
    SIMDJSON_TRY(val.get_double().get(x));
    out = static_cast<T>(x);
    return SUCCESS;
  }
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
    // Deserialize into a temporary first: an error or an exception (a user
    // tag_invoke may throw) must not leave a default-constructed element behind.
    value_type temp;
    if (auto const err = v.get<value_type>(temp); err) {
      return err;
    }
    concepts::emplace_one(out, std::move(temp));
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
error_code tag_invoke(deserialize_tag, SIMDJSON_IMPLEMENTATION::ondemand::object &obj, T &out) noexcept(false) {
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
error_code tag_invoke(deserialize_tag, SIMDJSON_IMPLEMENTATION::ondemand::value &val, T &out) noexcept(false) {
  SIMDJSON_IMPLEMENTATION::ondemand::object obj;
  SIMDJSON_TRY(val.get_object().get(obj));
  return simdjson::deserialize(obj, out);
}

template <concepts::string_view_keyed_map T>
error_code tag_invoke(deserialize_tag, SIMDJSON_IMPLEMENTATION::ondemand::document &doc, T &out) noexcept(false) {
  SIMDJSON_IMPLEMENTATION::ondemand::object obj;
  SIMDJSON_TRY(doc.get_object().get(obj));
  return simdjson::deserialize(obj, out);
}

template <concepts::string_view_keyed_map T>
error_code tag_invoke(deserialize_tag, SIMDJSON_IMPLEMENTATION::ondemand::document_reference &doc, T &out) noexcept(false) {
  SIMDJSON_IMPLEMENTATION::ondemand::object obj;
  SIMDJSON_TRY(doc.get_object().get(obj));
  return simdjson::deserialize(obj, out);
}


/**
 * This CPO (Customization Point Object) will help deserialize into
 * smart pointers.
 *
 * @tparam T The type inside the smart pointer
 * @tparam ValT document/value type
 * @param val document/value
 * @param out a reference to the smart pointer
 * @return status of the conversion
 */
template <concepts::smart_pointer T, typename ValT>
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept(false) {
  using element_type = typename std::remove_cvref_t<T>::element_type;

  // For better error messages, don't use these as constraints on
  // the tag_invoke CPO.
  static_assert(
      deserializable<element_type, ValT>,
      "The specified type inside the unique_ptr must itself be deserializable");
  static_assert(
      std::is_default_constructible_v<element_type>,
      "The specified type inside the unique_ptr must default constructible.");

  // Own the allocation before get(): a user tag_invoke may throw.
  std::unique_ptr<element_type> ptr(new (std::nothrow) element_type());
  if (!ptr) {
    return MEMALLOC;
  }
  SIMDJSON_TRY(val.template get<element_type>(*ptr));
  out = std::move(ptr);
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
!concepts::appendable_containers<T>
// simdjson's own types (array, object, value, raw_json_string, number,
// document, document_reference) have dedicated get<T>() specializations and
// must never go through reflection.
&& !is_builtin_deserializable_v<T>
&& !std::is_same_v<T, SIMDJSON_IMPLEMENTATION::ondemand::number>
&& !std::is_same_v<T, SIMDJSON_IMPLEMENTATION::ondemand::document>
&& !std::is_same_v<T, SIMDJSON_IMPLEMENTATION::ondemand::document_reference>);


// key_selector_reflection_detail is defined unconditionally (it only requires
// static reflection). It provides the compile-time machinery for building a
// key_selector from a struct's members, the per-member helpers shared by every
// deserialization path (they implement the annotations of annotations.h), the
// ordered per-member fallback used by the opt-out build and as an automatic
// fallback (see deserialize_struct_ordered and keys_fit_selector below), and the
// strict path used for structs annotated with deny_unknown_fields.
namespace key_selector_reflection_detail {

// A member participates if it is public, non-const, and not annotated with skip
// or skip_deserializing.
consteval bool is_eligible_member(std::meta::info mem) {
  return !std::meta::is_const(mem) && std::meta::is_public(mem)
      && !simdjson::detail::has_annotation(mem, ^^simdjson::detail::skip_tag)
      && !simdjson::detail::has_annotation(mem, ^^simdjson::detail::skip_deserializing_tag);
}

// A field is a data member reached from T through a chain of members: usually a
// direct member of T (a path of length one), or a member of a member annotated
// with flatten (the members of a flattened structure are fields of the enclosing
// one). A field is identified by the type member_path<m1, m2, ..., leaf>.
template <std::meta::info First, std::meta::info... Rest>
struct member_path {
  // The data member holding the value; its annotations drive (de)serialization.
  static constexpr std::meta::info leaf = [] {
    std::meta::info members[] = {First, Rest...};
    return members[sizeof...(Rest)];
  }();
  template <typename T>
  static simdjson_inline constexpr auto &get(T &obj) noexcept {
    if constexpr (sizeof...(Rest) == 0) {
      return obj.[:First:];
    } else {
      return member_path<Rest...>::get(obj.[:First:]);
    }
  }
};

consteval void append_eligible_fields(std::meta::info type, std::vector<std::meta::info> &prefix,
                                      std::vector<std::meta::info> &fields) {
  for (std::meta::info mem : std::meta::nonstatic_data_members_of(type, std::meta::access_context::unchecked())) {
    if (!is_eligible_member(mem)) { continue; }
    prefix.push_back(std::meta::reflect_constant(mem));
    if (simdjson::detail::has_annotation(mem, ^^simdjson::detail::flatten_tag)) {
      append_eligible_fields(simdjson::detail::flattened_type(mem), prefix, fields);
    } else {
      fields.push_back(std::meta::substitute(^^member_path, prefix));
    }
    prefix.pop_back();
  }
}

// The fields of `type` that participate in deserialization, in declaration order
// (the fields of a flattened member take its place). A field's position in this
// list is its "field index" below.
consteval std::vector<std::meta::info> eligible_fields(std::meta::info type) {
  std::vector<std::meta::info> prefix;
  std::vector<std::meta::info> fields;
  append_eligible_fields(type, prefix, fields);
  return fields;
}

// The data member at the end of a field path.
consteval std::meta::info field_leaf(std::meta::info path) {
  return std::meta::extract<std::meta::info>(std::meta::template_arguments_of(path).back());
}

// Number of fields that participate in deserialization. A class can have zero
// eligible fields (e.g. std::chrono::time_point, whose only data member is
// private): an empty key_selector cannot be built, so the tag_invoke below
// special-cases this count.
template <typename T>
consteval std::size_t eligible_field_count() {
  return eligible_fields(^^T).size();
}

// The keys accepted for a member (or an enumerator): its JSON key followed by its
// aliases. They are static strings, so they can be used as template arguments.
consteval std::vector<const char *> accepted_keys_of(std::meta::info entity) {
  std::vector<const char *> keys;
  for (std::string_view key : simdjson::detail::json_key_names(entity)) {
    keys.push_back(std::define_static_string(key));
  }
  return keys;
}

// Every key accepted for T: the eligible fields in order, each followed by its
// aliases. This is the key list of T's key_selector.
consteval std::vector<const char *> accepted_keys(std::meta::info type) {
  std::vector<const char *> keys;
  for (std::meta::info path : eligible_fields(type)) {
    for (const char *key : accepted_keys_of(field_leaf(path))) { keys.push_back(key); }
  }
  return keys;
}

// The field index of each entry of accepted_keys(type).
consteval std::vector<std::size_t> accepted_key_fields(std::meta::info type) {
  std::vector<std::size_t> key_fields;
  std::vector<std::meta::info> fields = eligible_fields(type);
  for (std::size_t i = 0; i < fields.size(); ++i) {
    for (std::size_t j = 0; j < accepted_keys_of(field_leaf(fields[i])).size(); ++j) { key_fields.push_back(i); }
  }
  return key_fields;
}

// True when some eligible field of T has aliases: several selector keys may then
// map to the same field.
template <typename T>
consteval bool has_aliases() {
  return accepted_keys(^^T).size() != eligible_fields(^^T).size();
}

// True when `mem` is annotated with default_value or default_from (directly, or
// through a default_value annotation on the enclosing structure).
template <auto mem>
consteval bool has_default() {
  return simdjson::detail::has_annotation(mem, ^^simdjson::detail::default_value_tag)
      || simdjson::detail::has_annotation(std::meta::parent_of(mem), ^^simdjson::detail::default_value_tag)
      || simdjson::detail::annotation_of_template(mem, ^^simdjson::detail::default_from_t) != std::meta::info{};
}

// True when a missing key for `mem` is not an error: optional members and
// members with a default.
template <auto mem>
consteval bool may_be_absent() {
  return concepts::optional_type<typename [: std::meta::type_of(mem) :]> || has_default<mem>();
}

// True when every eligible field of T is required (none may be absent). In that
// case presence can be checked with a single match count instead of a per-field
// "seen" array.
template <typename T>
consteval bool all_eligible_fields_required() {
  bool all_required = true;
  template for (constexpr auto path : std::define_static_array(eligible_fields(^^T))) {
    if constexpr (may_be_absent<[: path :]::leaf>()) {
      all_required = false;
    }
  }
  return all_required;
}

// `key` as a constevalutil::fixed_string usable as an NTTP.
template <const char *key>
consteval auto key_fixed_string() {
  constexpr std::string_view key_view{ key };
  char buffer[key_view.size() + 1] = {};
  for (std::size_t i = 0; i < key_view.size(); ++i) { buffer[i] = key_view[i]; }
  return constevalutil::fixed_string<key_view.size() + 1>(buffer);
}

// key_selector template arguments (one fixed_string per accepted key), in the
// order of accepted_keys.
template <typename T>
consteval std::vector<std::meta::info> selector_key_args() {
  std::vector<std::meta::info> args;
  template for (constexpr const char *key : std::define_static_array(accepted_keys(^^T))) {
    args.push_back(std::meta::reflect_constant(key_fixed_string<key>()));
  }
  return args;
}

// key_selector whose keys are exactly T's accepted keys (index i <-> i-th key).
template <typename T>
using selector_for = typename [: std::meta::substitute(
    ^^SIMDJSON_IMPLEMENTATION::ondemand::key_selector, selector_key_args<T>()) :];

// True when T's accepted keys satisfy every key_selector requirement, so a
// key_selector can be built for T without a compile-time error. This mirrors the
// key_selector limits (see key_selector.h): at most 255 keys, each key non-empty
// and at most 63 characters, no backslash / double-quote / null byte, and all
// keys distinct. When this returns false the deserializer falls back to the
// ordered per-field path instead of failing to compile.
template <typename T>
consteval bool keys_fit_selector() {
  std::vector<std::string_view> keys;
  for (const char *key : accepted_keys(^^T)) { keys.push_back(key); }
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

// Deserialize a JSON value into `target`, the storage of member `mem`, through
// the member's with<Adapter> annotation when the adapter provides a deserialize
// function.
template <auto mem, typename ValueT, typename M>
simdjson_warn_unused simdjson_inline error_code deserialize_member_value(ValueT &field_value, M &target) noexcept(false) {
  constexpr std::meta::info with_type = simdjson::detail::annotation_of_template(mem, ^^simdjson::detail::with_t);
  if constexpr (with_type != std::meta::info{}) {
    using adapter = typename [: with_type :]::adapter;
    if constexpr (requires { { adapter::deserialize(field_value, target) } -> std::convertible_to<error_code>; }) {
      return adapter::deserialize(field_value, target);
    } else {
      return field_value.get(target);
    }
  } else {
    return field_value.get(target);
  }
}

// Deserialize the storage `target` of member `mem` from a JSON value.
template <auto mem, typename ValueT, typename M>
simdjson_warn_unused simdjson_inline error_code deserialize_member(ValueT &field_value, M &target) noexcept(false) {
  if constexpr (has_default<mem>() && std::is_default_constructible_v<M> && std::is_move_assignable_v<M>) {
    // A present key replaces the default value: deserialize into a fresh
    // temporary so that, e.g., a container does not append to its default
    // content, and a failure leaves the default untouched.
    M value{};
    SIMDJSON_TRY(deserialize_member_value<mem>(field_value, value));
    target = std::move(value);
    return SUCCESS;
  } else {
    return deserialize_member_value<mem>(field_value, target);
  }
}

// Deserialize the field with the given field index.
template <typename T>
simdjson_warn_unused simdjson_inline error_code deserialize_field_at(
    std::size_t field_index, SIMDJSON_IMPLEMENTATION::ondemand::value field_value, T &out) noexcept(false) {
  std::size_t counter = 0;
  template for (constexpr auto path : std::define_static_array(eligible_fields(^^T))) {
    using field = [: path :];
    if (field_index == counter) { return deserialize_member<field::leaf>(field_value, field::get(out)); }
    ++counter;
  }
  return SUCCESS;
}

// Called when the key(s) of member `mem`, stored in `target`, are missing from
// the JSON object: an error for a required member, a call to the default_from
// factory, or nothing at all.
template <auto mem, typename M>
simdjson_warn_unused simdjson_inline error_code on_missing_member(M &target) noexcept(false) {
  constexpr std::meta::info default_from_type = simdjson::detail::annotation_of_template(mem, ^^simdjson::detail::default_from_t);
  if constexpr (default_from_type != std::meta::info{}) {
    target = [: default_from_type :]::factory();
    return SUCCESS;
  } else if constexpr (may_be_absent<mem>()) {
    // For optional and default_value members, a missing key is not an error:
    // leave the member at its current (default) value.
    (void)target;
    return SUCCESS;
  } else {
    (void)target;
    return NO_SUCH_FIELD;
  }
}

// Report or handle every field that was not seen in the JSON object.
template <typename T, std::size_t N>
simdjson_warn_unused simdjson_inline error_code handle_missing_fields(
    const std::array<bool, N> &seen_field, T &out) noexcept(false) {
  std::size_t counter = 0;
  template for (constexpr auto path : std::define_static_array(eligible_fields(^^T))) {
    using field = [: path :];
    if (!seen_field[counter]) { SIMDJSON_TRY(on_missing_member<field::leaf>(field::get(out))); }
    ++counter;
  }
  return SUCCESS;
}

// Ordered, per-field deserialization: one obj[key] lookup per eligible field
// (and per alias, until one is found). This is the opt-out path
// (-DSIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION=1) and the automatic fallback for
// structs whose keys do not fit the key_selector limits (see keys_fit_selector).
template <typename T>
simdjson_warn_unused error_code deserialize_struct_ordered(
    SIMDJSON_IMPLEMENTATION::ondemand::object &obj, T &out) noexcept(false) {
  template for (constexpr auto path : std::define_static_array(eligible_fields(^^T))) {
    using field = [: path :];
    SIMDJSON_IMPLEMENTATION::ondemand::value field_value;
    error_code error = NO_SUCH_FIELD;
    template for (constexpr const char *key : std::define_static_array(accepted_keys_of(field::leaf))) {
      if (error == NO_SUCH_FIELD) { error = obj[std::string_view(key)].get(field_value); }
    }
    if (error == NO_SUCH_FIELD) {
      SIMDJSON_TRY(on_missing_member<field::leaf>(field::get(out)));
    } else if (error) {
      return error;
    } else {
      SIMDJSON_TRY(deserialize_member<field::leaf>(field_value, field::get(out)));
    }
  }
  return SUCCESS;
}

// Strict deserialization for structs annotated with deny_unknown_fields: a single
// pass over every field of the object, where a key that does not map to an
// eligible field is reported as UNKNOWN_FIELD. As with object::for_each, the
// first occurrence of a field wins (later duplicates, or aliases of a field
// already seen, are ignored).
template <typename T>
simdjson_warn_unused error_code deserialize_struct_strict(
    SIMDJSON_IMPLEMENTATION::ondemand::object &obj, T &out) noexcept(false) {
  static constexpr auto keys = std::define_static_array(accepted_keys(^^T));
  static constexpr auto key_fields = std::define_static_array(accepted_key_fields(^^T));
  std::array<bool, eligible_field_count<T>()> seen_field{};
  for (auto field_result : obj) {
    SIMDJSON_IMPLEMENTATION::ondemand::field json_field;
    SIMDJSON_TRY(std::move(field_result).get(json_field));
    std::string_view key;
    SIMDJSON_TRY(json_field.unescaped_key().get(key));
    std::size_t key_index = keys.size();
    for (std::size_t i = 0; i < keys.size(); ++i) {
      if (key == std::string_view(keys[i])) { key_index = i; break; }
    }
    if (key_index == keys.size()) { return UNKNOWN_FIELD; }
    const std::size_t field_index = key_fields[key_index];
    if (seen_field[field_index]) { continue; }
    seen_field[field_index] = true;
    SIMDJSON_TRY(deserialize_field_at(field_index, json_field.value(), out));
  }
  return handle_missing_fields(seen_field, out);
}

template <typename T>
consteval bool is_transparent() {
  return simdjson::detail::has_annotation(^^T, ^^simdjson::detail::transparent_tag);
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
// Structs annotated with deny_unknown_fields always use the strict path, and
// structs annotated with transparent are deserialized as their single member.
//
// noexcept(false): a member's tag_invoke may throw and the exception must reach
// the caller of get<T>().
template <typename T, typename ValT>
  requires(user_defined_type<T> && std::is_class_v<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept(false) {
  if constexpr (key_selector_reflection_detail::is_transparent<T>()) {
    constexpr auto mem = simdjson::detail::transparent_member(^^T);
    if constexpr (std::is_same_v<std::remove_cvref_t<ValT>, SIMDJSON_IMPLEMENTATION::ondemand::object>) {
      // We were handed an object: only a structure can be deserialized from it.
      if constexpr (user_defined_type<typename [: std::meta::type_of(mem) :]>) {
        return tag_invoke(deserialize_tag{}, val, out.[:mem:]);
      } else {
        return INCORRECT_TYPE;
      }
    } else {
      return key_selector_reflection_detail::deserialize_member<mem>(val, out.[:mem:]);
    }
  } else {
  SIMDJSON_IMPLEMENTATION::ondemand::object obj;
  if constexpr (std::is_same_v<std::remove_cvref_t<ValT>, SIMDJSON_IMPLEMENTATION::ondemand::object>) {
    obj = val;
  } else {
    SIMDJSON_TRY(val.get_object().get(obj));
  }
  if constexpr (simdjson::detail::has_annotation(^^T, ^^simdjson::detail::deny_unknown_fields_tag)) {
    return key_selector_reflection_detail::deserialize_struct_strict(obj, out);
  } else {
#if defined(SIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION) && SIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION
  // Opt-out build: always use the ordered per-member path.
  return key_selector_reflection_detail::deserialize_struct_ordered(obj, out);
#else
  if constexpr (key_selector_reflection_detail::eligible_field_count<T>() == 0) {
    // No fields to deserialize: an empty key_selector cannot be built, so just
    // validate that the input is an object (done above) and succeed. Mirrors the
    // ordered per-member path, which iterates over zero members.
    (void)out;
    (void)obj;
    return SUCCESS;
  } else if constexpr (!key_selector_reflection_detail::keys_fit_selector<T>()) {
    // Automatic fallback: T's accepted keys do not fit the key_selector limits
    // (e.g. a member name longer than 63 characters), so building a selector
    // would be a compile error. Use the ordered per-member path instead, so the
    // default never breaks a struct that the opt-out path would accept.
    return key_selector_reflection_detail::deserialize_struct_ordered(obj, out);
  } else {
  using selector = key_selector_reflection_detail::selector_for<T>;
  if constexpr (key_selector_reflection_detail::all_eligible_fields_required<T>()
                && !key_selector_reflection_detail::has_aliases<T>()) {
    // Fast path: every member is required and has a single key. A single
    // for_each pass parses each matched field; the returned match count then
    // tells us whether every member was present (matched_count ==
    // selector::size()) without a per-member "seen" array. A value-parse error
    // (e.g. a type mismatch) is propagated by for_each.
    auto walk = obj.template for_each<selector>(
        [&](std::size_t matched_index, SIMDJSON_IMPLEMENTATION::ondemand::value field_value) -> error_code {
      std::size_t counter = 0;
      template for (constexpr auto path : std::define_static_array(key_selector_reflection_detail::eligible_fields(^^T))) {
        using field = [: path :];
        if (matched_index == counter) { return key_selector_reflection_detail::deserialize_member<field::leaf>(field_value, field::get(out)); }
        ++counter;
      }
      return SUCCESS;
    });
    if (walk.error) { return walk.error; }
    // A missing required member shows up as a short match count and is reported as
    // NO_SUCH_FIELD, mirroring the ordered obj[key] path.
    if (walk.matched_count != selector::size()) { return NO_SUCH_FIELD; }
    return SUCCESS;
  } else {
    static constexpr auto key_fields = std::define_static_array(key_selector_reflection_detail::accepted_key_fields(^^T));
    std::array<bool, key_selector_reflection_detail::eligible_field_count<T>()> seen_field{};
    // Single pass over the object: each field whose key matches a member (or one
    // of its aliases) yields its selector index, which we map back to the
    // corresponding member. The first key seen for a member wins. The callback
    // returns an error_code so that a value-parse error (e.g. a type mismatch on
    // a matched field) is propagated by for_each instead of being silently dropped.
    error_code walk_error = obj.template for_each<selector>(
        [&](std::size_t matched_index, SIMDJSON_IMPLEMENTATION::ondemand::value field_value) -> error_code {
      const std::size_t field_index = key_fields[matched_index];
      if (seen_field[field_index]) { return SUCCESS; }
      seen_field[field_index] = true;
      return key_selector_reflection_detail::deserialize_field_at(field_index, field_value, out);
    });
    if (walk_error) { return walk_error; }
    // Required members must be present: a missing one is reported as
    // NO_SUCH_FIELD, mirroring the ordered obj[key] path. Optional and defaulted
    // members may be absent.
    return key_selector_reflection_detail::handle_missing_fields(seen_field, out);
  }
  }
#endif // SIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION
  }
  }
}

// Support for enum deserialization - deserialize from string representation using
// expand approach from P2996R12. The accepted strings are the enumerator's JSON key
// (see rename and rename_all) and its aliases.
template <typename T, typename ValT>
  requires(std::is_enum_v<T>)
error_code tag_invoke(deserialize_tag, ValT &val, T &out) noexcept {
#if SIMDJSON_STATIC_REFLECTION
  std::string_view str;
  SIMDJSON_TRY(val.get_string().get(str));
  static constexpr auto enumerators = std::define_static_array(std::meta::enumerators_of(^^T));
  template for (constexpr auto enum_val : enumerators) {
    template for (constexpr const char *key : std::define_static_array(key_selector_reflection_detail::accepted_keys_of(enum_val))) {
      if (str == std::string_view(key)) {
        out = [:enum_val:];
        return SUCCESS;
      }
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
error_code tag_invoke(deserialize_tag, simdjson_value &val, std::unique_ptr<T> &out) noexcept(false) {
  std::unique_ptr<T> ptr(new (std::nothrow) T());
  if (!ptr) {
    return MEMALLOC;
  }
  SIMDJSON_TRY(val.get(*ptr));
  out = std::move(ptr);
  return SUCCESS;
}

template <typename simdjson_value, typename T>
  requires(user_defined_type<std::remove_cvref_t<T>>)
error_code tag_invoke(deserialize_tag, simdjson_value &val, std::shared_ptr<T> &out) noexcept(false) {
  std::shared_ptr<T> ptr(new (std::nothrow) T());
  if (!ptr) {
    return MEMALLOC;
  }
  SIMDJSON_TRY(val.get(*ptr));
  out = std::move(ptr);
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
