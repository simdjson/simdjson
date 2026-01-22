#ifndef SIMDJSON_PADDED_STRING_VIEW_INL_H
#define SIMDJSON_PADDED_STRING_VIEW_INL_H

#include "simdjson/padded_string_view.h"
#include "simdjson/error-inl.h"

#include <cstring> /* memcmp */

namespace simdjson {

inline padded_string_view::padded_string_view(const char* s, size_t len, size_t capacity) noexcept
  : std::string_view(s, len), _capacity(capacity)
{
  if(_capacity < len) { _capacity = len; }
}

inline padded_string_view::padded_string_view(const uint8_t* s, size_t len, size_t capacity) noexcept
  : padded_string_view(reinterpret_cast<const char*>(s), len, capacity)
{
}
#ifdef __cpp_char8_t
inline padded_string_view::padded_string_view(const char8_t* s, size_t len, size_t capacity) noexcept
  : padded_string_view(reinterpret_cast<const char*>(s), len, capacity)
{
}
#endif
inline padded_string_view::padded_string_view(const std::string &s) noexcept
  : std::string_view(s), _capacity(s.capacity())
{
}

inline padded_string_view::padded_string_view(std::string_view s, size_t capacity) noexcept
  : std::string_view(s), _capacity(capacity)
{
  if(_capacity < s.length()) { _capacity = s.length(); }
}

inline bool padded_string_view::has_sufficient_padding() const noexcept {
  if (padding() >= SIMDJSON_PADDING) {
    return true;
  }
  size_t missing_padding = SIMDJSON_PADDING - padding();
  if(length() < missing_padding) { return false; }

  for (size_t i = length() - missing_padding; i < length(); i++) {
    char c = data()[i];
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      return false;
    }
  }
  return true;
}

inline size_t padded_string_view::capacity() const noexcept { return _capacity; }

inline size_t padded_string_view::padding() const noexcept { return capacity() - length(); }

inline bool padded_string_view::remove_utf8_bom() noexcept {
  if(length() < 3) { return false; }
  if (std::memcmp(data(), "\xEF\xBB\xBF", 3) == 0) {
    remove_prefix(3);
    _capacity -= 3;
    return true;
  }
  return false;
}

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson_result<padded_string_view> &s) noexcept(false) { return out << s.value(); }
#endif

inline padded_string_view pad(std::string& s) noexcept {
  size_t existing_padding = 0;
  for (size_t i = s.size(); i > 0; i--) {
    char c = s[i - 1];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      existing_padding++;
    } else {
      break;
    }
  }
  size_t needed_padding = 0;
  if (existing_padding < SIMDJSON_PADDING) {
    needed_padding = SIMDJSON_PADDING - existing_padding;
    s.append(needed_padding, ' ');
  }

  return padded_string_view(s.data(), s.size() - needed_padding, s.size());
}

inline padded_string_view pad_with_reserve(std::string& s) noexcept {
  if (s.capacity() - s.size() < SIMDJSON_PADDING) {
    s.reserve(s.size() + SIMDJSON_PADDING );
  }
  return padded_string_view(s.data(), s.size(), s.capacity());
}



} // namespace simdjson


#endif // SIMDJSON_PADDED_STRING_VIEW_INL_H
