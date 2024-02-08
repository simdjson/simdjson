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

inline padded_string_view::padded_string_view(const std::string &s) noexcept
  : std::string_view(s), _capacity(s.capacity())
{
}

inline padded_string_view::padded_string_view(std::string_view s, size_t capacity) noexcept
  : std::string_view(s), _capacity(capacity)
{
  if(_capacity < s.length()) { _capacity = s.length(); }
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

} // namespace simdjson


#endif // SIMDJSON_PADDED_STRING_VIEW_INL_H
