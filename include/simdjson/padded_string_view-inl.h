#ifndef SIMDJSON_PADDED_STRING_VIEW_INL_H
#define SIMDJSON_PADDED_STRING_VIEW_INL_H

#include "simdjson/portability.h"
#include "simdjson/common_defs.h" // for SIMDJSON_PADDING

#include <climits>
#include <cstring>
#include <memory>
#include <string>

namespace simdjson {

inline padded_string_view::padded_string_view(const char* s, size_t s_len) noexcept
  : std::string_view(s, s_len)
{
}

inline padded_string_view::padded_string_view(const uint8_t* s, size_t s_len) noexcept
  : padded_string_view(reinterpret_cast<const char*>(s), s_len)
{
}

inline padded_string_view::padded_string_view(std::string_view s) noexcept
  : std::string_view(s)
{
}

inline padded_string_view promise_padded(const char* s, uint8_t s_len) noexcept {
  return padded_string_view(s, s_len);
}

inline padded_string_view promise_padded(std::string_view s) noexcept {
  return padded_string_view(s);
}

} // namespace simdjson

#endif // SIMDJSON_PADDED_STRING_VIEW_INL_H
