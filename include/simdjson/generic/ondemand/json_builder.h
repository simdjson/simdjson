/**
 * This file is part of the builder API. It is temporarily in the ondemand directory
 * but we will move it to a builder directory later.
 */
#ifndef SIMDJSON_GENERIC_BUILDER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_STRING_BUILDER_H
#include "simdjson/generic/builder/json_string_builder.h"
#include "simdjson/concepts.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE
#if SIMDJSON_STATIC_REFLECTION

#include <charconv>
#include <cstring>
#include <experimental/meta>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
// #include <static_reflection> // for std::define_static_string - header not available yet

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace builder {

// Concept that checks if a type is a container but not a string (because
// strings handling must be handled differently)
template <typename T>
concept container_but_not_string =
    requires(T a) {
      { a.size() } -> std::convertible_to<std::size_t>;
      {
        a[std::declval<std::size_t>()]
      }; // check if elements are accessible for the subscript operator
    } && !std::is_same_v<T, std::string> &&
    !std::is_same_v<T, std::string_view> && !std::is_same_v<T, const char *>;

template <class T>
  requires(container_but_not_string<T>)
constexpr void atom(string_builder &b, const T &t) {
  if (t.size() == 0) {
    b.append_raw("[]");
    return;
  }
  b.append('[');
  atom(b, t[0]);
  for (size_t i = 1; i < t.size(); ++i) {
    b.append(',');
    atom(b, t[i]);
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
#if SIMDJSON_CONSTEVAL
consteval std::string consteval_to_quoted_escaped(std::string_view input);
#endif

template <class T>
  requires(std::is_class_v<T> && !container_but_not_string<T> &&
           !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> &&
           !concepts::smart_pointer<T> &&
           !concepts::appendable_containers<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> &&
           !std::is_same_v<T, const char*> &&
           !std::is_same_v<T, char>)
constexpr void atom(string_builder &b, const T &t) {
  int i = 0;
  b.append('{');
  [:expand(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())):] >> [&]<auto dm>() {
    if (i != 0)
      b.append(',');
    constexpr auto key = std::define_static_string(consteval_to_quoted_escaped(std::meta::identifier_of(dm)));
    b.append_raw(key);
    b.append(':');
    atom(b, t.[:dm:]);
    i++;
  };
  b.append('}');
}

// Support for optional types (std::optional, etc.)
template <concepts::optional_type T>
constexpr void atom(string_builder &b, const T &opt) {
  if (opt) {
    atom(b, opt.value());
  } else {
    b.append_raw("null");
  }
}

// Support for smart pointers (std::unique_ptr, std::shared_ptr, etc.)
template <concepts::smart_pointer T>
constexpr void atom(string_builder &b, const T &ptr) {
  if (ptr) {
    atom(b, *ptr);
  } else {
    b.append_raw("null");
  }
}

// Support for enums - serialize as string representation using expand approach from P2996R12
template <typename T>
  requires(std::is_enum_v<T>)
void atom(string_builder &b, const T &e) {
#if SIMDJSON_STATIC_REFLECTION
  std::string_view result = "<unnamed>";
  [:expand(std::meta::enumerators_of(^^T)):] >> [&]<auto enum_val>{
    if (e == [:enum_val:]) {
      result = std::meta::identifier_of(enum_val);
    }
  };

  if (result != "<unnamed>") {
    b.append_raw("\"");
    b.append_raw(result);
    b.append_raw("\"");
  } else {
    // Fallback to integer if enum value not found
    atom(b, static_cast<std::underlying_type_t<T>>(e));
  }
#else
  // Fallback: serialize as integer if reflection not available
  atom(b, static_cast<std::underlying_type_t<T>>(e));
#endif
}

// Support for appendable containers that don't have operator[] (sets, etc.)
template <concepts::appendable_containers T>
  requires(!container_but_not_string<T> && !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> && !concepts::smart_pointer<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> && !std::is_same_v<T, const char*>)
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
void append(string_builder &b, const T &t) {
  atom(b, t);
}

template <concepts::smart_pointer T>
void append(string_builder &b, const T &t) {
  atom(b, t);
}

template <concepts::appendable_containers T>
  requires(!container_but_not_string<T> && !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> && !concepts::smart_pointer<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> && !std::is_same_v<T, const char*>)
void append(string_builder &b, const T &t) {
  atom(b, t);
}

template <concepts::string_view_keyed_map T>
void append(string_builder &b, const T &t) {
  atom(b, t);
}

// works for struct
template <class Z>
  requires(std::is_class_v<Z> && !container_but_not_string<Z> &&
           !concepts::string_view_keyed_map<Z> &&
           !concepts::optional_type<Z> &&
           !concepts::smart_pointer<Z> &&
           !concepts::appendable_containers<Z> &&
           !std::is_same_v<Z, std::string> &&
           !std::is_same_v<Z, std::string_view> &&
           !std::is_same_v<Z, const char*> &&
           !std::is_same_v<Z, char>)
void append(string_builder &b, const Z &z) {
  int i = 0;
  b.append('{');
  [:expand(std::meta::nonstatic_data_members_of(^^Z, std::meta::access_context::unchecked())):] >> [&]<auto dm>() {
    if (i != 0)
      b.append(',');
    constexpr auto key = std::define_static_string(consteval_to_quoted_escaped(std::meta::identifier_of(dm)));
    b.append_raw(key);
    b.append(':');
    atom(b, z.[:dm:]);
    i++;
  };
  b.append('}');
}

// works for container
template <class Z>
  requires(container_but_not_string<Z>)
void append(string_builder &b, const Z &z) {
  if (z.size() == 0) {
    b.append_raw("[]");
    return;
  }
  b.append('[');
  atom(b, z[0]);
  for (size_t i = 1; i < z.size(); ++i) {
    b.append(',');
    atom(b, z[i]);
  }
  b.append(']');
}

template <class Z>
simdjson_result<std::string> to_json_string(const Z &z, size_t initial_capacity = 1024) {
  string_builder b(initial_capacity);
  append(b, z);
  std::string_view s;
  if(auto e = b.view().get(s); e) { return e; }
  return std::string(s);
}

template <class Z>
simdjson_error to_json(const Z &z, std::string &s) {
  string_builder b;
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
} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION
// Alias the function template to 'to' in the global namespace
template <class Z>
simdjson_result<std::string> to_json(const Z &z, size_t initial_capacity = 1024) {
  return SIMDJSON_IMPLEMENTATION::builder::to_json_string(z, initial_capacity);
}
} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION

#endif