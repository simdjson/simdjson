namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline raw_json_string::raw_json_string(const uint8_t * _buf) noexcept : buf{_buf} {}

simdjson_really_inline const char * raw_json_string::raw() const noexcept { return (const char *)buf; }
simdjson_really_inline SIMDJSON_WARN_UNUSED simdjson_result<std::string_view> raw_json_string::unescape(uint8_t *&dst) const noexcept {
  uint8_t *end = stringparsing::parse_string(buf, dst);
  if (!end) { return STRING_ERROR; }
  std::string_view result((const char *)dst, end-dst);
  dst = end;
  return result;
}

simdjson_really_inline SIMDJSON_WARN_UNUSED simdjson_result<std::string_view> raw_json_string::unescape(json_iterator &iter) const noexcept {
  return unescape(iter.current_string_buf_loc);
}

SIMDJSON_UNUSED simdjson_really_inline bool operator==(const raw_json_string &a, std::string_view b) noexcept {
  return !memcmp(a.raw(), b.data(), b.size());
}

SIMDJSON_UNUSED simdjson_really_inline bool operator==(std::string_view a, const raw_json_string &b) noexcept {
  return b == a;
}

SIMDJSON_UNUSED simdjson_really_inline bool operator!=(const raw_json_string &a, std::string_view b) noexcept {
  return !(a == b);
}

SIMDJSON_UNUSED simdjson_really_inline bool operator!=(std::string_view a, const raw_json_string &b) noexcept {
  return !(a == b);
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>(error) {}

simdjson_really_inline simdjson_result<const char *> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::raw() const noexcept {
  if (error()) { return error(); }
  return first.raw();
}
simdjson_really_inline SIMDJSON_WARN_UNUSED simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::unescape(uint8_t *&dst) const noexcept {
  if (error()) { return error(); }
  return first.unescape(dst);
}
simdjson_really_inline SIMDJSON_WARN_UNUSED simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string>::unescape(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &iter) const noexcept {
  if (error()) { return error(); }
  return first.unescape(iter);
}

} // namespace simdjson
