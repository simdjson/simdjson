namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

// clang 6 doesn't think the default constructor can be noexcept, so we make it explicit
simdjson_really_inline field::field() noexcept : std::pair<raw_json_string, ondemand::value>() {}
simdjson_really_inline field::field(field &&other) noexcept = default;
simdjson_really_inline field &field::operator=(field &&other) noexcept = default;

simdjson_really_inline field::field(const uint8_t *key, ondemand::value &&value) noexcept
  : std::pair<raw_json_string, ondemand::value>(key, std::forward<ondemand::value>(value))
{
}

simdjson_really_inline simdjson_result<field> field::start(document *doc) noexcept {
  const uint8_t *key = doc->iter.advance();
  if ('"' != *key) {
    logger::log_error(doc->iter, "Missing key in object field");
    return { doc, TAPE_ERROR };
  }
  return field::start(doc, key);
}

simdjson_really_inline simdjson_result<field> field::start(document *doc, const uint8_t *key) noexcept {
  if (':' != *doc->iter.advance()) {
    logger::log_error(doc->iter, "Missing colon in object field");
    return { doc, TAPE_ERROR };
  }
  return field(key, value::start(doc));
}

simdjson_really_inline simdjson_result<value> field::start_value(document *doc) noexcept {
  if (':' != *doc->iter.advance()) {
    logger::log_error(doc->iter, "Missing colon in object field");
    return { doc, TAPE_ERROR };
  }
  return value::start(doc);
}

simdjson_really_inline raw_json_string field::key() const noexcept {
  return first;
}

simdjson_really_inline value &field::value() noexcept {
  return second;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::field &&value
) noexcept :
    internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::field>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::field>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::document *doc,
  error_code error
) noexcept :
    internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::field>(
      { nullptr, { doc, nullptr } },
      error
    )
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::key() noexcept {
  if (error()) { return error(); }
  return first.key();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field>::value() noexcept {
  return { std::move(first.value()), error() };
}

} // namespace simdjson
