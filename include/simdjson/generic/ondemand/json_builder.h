/**
 * This file is part of the builder API. It is temporarily in the ondemand directory
 * but we will move it to a builder directory later.
 */
#ifndef SIMDJSON_GENERIC_BUILDER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_STRING_BUILDER_H
#include "simdjson/generic/builder/json_string_builder.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE
#if SIMDJSON_STATIC_REFLECTION

#include "simdjson/concepts.h"
#include <charconv>
#include <cstring>
#include <experimental/meta>
#include <string_view>
#include <type_traits>
#include <utility>

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
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view>)
constexpr void atom(string_builder &b, const T &t) {
  int i = 0;
  b.append('{');
  [:expand(std::meta::nonstatic_data_members_of(^T)):] >> [&]<auto dm> {
    if (i != 0)
      b.append(',');
    constexpr auto key = consteval_to_quoted_escaped(std::meta::identifier_of(dm));
    b.append_raw(key);
    b.append(':');
    atom(b, t.[:dm:]);
    i++;
  };
  b.append('}');
}

// works for struct
template <class Z> void append(string_builder &b, const Z &z) {
  int i = 0;
  b.append('{');
  [:expand(std::meta::nonstatic_data_members_of(^Z)):] >> [&]<auto dm> {
    if (i != 0)
      b.append(',');
    constexpr auto key = consteval_to_quoted_escaped(std::meta::identifier_of(dm));
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
simdjson_result<std::string> to_json_string(const Z &z) {
  string_builder b;
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
} // namespace json_builder
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
#endif // SIMDJSON_STATIC_REFLECTION

#endif