#ifndef SIMDJSON_ONDEMAND_DESERIALIZE_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_ONDEMAND_DESERIALIZE_H
#endif // SIMDJSON_CONDITIONAL_INCLUDE
#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#ifdef __cpp_concepts
#include <utility>
#endif

namespace simdjson {

#ifdef __cpp_concepts
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

namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {
class value;
class document;
}
} // namespace SIMDJSON_IMPLEMENTATION

struct deserialize_tag;

template <typename T, typename ValT = SIMDJSON_IMPLEMENTATION::ondemand::value>
concept deserializable = tag_invocable<deserialize_tag, ValT&, T&>;

template <typename T, typename ValT = SIMDJSON_IMPLEMENTATION::ondemand::value>
concept nothrow_deserializable = nothrow_tag_invocable<deserialize_tag, ValT&, T&>;

/// Deserialize Tag
inline constexpr struct deserialize_tag {
  using value_type = SIMDJSON_IMPLEMENTATION::ondemand::value;
  using document_type = SIMDJSON_IMPLEMENTATION::ondemand::document;

  // Customization Point for value
  template <typename T>
    requires deserializable<T, value_type>
  [[nodiscard]] constexpr /* error_code */ auto operator()(value_type &object, T& output) const noexcept(nothrow_deserializable<T, value_type>) {
    return tag_invoke(*this, object, output);
  }

  // Customization Point for document
  template <typename T>
    requires deserializable<T, document_type>
  [[nodiscard]] constexpr /* error_code */ auto operator()(document_type &object, T& output) const noexcept(nothrow_deserializable<T, document_type>) {
    return tag_invoke(*this, object, output);
  }

  // default implementations can also be done here
} deserialize{};

#endif

} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_DESERIALIZE_H
