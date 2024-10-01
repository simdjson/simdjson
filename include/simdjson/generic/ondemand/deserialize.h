#if SIMDJSON_SUPPORTS_DESERIALIZATION

#ifndef SIMDJSON_ONDEMAND_DESERIALIZE_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_ONDEMAND_DESERIALIZE_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/array.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <concepts>
namespace simdjson {

namespace tag_invoke_fn_ns {
void tag_invoke();

struct tag_invoke_fn {
  template <typename Tag, typename... Args>
    requires requires(Tag tag, Args &&...args) {
      tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...);
    }
  constexpr auto operator()(Tag tag, Args &&...args) const
      noexcept(noexcept(tag_invoke(std::forward<Tag>(tag),
                                   std::forward<Args>(args)...)))
          -> decltype(tag_invoke(std::forward<Tag>(tag),
                                 std::forward<Args>(args)...)) {
    return tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...);
  }
};
} // namespace tag_invoke_fn_ns

inline namespace tag_invoke_ns {
inline constexpr tag_invoke_fn_ns::tag_invoke_fn tag_invoke = {};
} // namespace tag_invoke_ns

template <typename Tag, typename... Args>
concept tag_invocable = requires(Tag tag, Args... args) {
  tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...);
};

template <typename Tag, typename... Args>
concept nothrow_tag_invocable =
    tag_invocable<Tag, Args...> && requires(Tag tag, Args... args) {
      {
        tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...)
      } noexcept;
    };

template <typename Tag, typename... Args>
using tag_invoke_result =
    std::invoke_result<decltype(tag_invoke), Tag, Args...>;

template <typename Tag, typename... Args>
using tag_invoke_result_t =
    std::invoke_result_t<decltype(tag_invoke), Tag, Args...>;

template <auto &Tag> using tag_t = std::decay_t<decltype(Tag)>;


struct deserialize_tag;

/// These types are deserializable in a built-in way
template <typename> struct is_builtin_deserializable : std::false_type {};
template <> struct is_builtin_deserializable<int64_t> : std::true_type {};
template <> struct is_builtin_deserializable<uint64_t> : std::true_type {};
template <> struct is_builtin_deserializable<double> : std::true_type {};
template <> struct is_builtin_deserializable<bool> : std::true_type {};
template <> struct is_builtin_deserializable<SIMDJSON_IMPLEMENTATION::ondemand::array> : std::true_type {};
template <> struct is_builtin_deserializable<SIMDJSON_IMPLEMENTATION::ondemand::object> : std::true_type {};
template <> struct is_builtin_deserializable<SIMDJSON_IMPLEMENTATION::ondemand::value> : std::true_type {};
template <> struct is_builtin_deserializable<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> : std::true_type {};
template <> struct is_builtin_deserializable<std::string_view> : std::true_type {};

template <typename T>
concept is_builtin_deserializable_v = is_builtin_deserializable<T>::value;

template <typename T, typename ValT = SIMDJSON_IMPLEMENTATION::ondemand::value>
concept custom_deserializable = tag_invocable<deserialize_tag, ValT&, T&>;

template <typename T, typename ValT = SIMDJSON_IMPLEMENTATION::ondemand::value>
concept deserializable = custom_deserializable<T, ValT> || is_builtin_deserializable_v<T>;

template <typename T, typename ValT = SIMDJSON_IMPLEMENTATION::ondemand::value>
concept nothrow_custom_deserializable = nothrow_tag_invocable<deserialize_tag, ValT&, T&>;

// built-in types are noexcept and if an error happens, the value simply gets ignored and the error is returned.
template <typename T, typename ValT = SIMDJSON_IMPLEMENTATION::ondemand::value>
concept nothrow_deserializable = nothrow_custom_deserializable<T, ValT> || is_builtin_deserializable_v<T>;

/// Deserialize Tag
inline constexpr struct deserialize_tag {
  using value_type = SIMDJSON_IMPLEMENTATION::ondemand::value;
  using document_type = SIMDJSON_IMPLEMENTATION::ondemand::document;
  using document_reference_type = SIMDJSON_IMPLEMENTATION::ondemand::document_reference;

  // Customization Point for value
  template <typename T>
    requires custom_deserializable<T, value_type>
  [[nodiscard]] constexpr /* error_code */ auto operator()(value_type &object, T& output) const noexcept(nothrow_custom_deserializable<T, value_type>) {
    return tag_invoke(*this, object, output);
  }

  // Customization Point for document
  template <typename T>
    requires custom_deserializable<T, document_type>
  [[nodiscard]] constexpr /* error_code */ auto operator()(document_type &object, T& output) const noexcept(nothrow_custom_deserializable<T, document_type>) {
    return tag_invoke(*this, object, output);
  }

  // Customization Point for document reference
  template <typename T>
    requires custom_deserializable<T, document_reference_type>
  [[nodiscard]] constexpr /* error_code */ auto operator()(document_reference_type &object, T& output) const noexcept(nothrow_custom_deserializable<T, document_reference_type>) {
    return tag_invoke(*this, object, output);
  }


} deserialize{};

} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_DESERIALIZE_H
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION

