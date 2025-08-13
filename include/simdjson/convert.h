#ifndef SIMDJSON_CONVERT_H
#define SIMDJSON_CONVERT_H
#if __cpp_concepts

#include "simdjson/ondemand.h"
#include <optional>

namespace simdjson {

template <typename ParserType = ondemand::parser*>
struct auto_parser
{
  using value_type = simdjson_result<ondemand::value>;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;

private:
  ParserType m_parser;
  ondemand::document m_doc;
  error_code m_error{SUCCESS};

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
  simdjson_warn_unused std::remove_pointer_t<ParserType> &parser() noexcept {
    if constexpr (std::is_pointer_v<ParserType>) {
      return *m_parser;
    } else {
      return m_parser;
    }
  }

  template <typename T>
  simdjson_warn_unused simdjson_inline simdjson_result<T>
  result() noexcept(is_nothrow_gettable<T>) {
    if (m_error != SUCCESS) {
      return m_error;
    }
    // For array and object types, we need to be at the start of the document
    return m_doc.get<T>();
  }

  simdjson_warn_unused simdjson_inline simdjson_result<ondemand::array>
  array() noexcept {
    return result<ondemand::array>();
  }

  simdjson_warn_unused simdjson_inline simdjson_result<ondemand::object>
  object() noexcept {
    return result<ondemand::object>();
  }

  simdjson_warn_unused simdjson_inline simdjson_result<ondemand::number>
  number() noexcept {
    return result<ondemand::number>();
  }

  template <typename T>
  simdjson_warn_unused simdjson_inline explicit(false)
  operator simdjson_result<T>() noexcept(is_nothrow_gettable<T>) {
    return result<T>();
  }

  template <typename T>
  simdjson_warn_unused simdjson_inline explicit(false) operator T() noexcept(false) {
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
  simdjson_warn_unused simdjson_inline std::optional<T>
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
};



template <typename T = void>
struct to_adaptor {

  /// Convert to T
  T operator()(simdjson_result<ondemand::value> &val) const noexcept {
    return val.get<T>();
  }
  /**
   * Parse input string into any object if possible. This function call
   * will return an auto_parser that can be used to extract the desired
   * object from the input string. The ondemand::parser instance is created
   * internally.
   *
   * This function uses the simdjson::ondemand::parser::get_parser() instance.
   * A parser should only be used for one document at a time.
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
 * std::map<std::string, std::string> obj =
 *   simdjson::from(R"({"key": "value"})"_padded);
 * ```
 *
 * This will parse the JSON string and return an object representation. By default, we
 * use the simdjson::ondemand::parser::get_parser() instance. A parser instance should
 * be used for just one document at a time.
 *
 * You can also pass you own parser instance:
 * ```cpp
 * simdjson::ondemand::parser parser;
 * std::map<std::string, std::string> obj =
 *   simdjson::from(parser, R"({"key": "value"})"_padded);
 * ```
 * The parser instance can be reused.
 *
 */
static constexpr to_adaptor<> from{};

} // namespace simdjson

#endif // __cpp_concepts
#endif // SIMDJSON_CONVERT_H