namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline raw_json_string::raw_json_string() noexcept : buf{nullptr} {} // for constructing a simdjson_result
simdjson_really_inline raw_json_string::raw_json_string(const uint8_t * _buf) noexcept : buf{_buf} {}
simdjson_really_inline raw_json_string::raw_json_string(const raw_json_string &other) noexcept : buf{other.buf} {}
simdjson_really_inline raw_json_string &raw_json_string::operator=(const raw_json_string &other) noexcept { buf = other.buf; return *this; }
simdjson_really_inline const char * raw_json_string::raw() const noexcept { return (const char *)buf; }
simdjson_really_inline SIMDJSON_WARN_UNUSED simdjson_result<std::string_view> raw_json_string::unescape(uint8_t *&dst) const noexcept {
  uint8_t *end = stage2::stringparsing::parse_string(buf, dst);
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

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
