#ifndef SIMDJSON_PADDED_STRING_VIEW_INL_H
#define SIMDJSON_PADDED_STRING_VIEW_INL_H

#include "simdjson/portability.h"
#include "simdjson/common_defs.h" // for SIMDJSON_PADDING

#include <climits>
#include <cstring>
#include <memory>
#include <string>

namespace simdjson {

inline padded_string_view::padded_string_view(const char* s, size_t len, size_t capacity) noexcept
  : std::string_view(s, len), _capacity(capacity)
{
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
}

inline size_t padded_string_view::capacity() const noexcept { return _capacity; }

inline size_t padded_string_view::padding() const noexcept { return capacity() - length(); }

} // namespace simdjson

#endif // SIMDJSON_PADDED_STRING_VIEW_INL_H
