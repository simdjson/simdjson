#ifndef SIMDJSON_GENERIC_BUILDER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_STRING_BUILDER_H
#include "simdjson/generic/builder/json_string_builder.h"
#include "simdjson/concepts.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE
#if SIMDJSON_STATIC_REFLECTION

#include <charconv>
#include <cstring>
#include <meta>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
// #include <static_reflection> // for std::define_static_string - header not available yet

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace builder {

template <class T>
  requires(concepts::container_but_not_string<T> && ! concepts::optional_type<T> && !require_custom_serialization<T>)
constexpr void atom(string_builder &b, const T &t) {
  auto it = t.begin();
  auto end = t.end();
  if (it == end) {
    b.append_raw("[]");
    return;
  }
  b.append('[');
  atom(b, *it);
  ++it;
  for (; it != end; ++it) {
    b.append(',');
    atom(b, *it);
  }
  b.append(']');
}

template <class T>
  requires(std::is_same_v<T, std::string> ||
           std::is_same_v<T, std::string_view> ||
           std::is_same_v<T, const char *> ||
           std::is_same_v<T, char>)
constexpr void atom(string_builder &b, const T &t) {
  b.escape_and_append_with_quotes(t);
}

template <concepts::string_view_keyed_map T>
  requires(!require_custom_serialization<T>)
constexpr void atom(string_builder &b, const T &m) {
  if (m.empty()) {
    b.append_raw("{}");
    return;
  }
  b.append('{');
  bool first = true;
  for (const auto& [key, value] : m) {
    if (!first) {
      b.append(',');
    }
    first = false;
    // Keys must be convertible to string_view per the concept
    b.escape_and_append_with_quotes(key);
    b.append(':');
    atom(b, value);
  }
  b.append('}');
}


template<typename number_type,
         typename = typename std::enable_if<std::is_arithmetic<number_type>::value && !std::is_same_v<number_type, char>>::type>
constexpr void atom(string_builder &b, const number_type t) {
  b.append(t);
}

template <class T>
  requires(std::is_class_v<T> && !concepts::container_but_not_string<T> &&
           !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> &&
           !concepts::smart_pointer<T> &&
           !concepts::appendable_containers<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> &&
           !std::is_same_v<T, const char*> &&
           !std::is_same_v<T, char> && !require_custom_serialization<T>)
constexpr void atom(string_builder &b, const T &t) {
  // Hoist the write position into a local register for each run of fixed
  // bytes (separator, key, colon, brace, and arithmetic field values). The
  // position is synced back to the builder before any call that may grow the
  // buffer, and reloaded after, so unsafe writes between syncs avoid the
  // per-call capacity_check + memory traffic that string_builder member
  // access would otherwise impose.
  if (!b.reserve(1)) { return; }
  char *buf = b.unsafe_data();
  size_t pos = b.unsafe_position();
  buf[pos++] = '{';
  int i = 0;
  template for (constexpr auto dm : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
    // The two key forms (with and without leading comma) are precomputed at
    // consteval, then their lengths are kept as separate constants so that
    // the runtime memcpy uses an explicit known size rather than strlen.
    constexpr auto first_key = std::define_static_string(
        constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(dm)) + ":");
    constexpr auto rest_key = std::define_static_string(
        std::string(",") + constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(dm)) + ":");
    constexpr size_t quoted_size =
        constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(dm)).size();
    constexpr size_t first_key_len = quoted_size + 1;            // "key":
    constexpr size_t rest_key_len = quoted_size + 2;             // ,"key":
    using FieldT = [: std::meta::type_of(dm) :];
    constexpr bool is_unsigned_int_field =
        std::is_unsigned_v<FieldT> && !std::is_same_v<FieldT, bool> &&
        !std::is_same_v<FieldT, char>;
    constexpr size_t max_value_size =
        is_unsigned_int_field ? string_builder::MAX_UINT64_DIGITS : 0;
    // Reserve enough for the key plus (for arithmetic fields) the value. If
    // reserve grew the buffer the local `buf` pointer is stale and must be
    // refetched.
    b.unsafe_position() = pos;
    if (!b.reserve(rest_key_len + max_value_size)) { return; }
    buf = b.unsafe_data();
    pos = b.unsafe_position();
    if (i == 0) {
      std::memcpy(buf + pos, first_key, first_key_len);
      pos += first_key_len;
    } else {
      std::memcpy(buf + pos, rest_key, rest_key_len);
      pos += rest_key_len;
    }
    if constexpr (is_unsigned_int_field) {
      // Capacity for the value was reserved above; just sync, write, reload.
      b.unsafe_position() = pos;
      b.unsafe_append_uint(t.[:dm:]);
      pos = b.unsafe_position();
    } else {
      // The inner atom() may grow the buffer, invalidating both `buf` and
      // any remaining unwritten reservation. Sync, recurse, reload.
      b.unsafe_position() = pos;
      atom(b, t.[:dm:]);
      buf = b.unsafe_data();
      pos = b.unsafe_position();
    }
    i++;
  };
  // Closing brace.
  b.unsafe_position() = pos;
  if (!b.reserve(1)) { return; }
  buf = b.unsafe_data();
  pos = b.unsafe_position();
  buf[pos++] = '}';
  b.unsafe_position() = pos;
}

