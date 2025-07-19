#ifndef SIMDJSON_CONVERT_H
#define SIMDJSON_CONVERT_H
#if __cpp_concepts

#include "simdjson/ondemand.h"
#include <optional>
#ifdef __cpp_lib_ranges
#include <ranges>
#endif

namespace simdjson {

/**
 * A Wrapper for simdjson_result<ondemand::array_iterator> in order to make it
 * compatible with ranges (to satisfy std::ranges::input_range).
 */
struct auto_iterator {
  using iterator_category = std::forward_iterator_tag;
  using type = simdjson_result<ondemand::array_iterator>;
  using value_type = simdjson_result<ondemand::value>; // type::value_type
  using reference = value_type &;
  using const_reference = const value_type &;
  using difference_type = std::ptrdiff_t;

private:
  type m_iter;
  value_type m_value;

public:
  constexpr auto_iterator() noexcept = default;
  explicit auto_iterator(type const &iter) noexcept
      : m_iter{iter},
        m_value{m_iter.at_end() || m_iter.error() != SUCCESS ? value_type{}
                                                             : *m_iter} {};
  auto_iterator(auto_iterator const &) = default;
  auto_iterator(auto_iterator &&) = default;
  auto_iterator &operator=(auto_iterator const &) = default;
  auto_iterator &operator=(auto_iterator &&) noexcept = default;

  const_reference operator*() const noexcept { return m_value; }

  auto_iterator &operator++() noexcept {
    ++m_iter;
    m_value =
        m_iter.at_end() || m_iter.error() != SUCCESS ? value_type{} : *m_iter;
    return *this;
  }
  auto_iterator operator++(int) noexcept {
    auto_iterator const tmp = *this;
    operator++();
    return tmp;
  }

  [[nodiscard]] bool operator==(auto_iterator const &other) const noexcept {
    return m_iter == other.m_iter;
  }

  [[nodiscard]] bool operator!=(auto_iterator const &other) const noexcept {
    return m_iter != other.m_iter;
  }
};

template <typename ParserType = ondemand::parser>
struct [[nodiscard]] auto_parser
#if __cpp_lib_ranges >= 202202L
    : std::ranges::range_adaptor_closure<auto_parser<ParserType>>
#endif
{
  using difference_type = std::ptrdiff_t;

private:
  ParserType m_parser;
  ondemand::document m_doc;

  template <typename T>
  static constexpr bool is_nothrow_gettable = requires(ondemand::document doc) {
    { doc.get<T>() } noexcept;
  };

public:
  explicit auto_parser(ParserType &&parser, ondemand::document &&doc) noexcept
      : m_parser{std::move(parser)}, m_doc{std::move(doc)} {}

  explicit auto_parser(ParserType &&parser,
                       padded_string_view const str) noexcept
      : m_parser{std::move(parser)}, m_doc{m_parser.iterate(str)} {}

  explicit auto_parser(std::remove_pointer_t<ParserType> &parser,
                       ondemand::document &&doc) noexcept
    requires(std::is_pointer_v<ParserType>)
      : m_parser{&parser}, m_doc{std::move(doc)} {}

  explicit auto_parser(std::remove_pointer_t<ParserType> &parser,
                       padded_string_view const str) noexcept
    requires(std::is_pointer_v<ParserType>)
      : m_parser{&parser}, m_doc{m_parser->iterate(str)} {}

  explicit auto_parser(ParserType parser, ondemand::document &&doc) noexcept
    requires(std::is_pointer_v<ParserType>)
      : m_parser{parser}, m_doc{std::move(doc)} {}

  explicit auto_parser(padded_string_view const str) noexcept
    requires(!std::is_pointer_v<ParserType>)
      : m_parser{}, m_doc{m_parser.iterate(str)} {}

  auto_parser(auto_parser const &) = delete;
  auto_parser &operator=(auto_parser const &) = delete;
  auto_parser(auto_parser &&) noexcept = default;
  auto_parser &operator=(auto_parser &&) noexcept = default;
  ~auto_parser() = default;

  /// Get the parser
  std::remove_pointer_t<ParserType> &parser() noexcept {
    if constexpr (std::is_pointer_v<ParserType>) {
      return *m_parser;
    } else {
      return m_parser;
    }
  }

  template <typename T>
  simdjson_inline simdjson_result<T> result() noexcept(is_nothrow_gettable<T>) {
    return m_doc.get<T>();
  }

  simdjson_inline simdjson_result<ondemand::array> array() noexcept {
    return result<ondemand::array>();
  }

  simdjson_inline simdjson_result<ondemand::object> object() noexcept {
    return result<ondemand::object>();
  }

  simdjson_inline simdjson_result<ondemand::number> number() noexcept {
    return result<ondemand::number>();
  }

  template <typename T>
  simdjson_inline explicit(false)
  operator simdjson_result<T>() noexcept(is_nothrow_gettable<T>) {
    return result<T>();
  }

  template <typename T>
  simdjson_inline explicit(false) operator T() noexcept(false) {
    return m_doc.get<T>();
  }

  // We can't have "operator std::optional<T>" because it would create an
  // ambiguity for the compiler.
  // We also cannot have "operator T*" without manual memory management.
  // We also cannot have "operator T&" without manual memory management either.

  template <typename T>
  simdjson_inline std::optional<T> optional() noexcept(is_nothrow_gettable<T>) {
    // For std::optional<T>
    auto res = m_doc.get<T>();
    if (res.error()) [[unlikely]] {
      return std::nullopt;
    }
    return {res.value()};
  }

  simdjson_inline auto_iterator begin() noexcept {
    return auto_iterator{m_doc.begin()};
  }
  simdjson_inline auto_iterator end() noexcept {
    return auto_iterator{m_doc.end()};
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

#ifdef __cpp_lib_ranges

template <typename T> consteval auto to() noexcept {
  return
      // filter out the bad types
      std::views::filter(
          [](simdjson_result<ondemand::value> const &obj) noexcept {
            return obj.error() == simdjson::SUCCESS;
          })
      // convert to T
      | std::views::transform([](simdjson_result<ondemand::value> &&obj) {
          return obj.get<T>();
        });
}
#endif

} // namespace simdjson

#endif // __cpp_concepts
#endif // SIMDJSON_CONVERT_H
