#ifndef SIMDJSON_INLINE_PADDED_STRING_H
#define SIMDJSON_INLINE_PADDED_STRING_H

#include "simdjson/portability.h"
#include "simdjson/common_defs.h" // for SIMDJSON_PADDING

#include <climits>
#include <cstring>
#include <memory>
#include <string>

namespace simdjson {
namespace internal {

// The allocate_padded_buffer function is a low-level function to allocate memory 
// with padding so we can read past the "length" bytes safely. It is used by 
// the padded_string class automatically. It returns nullptr in case
// of error: the caller should check for a null pointer.
// The length parameter is the maximum size in bytes of the string. 
// The caller is responsible to free the memory (e.g., delete[] (...)).
inline char *allocate_padded_buffer(size_t length) noexcept {
  size_t totalpaddedlength = length + SIMDJSON_PADDING;
  char *padded_buffer = new (std::nothrow) char[totalpaddedlength];
  if (padded_buffer == nullptr) {
    return nullptr;
  }
  // We write zeroes in the padded region to avoid having uninitized 
  // garbage. If nothing else, garbage getting read might trigger a 
  // warning in a memory checking.
  memset(padded_buffer + length, 0, totalpaddedlength - length);
  return padded_buffer;
} // allocate_padded_buffer()

} // namespace internal


inline padded_string::padded_string() noexcept {}
inline padded_string::padded_string(size_t length) noexcept
    : viable_size(length), data_ptr(internal::allocate_padded_buffer(length)) {
  if (data_ptr != nullptr)
    data_ptr[length] = '\0'; // easier when you need a c_str
}
inline padded_string::padded_string(const char *data, size_t length) noexcept
    : viable_size(length), data_ptr(internal::allocate_padded_buffer(length)) {
  if ((data != nullptr) and (data_ptr != nullptr)) {
    memcpy(data_ptr, data, length);
    data_ptr[length] = '\0'; // easier when you need a c_str
  }
}
// note: do not pass std::string arguments by value
inline padded_string::padded_string(const std::string & str_ ) noexcept
    : viable_size(str_.size()), data_ptr(internal::allocate_padded_buffer(str_.size())) {
  if (data_ptr != nullptr) {
    memcpy(data_ptr, str_.data(), str_.size());
    data_ptr[str_.size()] = '\0'; // easier when you need a c_str
  }
}
// note: do pass std::string_view arguments by value
inline padded_string::padded_string(std::string_view sv_) noexcept
    : viable_size(sv_.size()), data_ptr(internal::allocate_padded_buffer(sv_.size())) {
  if (data_ptr != nullptr) {
    memcpy(data_ptr, sv_.data(), sv_.size());
    data_ptr[sv_.size()] = '\0'; // easier when you need a c_str
  }
}
inline padded_string::padded_string(padded_string &&o) noexcept
    : viable_size(o.viable_size), data_ptr(o.data_ptr) {
  o.data_ptr = nullptr; // we take ownership
}

inline padded_string &padded_string::operator=(padded_string &&o) noexcept {
  delete[] data_ptr;
  data_ptr = o.data_ptr;
  viable_size = o.viable_size;
  o.data_ptr = nullptr; // we take ownership
  o.viable_size = 0;
  return *this;
}

inline void padded_string::swap(padded_string &o) noexcept {
  size_t tmp_viable_size = viable_size;
  char *tmp_data_ptr = data_ptr;
  viable_size = o.viable_size;
  data_ptr = o.data_ptr;
  o.data_ptr = tmp_data_ptr;
  o.viable_size = tmp_viable_size;
}

inline padded_string::~padded_string() noexcept {
  delete[] data_ptr;
}

inline size_t padded_string::size() const noexcept { return viable_size; }

inline size_t padded_string::length() const noexcept { return viable_size; }

inline const char *padded_string::data() const noexcept { return data_ptr; }

inline char *padded_string::data() noexcept { return data_ptr; }

inline padded_string::operator std::string_view() const { return std::string_view(data(), length()); }

inline simdjson_result<padded_string> padded_string::load(const std::string &filename) noexcept {
  // Open the file
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (fp == nullptr) {
    return IO_ERROR;
  }

  // Get the file size
  if(std::fseek(fp, 0, SEEK_END) < 0) {
    std::fclose(fp);
    return IO_ERROR;
  }
  long llen = std::ftell(fp);
  if((llen < 0) || (llen == LONG_MAX)) {
    std::fclose(fp);
    return IO_ERROR;
  }

  // Allocate the padded_string
  size_t len = (size_t) llen;
  padded_string s(len);
  if (s.data() == nullptr) {
    std::fclose(fp);
    return MEMALLOC;
  }

  // Read the padded_string
  std::rewind(fp);
  size_t bytes_read = std::fread(s.data(), 1, len, fp);
  if (std::fclose(fp) != 0 || bytes_read != len) {
    return IO_ERROR;
  }

  return s;
}

} // namespace simdjson

#endif // SIMDJSON_INLINE_PADDED_STRING_H
