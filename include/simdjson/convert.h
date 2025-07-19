#ifndef SIMDJSON_CONVERT_H
#define SIMDJSON_CONVERT_H
#if __cpp_concepts

#include "simdjson/ondemand.h"
#include <optional>

namespace simdjson {

template <typename ParserType = ondemand::parser>
struct [[nodiscard]] auto_parser {
private:
  ParserType m_parser;
  padded_string_view m_str;

  template <typename T>
  static constexpr bool is_nothrow_gettable = requires(ondemand::document doc) {
    { doc.get<T>() } noexcept;
  };

public:
  explicit auto_parser(ParserType &&parser, padded_string_view str)
      : m_parser{std::move(parser)}, m_str{str} {}
  explicit auto_parser(std::remove_pointer_t<ParserType> &parser,
                       padded_string_view str)
    requires(std::is_pointer_v<ParserType>)
      : m_parser{&parser}, m_str{str} {}
  explicit auto_parser(ParserType parser, padded_string_view str)
    requires(std::is_pointer_v<ParserType>)
      : m_parser{parser}, m_str{str} {}
  explicit auto_parser(padded_string_view const str) : m_str{str} {}
  auto_parser(auto_parser const &) = delete;
  auto_parser &operator=(auto_parser const &) = delete;
  auto_parser(auto_parser &&) noexcept = default;
  auto_parser &operator=(auto_parser &&) noexcept = default;
  ~auto_parser() = default;

  template <typename T>
  simdjson_inline simdjson_result<T> result() noexcept(is_nothrow_gettable<T>) {
    ondemand::document doc = m_parser.iterate(m_str);
    return doc.get<T>();
  }

  template <typename T>
  simdjson_inline explicit(false)
  operator simdjson_result<T>() noexcept(is_nothrow_gettable<T>) {
    return result<T>();
  }

  /// Get the parser
  std::remove_pointer_t<ParserType> &parser() noexcept {
    if constexpr (std::is_pointer_v<ParserType>) {
      return *m_parser;
    } else {
      return m_parser;
    }
  }

  template <typename T>
  simdjson_inline explicit(false) operator T() noexcept(false) {
    ondemand::document doc = parser().iterate(m_str);
    return doc.get<T>();
  }

  // We can't have "operator std::optional<T>" because it would create an
  // ambiguity for the compiler.
  // We also cannot have "operator T*" without manual memory management.
  // We also cannot have "operator T&" without manual memory management either.

  template <typename T>
  simdjson_inline std::optional<T> optional() noexcept(is_nothrow_gettable<T>) {
    // For std::optional<T>
    ondemand::document doc = parser().iterate(m_str);
    auto res = doc.get<T>();
    if (res.error()) [[unlikely]] {
      return std::nullopt;
    }
    return {res.value()};
  }
};

/**
 * Parse input string into any object if possible.
 */
simdjson_inline auto to(padded_string_view const str) noexcept {
  return auto_parser{str};
}

/**
 * Parse the input using the specified parser into any object if possible.
 */
simdjson_inline auto to(ondemand::parser &parser,
                        padded_string_view const str) noexcept {
  return auto_parser<ondemand::parser *>{parser, str};
}

} // namespace simdjson
#endif // __cpp_concepts
#endif // SIMDJSON_CONVERT_H
