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

template <typename ParserType = ondemand::parser*>
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
  error_code m_error{SUCCESS};

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
      : m_parser{std::move(parser)}, m_doc{}, m_error{SUCCESS} {
    m_error = m_parser.iterate(str).get(m_doc);
  }

  // pointer constructors:
  explicit auto_parser(std::remove_pointer_t<ParserType> &parser,
                       ondemand::document &&doc) noexcept
    requires(std::is_pointer_v<ParserType>)
      : m_parser{&parser}, m_doc{std::move(doc)} {}

  explicit auto_parser(std::remove_pointer_t<ParserType> &parser,
                       padded_string_view const str) noexcept
    requires(std::is_pointer_v<ParserType>)
      : m_parser{&parser}, m_doc{}, m_error{SUCCESS} {
    m_error = m_parser->iterate(str).get(m_doc);
  }

  explicit auto_parser(padded_string_view const str) noexcept
    requires(std::is_pointer_v<ParserType>)
      : auto_parser{ondemand::parser::get_parser(), str} {}

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
    if (m_error != SUCCESS) {
      return m_error;
    }
    // For array and object types, we need to be at the start of the document
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
    if (m_error != SUCCESS) {
      throw simdjson_error(m_error);
    }
    return m_doc.get<T>();
  }

  // We can't have "operator std::optional<T>" because it would create an
  // ambiguity for the compiler.
  // We also cannot have "operator T*" without manual memory management.
  // We also cannot have "operator T&" without manual memory management either.

  template <typename T>
  [[nodiscard]] simdjson_inline std::optional<T>
  optional() noexcept(is_nothrow_gettable<T>) {
    if (m_error != SUCCESS) {
      return std::nullopt;
    }
    T value;
    // For std::optional<T>
    if (m_doc.get<T>().get(value)) [[unlikely]] {
      return std::nullopt;
    }
    return {std::move(value)};
  }

  simdjson_inline auto_iterator begin() noexcept {
    if (m_error != SUCCESS) {
      // Create an iterator with the error
      iter_storage.m_iter = iterator::type(m_error);
      iter_storage.m_value = value_type{};
      return auto_iterator{iter_storage};
    }
    if (iter_storage.m_iter.error() != SUCCESS &&
        !iter_storage.m_iter.at_end()) {
      // Try to get the document as an array
      ondemand::array arr;
      if(auto error = m_doc.get_array().get(arr); error == SUCCESS) {
        iter_storage = {.m_iter = iterator::type{arr.begin()},
                        .m_value = iterator::value_type{
                            iter_storage.m_iter.at_end() ||
                                    iter_storage.m_iter.error() != SUCCESS
                                ? value_type{}
                                : *iter_storage.m_iter}};
      } else {
        // If it's not an array, create an error iterator
        iter_storage.m_iter = iterator::type(error);
        iter_storage.m_value = value_type{};
      }
    }
    return auto_iterator{iter_storage};
  }
  simdjson_inline auto_iterator_end end() noexcept { return {}; }
};

#ifdef __cpp_lib_ranges

// For C++20, we implement our own pipe operator since range_adaptor_closure is C++23
static constexpr struct [[nodiscard]] no_errors_adaptor {

  [[nodiscard]] bool
  operator()(simdjson_result<ondemand::value> const &val) const noexcept {
    return val.error() == SUCCESS;
  }

  template <std::ranges::range Range>
  auto operator()(Range &&rng) const noexcept {
    return std::forward<Range>(rng) | std::views::filter(*this);
  }
} no_errors;

template <typename T = void>
struct [[nodiscard]] to_adaptor {

  /// Convert to T
  [[nodiscard]] T
  operator()(simdjson_result<ondemand::value> &val) const noexcept {
    return val.get<T>();
  }

  /// Make it an adaptor
  template <std::ranges::range Range>
  auto operator()(Range &&rng) const noexcept {
    return std::forward<Range>(rng) | no_errors | std::views::transform(*this);
  }

  /**
   * Parse input string into any object if possible. This function call
   * will return an auto_parser that can be used to extract the desired
   * object from the input string. The ondemand::parser instance is created
   * internally.
   *
   * *WARNING*: This function call will create a new parser instance each time
   * it is called. This has performance implications. We strongly recommend
   * that you create a parser instance once and reuse it many times.
   */
  auto operator()(padded_string_view const str) const noexcept {
    return auto_parser{str};
  }

  /**
   * Parse the input using the specified parser into any object if possible.
   * This function call will return an auto_parser that can be used to extract
   * the desired object from the input string. The provided ondemand::parser instance
   * handles memory allocations and indexing, and it can be reused from call to call.
   */
  auto operator()(ondemand::parser &parser,
                            padded_string_view const str) const noexcept {
    return auto_parser<ondemand::parser *>{parser, str};
  }
};

template <typename T> static constexpr to_adaptor<T> to{};

/**
 * The `from` instance is a utility adaptor for parsing JSON strings into objects.
 * It provides a convenient way to convert JSON data into C++ objects using the `auto_parser`.
 *
 * Example usage:
 *
 * ```cpp
 * simdjson::ondemand::parser parser;
 * std::map<std::string, std::string> obj =
 *   simdjson::from(parser, R"({"key": "value"})"_padded);
 * ```
 *
 * This will parse the JSON string and return an object representation. The `ondemand::parser`
 * instance can be reused. It is also possible to omit the parser instance, in which case a parser
 * instance will be created internally, although this can have negative performance consequences.
 */
static constexpr to_adaptor<> from{};

template <typename T = void>
using as = to_adaptor<T>;

// For C++20 ranges without range_adaptor_closure, we need to define pipe operators
template <std::ranges::range Range>
inline auto operator|(Range&& range, const no_errors_adaptor& adaptor) {
  return adaptor(std::forward<Range>(range));
}

template <std::ranges::range Range, typename T>
inline auto operator|(Range&& range, const to_adaptor<T>& adaptor) {
  return adaptor(std::forward<Range>(range));
}

#endif // __cpp_lib_ranges

} // namespace simdjson

#endif // __cpp_concepts
#endif // SIMDJSON_CONVERT_H
