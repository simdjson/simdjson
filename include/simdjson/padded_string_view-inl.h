#ifndef SIMDJSON_PADDED_STRING_VIEW_INL_H
#define SIMDJSON_PADDED_STRING_VIEW_INL_H

#include "simdjson/padded_string_view.h"
#include "simdjson/error-inl.h"

#include <cstring> /* memcmp */

// for page size computation.
#if SIMDJSON_HAS_UNISTD_H
  #include <unistd.h>
  #if defined(__APPLE__)
    #include <sys/sysctl.h>
  #endif
#endif


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

inline uint32_t get_page_size() noexcept {
#if defined(_WINDOWS_) // if and only if someone loaded Windows.h, we can get the page size from there.
// Otherwise, we assume 4096.
    static const uint32_t cached = []() -> uint32_t {
      SYSTEM_INFO si;
      GetSystemInfo(&si);
      return static_cast<std::uint32_t>(si.dwPageSize);
    }();
    return cached;
#elif SIMDJSON_HAS_UNISTD_H
    static const uint32_t cached = []() -> uint32_t {
      long page_size = sysconf(_SC_PAGESIZE);
      if (page_size > 0) {
          return static_cast<uint32_t>(page_size);
      }
      return 4096; // fallback
    }();
    return cached;
#else
    return 4096; // fallback
#endif
}
#if SIMDJSON_CPLUSPLUS17

inline padded_input::padded_input(std::string_view sv)
    : storage(simdjson::padded_string_view{}) {
  if (needs_allocation(sv.data(), sv.size())) {
      storage = simdjson::padded_string(sv);
  } else {
      storage = simdjson::padded_string_view(
          sv.data(), sv.size(), sv.size() + simdjson::SIMDJSON_PADDING);
  }
}

inline padded_input::padded_input(const char *data, size_t length)
    : storage(simdjson::padded_string_view{}) {
  if (needs_allocation(data, length)) {
      storage = simdjson::padded_string(data, length);
  } else {
      storage = simdjson::padded_string_view(
          data, length, length + simdjson::SIMDJSON_PADDING);
  }
}

inline padded_input::padded_input(const std::string &s)
    : storage(simdjson::padded_string_view{}) {
  const size_t len = s.size();
  const size_t cap = s.capacity();
  // Here we have the string content from data() to data() + size(),
  // but the memory is accessible from data() to data() + capacity().
  const size_t needed_padding = (cap - len) < simdjson::SIMDJSON_PADDING
    ? simdjson::SIMDJSON_PADDING - (cap - len) : 0;
  if (needed_padding > 0 && needs_allocation(s.data(), cap, needed_padding)) {
      storage = simdjson::padded_string(s);
  } else {
      storage = simdjson::padded_string_view(
          s.data(), len, len + simdjson::SIMDJSON_PADDING);
  }
}

inline bool padded_input::is_view() const noexcept {
  return std::holds_alternative<simdjson::padded_string_view>(storage);
}

inline padded_input::operator simdjson::padded_string_view() const noexcept {
  return std::visit([](const auto& p) -> simdjson::padded_string_view {
      return p;
  }, storage);
}

inline bool padded_input::needs_allocation(const char* buf, size_t len, size_t padding) noexcept {
  if(len == 0) { return false; }
  const auto page_size = get_page_size();
  return ((reinterpret_cast<uintptr_t>(buf + len - 1) % page_size)
          + padding >= static_cast<uintptr_t>(page_size));
}
#endif // SIMDJSON_CPLUSPLUS17

} // namespace simdjson


#endif // SIMDJSON_PADDED_STRING_VIEW_INL_H
