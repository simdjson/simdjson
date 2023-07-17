#ifndef SIMDJSON_GENERIC_SINGLESTAGE_FIELD_INL_H

#ifndef SIMDJSON_AMALGAMATED
#define SIMDJSON_GENERIC_SINGLESTAGE_FIELD_INL_H
#include "simdjson/generic/singlestage/base.h"
#include "simdjson/generic/singlestage/field.h"
#include "simdjson/generic/singlestage/value-inl.h"
#include "simdjson/generic/singlestage/value_iterator-inl.h"
#endif // SIMDJSON_AMALGAMATED

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace singlestage {

// clang 6 doesn't think the default constructor can be noexcept, so we make it explicit
simdjson_inline field::field() noexcept : std::pair<raw_json_string, singlestage::value>() {}

simdjson_inline field::field(raw_json_string key, singlestage::value &&value) noexcept
  : std::pair<raw_json_string, singlestage::value>(key, std::forward<singlestage::value>(value))
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

simdjson_inline raw_json_string field::key() const noexcept {
  SIMDJSON_ASSUME(first.buf != nullptr); // We would like to call .alive() by Visual Studio won't let us.
  return first;
}

simdjson_inline value &field::value() & noexcept {
  return second;
}

simdjson_inline value field::value() && noexcept {
  return std::forward<field>(*this).second;
}

} // namespace singlestage
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::field>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::singlestage::field &&value
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::singlestage::field>(
      std::forward<SIMDJSON_IMPLEMENTATION::singlestage::field>(value)
    )
{
}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::field>::simdjson_result(
  error_code error
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::singlestage::field>(error)
{
}

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::field>::key() noexcept {
  if (error()) { return error(); }
  return first.key();
}
simdjson_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::field>::unescaped_key(bool allow_replacement) noexcept {
  if (error()) { return error(); }
  return first.unescaped_key(allow_replacement);
}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::value> simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::field>::value() noexcept {
  if (error()) { return error(); }
  return std::move(first.value());
}

} // namespace simdjson

#endif // SIMDJSON_GENERIC_SINGLESTAGE_FIELD_INL_H