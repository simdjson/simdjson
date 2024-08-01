#ifndef SIMDJSON_GENERIC_ONDEMAND_FIELD_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_FIELD_INL_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/field.h"
#include "simdjson/generic/ondemand/value-inl.h"
#include "simdjson/generic/ondemand/value_iterator-inl.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

// clang 6 does not think the default constructor can be noexcept, so we make it explicit
simdjson_inline field::field() noexcept : std::pair<raw_json_string, ondemand::value>() {}

simdjson_inline field::field(raw_json_string key, ondemand::value &&value) noexcept
  : std::pair<raw_json_string, ondemand::value>(key, std::forward<ondemand::value>(value))
{
}

simdjson_inline simdjson_result<field> field::start(value_iterator &parent_iter) noexcept {
  raw_json_string key;
  SIMDJSON_TRY( parent_iter.field_key().get(key) );
  SIMDJSON_TRY( parent_iter.field_value() );
  return field::start(parent_iter, key);
}

simdjson_inline simdjson_result<field> field::start(const value_iterator &parent_iter, raw_json_string key) noexcept {
    return field(key, parent_iter.child());
}

simdjson_inline simdjson_warn_unused simdjson_result<std::string_view> field::unescaped_key(bool allow_replacement) noexcept {
  SIMDJSON_ASSUME(first.buf != nullptr); // We would like to call .alive() but Visual Studio won't let us.
  simdjson_result<std::string_view> answer = first.unescape(second.iter.json_iter(), allow_replacement);
  first.consume();
  return answer;
}

template <typename string_type>
simdjson_inline simdjson_warn_unused error_code field::unescaped_key(string_type& receiver, bool allow_replacement) noexcept {
  std::string_view key;
  SIMDJSON_TRY( unescaped_key(allow_replacement).get(key) );
  receiver = key;
  return SUCCESS;
}

simdjson_inline raw_json_string field::key() const noexcept {
  SIMDJSON_ASSUME(first.buf != nullptr); // We would like to call .alive() by Visual Studio won't let us.
  return first;
}


simdjson_inline std::string_view field::key_raw_json_token() const noexcept {
  SIMDJSON_ASSUME(first.buf != nullptr); // We would like to call .alive() by Visual Studio won't let us.
  return std::string_view(reinterpret_cast<const char*>(first.buf-1), second.iter._json_iter->token.peek(-1) - first.buf + 1);
}

simdjson_inline std::string_view field::escaped_key() const noexcept {
  SIMDJSON_ASSUME(first.buf != nullptr); // We would like to call .alive() by Visual Studio won't let us.
  auto end_quote = second.iter._json_iter->token.peek(-1);
  while(*end_quote != '"') end_quote--;
  return std::string_view(reinterpret_cast<const char*>(first.buf), end_quote - first.buf);
}

simdjson_inline value &field::value() & noexcept {
  return second;
}

simdjson_inline value field::value() && noexcept {
  return std::forward<field>(*this).second;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::field &&value
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::field>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::field>(value)
    )
{
}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::simdjson_result(
  error_code error
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::field>(error)
{
}

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::key() noexcept {
  if (error()) { return error(); }
  return first.key();
}

simdjson_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::key_raw_json_token() noexcept {
  if (error()) { return error(); }
  return first.key_raw_json_token();
}

simdjson_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::escaped_key() noexcept {
  if (error()) { return error(); }
  return first.escaped_key();
}

simdjson_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::unescaped_key(bool allow_replacement) noexcept {
  if (error()) { return error(); }
  return first.unescaped_key(allow_replacement);
}

template<typename string_type>
simdjson_inline error_code simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::unescaped_key(string_type &receiver, bool allow_replacement) noexcept {
  if (error()) { return error(); }
  return first.unescaped_key(receiver, allow_replacement);
}

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::value() noexcept {
  if (error()) { return error(); }
  return std::move(first.value());
}

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_FIELD_INL_H