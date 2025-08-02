#ifndef SIMDJSON_CONVERT_H
#define SIMDJSON_CONVERT_H
#if __cpp_concepts

#include "simdjson/ondemand.h"
#include <optional>
#ifdef __cpp_lib_ranges
#include <ranges>
#endif

namespace simdjson {

struct [[nodiscard]] auto_iterator_end {};

/**
 * A Wrapper for simdjson_result<ondemand::array_iterator> in order to make it
 * compatible with ranges (to satisfy std::ranges::input_range).
 */
struct [[nodiscard]] auto_iterator {
  using iterator_category = std::forward_iterator_tag;
  using type = simdjson_result<ondemand::array_iterator>;
  using value_type = simdjson_result<ondemand::value>; // type::value_type
  using reference = value_type &;
  using const_reference = const value_type &;
  using difference_type = std::ptrdiff_t;

  struct auto_iterator_storage {
    type m_iter{};
    mutable value_type m_value{};
  };

private:
  auto_iterator_storage *m_storage = nullptr;

public:
  constexpr auto_iterator() noexcept = default;
  explicit auto_iterator(auto_iterator_storage &storage) noexcept
      : m_storage{&storage} {};
  auto_iterator(auto_iterator const &) = default;
  auto_iterator(auto_iterator &&) = default;
  auto_iterator &operator=(auto_iterator const &) = default;
  auto_iterator &operator=(auto_iterator &&) noexcept = default;
  ~auto_iterator() = default;

  reference operator*() const noexcept { return m_storage->m_value; }
  reference operator*() noexcept { return m_storage->m_value; }

  auto_iterator &operator++() noexcept {
    ++m_storage->m_iter;
    m_storage->m_value =
        m_storage->m_iter.at_end() || m_storage->m_iter.error() != SUCCESS
            ? value_type{}
            : *m_storage->m_iter;
    return *this;
  }
  auto_iterator operator++(int) noexcept {
    auto_iterator const tmp = *this;
    operator++();
    return tmp;
  }

  [[nodiscard]] bool operator==(auto_iterator const &other) const noexcept {
    return m_storage == other.m_storage &&
           m_storage->m_iter == other.m_storage->m_iter;
  }