// Support for optional types (std::optional, etc.)
template <concepts::optional_type T>
  requires(!require_custom_serialization<T>)
constexpr void atom(string_builder &b, const T &opt) {
  if (opt) {
    atom(b, opt.value());
  } else {
    b.append_raw("null");
  }
}

// Support for smart pointers (std::unique_ptr, std::shared_ptr, etc.)
template <concepts::smart_pointer T>
  requires(!require_custom_serialization<T>)
constexpr void atom(string_builder &b, const T &ptr) {
  if (ptr) {
    atom(b, *ptr);
  } else {
    b.append_raw("null");
  }
}

// Support for enums - serialize as string representation using expand approach from P2996R12
template <typename T>
  requires(std::is_enum_v<T> && !require_custom_serialization<T>)
void atom(string_builder &b, const T &e) {
#if SIMDJSON_STATIC_REFLECTION
  static constexpr auto enumerators = std::define_static_array(std::meta::enumerators_of(^^T));
  template for (constexpr auto enum_val : enumerators) {
    constexpr auto enum_str = std::define_static_string(constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(enum_val)));
    if (e == [:enum_val:]) {
      b.append_raw(enum_str);
      return;
    }
  };
  // Fallback to integer if enum value not found
  atom(b, static_cast<std::underlying_type_t<T>>(e));
#else
  // Fallback: serialize as integer if reflection not available
  atom(b, static_cast<std::underlying_type_t<T>>(e));
#endif
}

// Support for appendable containers that don't have operator[] (sets, etc.)
template <concepts::appendable_containers T>
  requires(!concepts::container_but_not_string<T> && !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> && !concepts::smart_pointer<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> && !std::is_same_v<T, const char*> && !require_custom_serialization<T>)
constexpr void atom(string_builder &b, const T &container) {
  if (container.empty()) {
    b.append_raw("[]");
    return;
  }
  b.append('[');
  bool first = true;
  for (const auto& item : container) {
    if (!first) {
      b.append(',');
    }
    first = false;
    atom(b, item);
  }
  b.append(']');
}

// append functions that delegate to atom functions for primitive types
template <class T>
  requires(std::is_arithmetic_v<T> && !std::is_same_v<T, char>)
void append(string_builder &b, const T &t) {
  atom(b, t);
}

template <class T>
  requires(std::is_same_v<T, std::string> ||
           std::is_same_v<T, std::string_view> ||
           std::is_same_v<T, const char *> ||
           std::is_same_v<T, char>)
void append(string_builder &b, const T &t) {
  atom(b, t);
}

template <concepts::optional_type T>
  requires(!require_custom_serialization<T>)
void append(string_builder &b, const T &t) {
  atom(b, t);
}

template <concepts::smart_pointer T>
  requires(!require_custom_serialization<T>)
void append(string_builder &b, const T &t) {
  atom(b, t);
}

template <concepts::appendable_containers T>
  requires(!concepts::container_but_not_string<T> && !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> && !concepts::smart_pointer<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> && !std::is_same_v<T, const char*> && !require_custom_serialization<T>)
void append(string_builder &b, const T &t) {
  atom(b, t);
}

template <concepts::string_view_keyed_map T>
  requires(!require_custom_serialization<T>)
void append(string_builder &b, const T &t) {
  atom(b, t);
}

// works for struct
template <class Z>
  requires(std::is_class_v<Z> && !concepts::container_but_not_string<Z> &&
           !concepts::string_view_keyed_map<Z> &&
           !concepts::optional_type<Z> &&
           !concepts::smart_pointer<Z> &&
           !concepts::appendable_containers<Z> &&
           !std::is_same_v<Z, std::string> &&
           !std::is_same_v<Z, std::string_view> &&
           !std::is_same_v<Z, const char*> &&
           !std::is_same_v<Z, char> && !require_custom_serialization<Z>)
