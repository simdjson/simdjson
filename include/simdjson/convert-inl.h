
#ifndef SIMDJSON_CONVERT_INL_H
#define SIMDJSON_CONVERT_INL_H

#include "simdjson/convert.h"
#if SIMDJSON_SUPPORTS_CONCEPTS
#include "simdjson/internal/direct_parse.h"
namespace simdjson {
namespace convert {
namespace internal {
// auto_parser method definitions
template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(parser_type &&parser, ondemand::document &&doc) noexcept requires(!std::is_pointer_v<parser_type>)
  : m_parser{std::move(parser)}, m_doc{std::move(doc)}, m_doc_ready{true} {}

template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(parser_type &&parser, padded_string_view const str) noexcept requires(!std::is_pointer_v<parser_type>)
  : m_parser{std::move(parser)}, m_doc{}, m_error{SUCCESS},
    m_direct_buf{reinterpret_cast<const uint8_t *>(str.data())}, m_direct_len{str.size()},
    m_direct_cap{str.capacity()}, m_doc_ready{false} {
  // iterate is deferred so atoms and arrays of atoms can be direct-parsed.
}

template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(std::remove_pointer_t<parser_type> &parser, ondemand::document &&doc) noexcept requires(std::is_pointer_v<parser_type>)
  : m_parser{&parser}, m_doc{std::move(doc)}, m_doc_ready{true} {}

template <typename parser_type>
inline auto_parser<parser_type>::auto_parser(std::remove_pointer_t<parser_type> &parser, padded_string_view const str) noexcept requires(std::is_pointer_v<parser_type>)
  : m_parser{&parser}, m_doc{}, m_error{SUCCESS},
    m_direct_buf{reinterpret_cast<const uint8_t *>(str.data())}, m_direct_len{str.size()},
    m_direct_cap{str.capacity()}, m_doc_ready{false} {
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
inline simdjson_result<T> auto_parser<parser_type>::result() & noexcept(is_nothrow_gettable<T>) {
  if (m_error != SUCCESS) {
    return m_error;
  }
  // Direct single-pass parse for atoms and flat arrays. UNINITIALIZED means
  // "not that shape"; fall back to the full ondemand parser.
  if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t> ||
                std::is_same_v<T, bool> ||
                std::is_same_v<T, std::string> ||
                simdjson::internal::direct::appendable_container_of<T, std::string>) {
    if (m_direct_buf != nullptr) {
      T direct_value{};
      error_code direct_error = simdjson::internal::direct::parse_document(
          m_direct_buf, m_direct_len, direct_value);
      if (direct_error == SUCCESS) {
        return direct_value;
      }
      if (direct_error != UNINITIALIZED) {
        return direct_error;
      }
    }
  } else if constexpr (simdjson::internal::direct::appendable_container_of<T, double>) {
    // The number parser reads ahead, so this needs the padding On Demand needs.
    if (m_direct_buf != nullptr && m_direct_cap >= m_direct_len + SIMDJSON_PADDING) {
      T direct_value{};
      error_code direct_error = simdjson::internal::direct::parse_padded_document(
          m_direct_buf, m_direct_len, direct_value);
      if (direct_error == SUCCESS) {
        return direct_value;
      }
      if (direct_error != UNINITIALIZED) {
        return direct_error;
      }
    }
  }
  if (!m_doc_ready) {
    if (m_direct_buf == nullptr) {
      return UNINITIALIZED;
    }
    padded_string_view view(reinterpret_cast<const char *>(m_direct_buf), m_direct_len, m_direct_cap);
    if constexpr (std::is_pointer_v<parser_type>) {
      m_error = m_parser->iterate(view).get(m_doc);
    } else {
      m_error = m_parser.iterate(view).get(m_doc);
    }
    m_doc_ready = true;
    if (m_error != SUCCESS) {
      return m_error;
    }
  }
  return m_doc.get<T>();
}

template <typename parser_type>
template <typename T>
inline simdjson_result<T> auto_parser<parser_type>::result() && noexcept(is_nothrow_gettable<T>) requires(!views_document<T>) {
  return result<T>();
}

template <typename parser_type>
template <typename T>
simdjson_warn_unused simdjson_inline error_code auto_parser<parser_type>::get(T &value) && noexcept(is_nothrow_gettable<T>) requires(!views_document<T>) {
  return result<T>().get(value);
}

template <typename parser_type>
inline simdjson_result<ondemand::array> auto_parser<parser_type>::array() & noexcept {
  return result<ondemand::array>();
}

template <typename parser_type>
inline simdjson_result<ondemand::object> auto_parser<parser_type>::object() & noexcept {
  return result<ondemand::object>();
}

template <typename parser_type>
inline simdjson_result<ondemand::number> auto_parser<parser_type>::number() noexcept {
  return result<ondemand::number>();
}

#if SIMDJSON_EXCEPTIONS
template <typename parser_type>
template <typename T>
inline auto_parser<parser_type>::operator T() & noexcept(false) {
  // Use result() so direct-parse and deferred iterate both run.
  T out;
  error_code err = result<T>().get(out);
  if (err != SUCCESS) {
    throw simdjson_error(err);
  }
  return out;
}

template <typename parser_type>
template <typename T>
inline auto_parser<parser_type>::operator T() && noexcept(false) requires(!views_document<T>) {
  T out;
  error_code err = result<T>().get(out);
  if (err != SUCCESS) {
    throw simdjson_error(err);
  }
  return out;
}
#endif // SIMDJSON_EXCEPTIONS

template <typename parser_type>
template <typename T>
inline std::optional<T> auto_parser<parser_type>::optional() && noexcept(is_nothrow_gettable<T>) requires(!views_document<T>) {
  return optional<T>();
}

template <typename parser_type>
template <typename T>
inline std::optional<T> auto_parser<parser_type>::optional() & noexcept(is_nothrow_gettable<T>) {
  if (m_error != SUCCESS) {
    return std::nullopt;
  }
  T value;
  if (result<T>().get(value)) [[unlikely]] {
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
} // namespace internal
} // namespace convert
} // namespace simdjson
#endif // SIMDJSON_SUPPORTS_CONCEPTS
#endif // SIMDJSON_CONVERT_INL_H