  [[nodiscard]] bool operator==(auto_iterator_end) const noexcept {
    return m_storage != nullptr && m_storage->m_iter.at_end();
  }
};

template <typename ParserType = ondemand::parser>
struct [[nodiscard]] auto_parser
#if __cpp_lib_ranges
    : std::ranges::view_interface<auto_parser<ParserType>>
#endif
{
  using value_type = simdjson_result<ondemand::value>;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using iterator = auto_iterator;
  using const_iterator = auto_iterator; // auto_iterator is already const

private:
  ParserType m_parser;
  ondemand::document m_doc;

  // Caching the iterator here:
  iterator::auto_iterator_storage iter_storage{};

  template <typename T>
  static constexpr bool is_nothrow_gettable = requires(ondemand::document doc) {
    { doc.get<T>() } noexcept;
  };

public:
  // non-pointer constructors:
  explicit auto_parser(ParserType &&parser, ondemand::document &&doc) noexcept
    requires(!std::is_pointer_v<ParserType>)
      : m_parser{std::move(parser)}, m_doc{std::move(doc)} {}

  explicit auto_parser(ParserType &&parser,
                       padded_string_view const str) noexcept
    requires(!std::is_pointer_v<ParserType>)
      : m_parser{std::move(parser)}, m_doc{m_parser.iterate(str)} {}

  explicit auto_parser(padded_string_view const str) noexcept
    requires(!std::is_pointer_v<ParserType>)
      : auto_parser{ParserType{}, str} {}

  // pointer constructors:
  explicit auto_parser(std::remove_pointer_t<ParserType> &parser,
                       ondemand::document &&doc) noexcept
    requires(std::is_pointer_v<ParserType>)
      : m_parser{&parser}, m_doc{std::move(doc)} {}

  explicit auto_parser(std::remove_pointer_t<ParserType> &parser,
                       padded_string_view const str) noexcept
    requires(std::is_pointer_v<ParserType>)
      : auto_parser{parser, parser.iterate(str)} {}

  explicit auto_parser(ParserType parser, ondemand::document &&doc) noexcept
    requires(std::is_pointer_v<ParserType>)
      : auto_parser{*parser, std::move(doc)} {}

  auto_parser(auto_parser const &) = delete;
  auto_parser &operator=(auto_parser const &) = delete;
  auto_parser(auto_parser &&) noexcept = default;
  auto_parser &operator=(auto_parser &&) noexcept = default;
  ~auto_parser() = default;

  /// Get the parser
  [[nodiscard]] std::remove_pointer_t<ParserType> &parser() noexcept {
    if constexpr (std::is_pointer_v<ParserType>) {
      return *m_parser;
    } else {
      return m_parser;
    }
  }

  template <typename T>
  [[nodiscard]] simdjson_inline simdjson_result<T>
  result() noexcept(is_nothrow_gettable<T>) {
    return m_doc.get<T>();
  }

  [[nodiscard]] simdjson_inline simdjson_result<ondemand::array>
  array() noexcept {
    return result<ondemand::array>();
  }

  [[nodiscard]] simdjson_inline simdjson_result<ondemand::object>
  object() noexcept {
    return result<ondemand::object>();
  }

  [[nodiscard]] simdjson_inline simdjson_result<ondemand::number>
  number() noexcept {
    return result<ondemand::number>();
  }

  template <typename T>
  [[nodiscard]] simdjson_inline explicit(false)
  operator simdjson_result<T>() noexcept(is_nothrow_gettable<T>) {
    return result<T>();
  }

  template <typename T>
  [[nodiscard]] simdjson_inline explicit(false) operator T() noexcept(false) {
    return m_doc.get<T>();
  }

  // We can't have "operator std::optional<T>" because it would create an
  // ambiguity for the compiler.
  // We also cannot have "operator T*" without manual memory management.
  // We also cannot have "operator T&" without manual memory management either.

  template <typename T>
  [[nodiscard]] simdjson_inline std::optional<T>
  optional() noexcept(is_nothrow_gettable<T>) {
    // For std::optional<T>
    auto res = m_doc.get<T>();
    if (res.error()) [[unlikely]] {
      return std::nullopt;
    }
    return {res.value()};
  }

  simdjson_inline auto_iterator begin() noexcept {
    if (iter_storage.m_iter.error() != SUCCESS &&
        !iter_storage.m_iter.at_end()) {
      iter_storage = {.m_iter = iterator::type{m_doc.begin()},
                      .m_value = iterator::value_type{
                          iter_storage.m_iter.at_end() ||
                                  iter_storage.m_iter.error() != SUCCESS
                              ? value_type{}
                              : *iter_storage.m_iter}};
    }
    return auto_iterator{iter_storage};
  }
  simdjson_inline auto_iterator_end end() noexcept { return {}; }
};

#ifdef __cpp_lib_ranges
#if __cpp_lib_ranges_zip >= 202110L

static constexpr struct [[nodiscard]] no_errors_adaptor
    : std::ranges::range_adaptor_closure<no_errors_adaptor> {

  [[nodiscard]] constexpr bool
  operator()(simdjson_result<ondemand::value> const &val) const noexcept {
    return val.error() == SUCCESS;
  }

  template <std::ranges::range Range>
  constexpr auto operator()(Range &&rng) const noexcept {
    return std::forward<Range>(rng) | std::views::filter(*this);
  }
} no_errors;

template <typename T = void>
struct [[nodiscard]] to_adaptor
    : std::ranges::range_adaptor_closure<to_adaptor<T>> {

  /// Convert to T
  [[nodiscard]] constexpr T
  operator()(simdjson_result<ondemand::value> &val) const noexcept {
    return val.get<T>();
  }

  /// Make it an adaptor
  template <std::ranges::range Range>
  constexpr auto operator()(Range &&rng) const noexcept {
    return std::forward<Range>(rng) | no_errors | std::views::transform(*this);
  }

  /**
   * Parse input string into any object if possible.
   */
  constexpr auto operator()(padded_string_view const str) const noexcept {
    return auto_parser{str};
  }

  /**
   * Parse the input using the specified parser into any object if possible.
   */
  constexpr auto operator()(ondemand::parser &parser,
                            padded_string_view const str) const noexcept {
    return auto_parser<ondemand::parser *>{parser, str};
  }
};

template <typename T> static constexpr to_adaptor<T> to{};

static constexpr to_adaptor<> from{};

#endif
#endif

} // namespace simdjson

#endif // __cpp_concepts
#endif // SIMDJSON_CONVERT_H