void append(string_builder &b, const Z &z) {
  // Same coalescing as the atom() overload above.
  int i = 0;
  b.append('{');
  template for (constexpr auto dm : std::define_static_array(std::meta::nonstatic_data_members_of(^^Z, std::meta::access_context::unchecked()))) {
    constexpr auto first_key = std::define_static_string(
        constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(dm)) + ":");
    constexpr auto rest_key = std::define_static_string(
        std::string(",") + constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(dm)) + ":");
    b.append_raw(i == 0 ? first_key : rest_key);
    atom(b, z.[:dm:]);
    i++;
  };
  b.append('}');
}

// works for container that have begin() and end() iterators
template <class Z>
  requires(concepts::container_but_not_string<Z> && !require_custom_serialization<Z>)
void append(string_builder &b, const Z &z) {
  auto it = z.begin();
  auto end = z.end();
  if (it == end) {
    b.append_raw("[]");
    return;
  }
  b.append('[');
  atom(b, *it);
  ++it;
  for (; it != end; ++it) {
    b.append(',');
    atom(b, *it);
  }
  b.append(']');
}

template <class Z>
  requires (require_custom_serialization<Z>)
void append(string_builder &b, const Z &z) {
  b.append(z);
}


template <class Z>
simdjson_warn_unused simdjson_result<std::string> to_json_string(const Z &z, size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  string_builder b(initial_capacity);
  append(b, z);
  std::string_view s;
  if(auto e = b.view().get(s); e) { return e; }
  return std::string(s);
}

template <class Z>
simdjson_warn_unused error_code to_json(const Z &z, std::string &s, size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  string_builder b(initial_capacity);
  append(b, z);
  std::string_view view;
  if(auto e = b.view().get(view); e) { return e; }
  s.assign(view);
  return SUCCESS;
}

template <class Z>
string_builder& operator<<(string_builder& b, const Z& z) {
  append(b, z);
  return b;
}

// extract_from: Serialize only specific fields from a struct to JSON
template<constevalutil::fixed_string... FieldNames, typename T>
  requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
void extract_from(string_builder &b, const T &obj) {
  b.append('{');
  bool first = true;
  // Iterate through all members of T using reflection
  static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));
  template for (constexpr auto mem : members) {
    if constexpr (std::meta::is_public(mem)) {
      static constexpr std::string_view key = std::define_static_string(std::meta::identifier_of(mem));

      // Only serialize this field if it's in our list of requested fields
      if constexpr (((FieldNames.view() == key) || ...)) {
        // Same coalescing as the atom() / append() struct overloads.
        static constexpr auto first_key = std::define_static_string(
            constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(mem)) + ":");
        static constexpr auto rest_key = std::define_static_string(
            std::string(",") + constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(mem)) + ":");
        b.append_raw(first ? first_key : rest_key);
        first = false;

        // Serialize the value
        atom(b, obj.[:mem:]);
      }
    }
  };

  b.append('}');
}

template<constevalutil::fixed_string... FieldNames, typename T>
  requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
simdjson_warn_unused simdjson_result<std::string> extract_from(const T &obj, size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  string_builder b(initial_capacity);
  extract_from<FieldNames...>(b, obj);
  std::string_view s;
  if(auto e = b.view().get(s); e) { return e; }
  return std::string(s);
}

} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION
// Alias the function template to 'to' in the global namespace
template <class Z>
simdjson_warn_unused simdjson_result<std::string> to_json(const Z &z, size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  SIMDJSON_IMPLEMENTATION::builder::string_builder b(initial_capacity);
  SIMDJSON_IMPLEMENTATION::builder::append(b, z);
  std::string_view s;
  if(auto e = b.view().get(s); e) { return e; }
  return std::string(s);
}
template <class Z>
simdjson_warn_unused error_code to_json(const Z &z, std::string &s, size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  SIMDJSON_IMPLEMENTATION::builder::string_builder b(initial_capacity);
  SIMDJSON_IMPLEMENTATION::builder::append(b, z);
  std::string_view view;
  if(auto e = b.view().get(view); e) { return e; }
  s.assign(view);
  return SUCCESS;
}
// Global namespace function for extract_from
template<constevalutil::fixed_string... FieldNames, typename T>
  requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
simdjson_warn_unused simdjson_result<std::string> extract_from(const T &obj, size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  SIMDJSON_IMPLEMENTATION::builder::string_builder b(initial_capacity);
  SIMDJSON_IMPLEMENTATION::builder::extract_from<FieldNames...>(b, obj);
  std::string_view s;
  if(auto e = b.view().get(s); e) { return e; }
  return std::string(s);
}

} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION

#endif