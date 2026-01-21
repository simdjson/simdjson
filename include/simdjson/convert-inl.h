
#ifndef SIMDJSON_CONVERT_INL_H
#define SIMDJSON_CONVERT_INL_H

#include "simdjson/convert.h"
#if SIMDJSON_SUPPORTS_CONCEPTS
namespace simdjson {
namespace convert {
namespace internal {
// auto_parser method definitions
template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(parser_type &&parser, ondemand::document &&doc) noexcept requires(!std::is_pointer_v<parser_type>)
  : m_parser{std::move(parser)}, m_doc{std::move(doc)} {}

template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(parser_type &&parser, padded_string_view const str) noexcept requires(!std::is_pointer_v<parser_type>)
  : m_parser{std::move(parser)}, m_doc{}, m_error{SUCCESS} {
  m_error = m_parser.iterate(str).get(m_doc);
}

template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(std::remove_pointer_t<parser_type> &parser, ondemand::document &&doc) noexcept requires(std::is_pointer_v<parser_type>)
  : m_parser{&parser}, m_doc{std::move(doc)} {}

template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(std::remove_pointer_t<parser_type> &parser, padded_string_view const str) noexcept requires(std::is_pointer_v<parser_type>)
  : m_parser{&parser}, m_doc{}, m_error{SUCCESS} {
  m_error = m_parser->iterate(str).get(m_doc);
}

template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(padded_string_view const str) noexcept requires(std::is_pointer_v<parser_type>)
  : auto_parser{ondemand::parser::get_parser(), str} {}

template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(parser_type parser, ondemand::document &&doc) noexcept requires(std::is_pointer_v<parser_type>)
  : auto_parser{*parser, std::move(doc)} {}





template <typename parser_type>
inline std::remove_pointer_t<parser_type> &auto_parser<parser_type>::parser() noexcept {
  if constexpr (std::is_pointer_v<parser_type>) {
    return *m_parser;
  } else {
    return m_parser;
  }
}

template <typename parser_type>
template <typename T>
inline simdjson_result<T> auto_parser<parser_type>::result() noexcept(is_nothrow_gettable<T>) {
  if (m_error != SUCCESS) {
    return m_error;
  }
  return m_doc.get<T>();
}

template <typename parser_type>
template <typename T>
simdjson_warn_unused simdjson_inline error_code auto_parser<parser_type>::get(T &value) && noexcept(is_nothrow_gettable<T>) {
  return result<T>().get(value);
}

template <typename parser_type>
inline simdjson_result<ondemand::array> auto_parser<parser_type>::array() noexcept {
  return result<ondemand::array>();
}

template <typename parser_type>
inline simdjson_result<ondemand::object> auto_parser<parser_type>::object() noexcept {
  return result<ondemand::object>();
}

template <typename parser_type>
inline simdjson_result<ondemand::number> auto_parser<parser_type>::number() noexcept {
  return result<ondemand::number>();
}

#if SIMDJSON_EXCEPTIONS
template <typename parser_type>
template <typename T>
inline auto_parser<parser_type>::operator T() noexcept(false) {
  if (m_error != SUCCESS) {
    throw simdjson_error(m_error);
  }
  return m_doc.get<T>();
}
#endif // SIMDJSON_EXCEPTIONS

template <typename parser_type>
template <typename T>
inline std::optional<T> auto_parser<parser_type>::optional() noexcept(is_nothrow_gettable<T>) {
  if (m_error != SUCCESS) {
    return std::nullopt;
  }
  T value;
  if (m_doc.get<T>().get(value)) [[unlikely]] {
    return std::nullopt;
  }
  return {std::move(value)};
}

// to_adaptor method definitions
template <typename T>
inline T to_adaptor<T>::operator()(simdjson_result<ondemand::value> &val) const noexcept {
  return val.get<T>();
}

template <typename T>
inline auto to_adaptor<T>::operator()(padded_string_view const str) const noexcept {
  return auto_parser<ondemand::parser *>{str};
}

template <typename T>
inline auto to_adaptor<T>::operator()(ondemand::parser &parser, padded_string_view const str) const noexcept {
  return auto_parser<ondemand::parser *>{parser, str};
}

template <typename T>
inline auto to_adaptor<T>::operator()(std::string str) const noexcept {
  return auto_parser<ondemand::parser *>{pad_with_reserve(str)};
}

template <typename T>
inline auto to_adaptor<T>::operator()(ondemand::parser &parser, std::string str) const noexcept {
  return auto_parser<ondemand::parser *>{parser, pad_with_reserve(str)};
}
} // namespace internal
} // namespace convert
} // namespace simdjson
#endif // SIMDJSON_SUPPORTS_CONCEPTS
#endif // SIMDJSON_CONVERT_INL_H
