#ifndef SIMDJSON_PADDED_STRING_INL_H
#define SIMDJSON_PADDED_STRING_INL_H

#include "simdjson/padded_string.h"
#include "simdjson/padded_string_view.h"

#include "simdjson/error-inl.h"
#include "simdjson/padded_string_view-inl.h"

#include <climits>
#include <cwchar>

namespace simdjson {
namespace internal {

// The allocate_padded_buffer function is a low-level function to allocate memory
// with padding so we can read past the "length" bytes safely. It is used by
// the padded_string class automatically. It returns nullptr in case
// of error: the caller should check for a null pointer.
// The length parameter is the maximum size in bytes of the string.
// The caller is responsible to free the memory (e.g., delete[] (...)).
inline char *allocate_padded_buffer(size_t length) noexcept {
  const size_t totalpaddedlength = length + SIMDJSON_PADDING;
  if(totalpaddedlength<length) {
    // overflow
    return nullptr;
  }
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
  // avoid getting out of memory
  if (totalpaddedlength>(1UL<<20)) {
    return nullptr;
  }
#endif

  char *padded_buffer = new (std::nothrow) char[totalpaddedlength];
  if (padded_buffer == nullptr) {
    return nullptr;
  }
  // We write nulls in the padded region to avoid having uninitialized
  // content which may trigger warning for some sanitizers
  std::memset(padded_buffer + length, 0, totalpaddedlength - length);
  return padded_buffer;
} // allocate_padded_buffer()

} // namespace internal


inline padded_string::padded_string() noexcept = default;
inline padded_string::padded_string(size_t length) noexcept
    : viable_size(length), data_ptr(internal::allocate_padded_buffer(length)) {
}
inline padded_string::padded_string(const char *data, size_t length) noexcept
    : viable_size(length), data_ptr(internal::allocate_padded_buffer(length)) {
  if ((data != nullptr) && (data_ptr != nullptr)) {
    std::memcpy(data_ptr, data, length);
  }
  if (data_ptr == nullptr) {
    viable_size = 0;
  }
}
#ifdef __cpp_char8_t
inline padded_string::padded_string(const char8_t *data, size_t length) noexcept
    : viable_size(length), data_ptr(internal::allocate_padded_buffer(length)) {
  if ((data != nullptr) && (data_ptr != nullptr)) {
    std::memcpy(data_ptr, reinterpret_cast<const char *>(data), length);
  }
  if (data_ptr == nullptr) {
    viable_size = 0;
  }
}
#endif
// note: do not pass std::string arguments by value
inline padded_string::padded_string(const std::string & str_ ) noexcept
    : viable_size(str_.size()), data_ptr(internal::allocate_padded_buffer(str_.size())) {
  if (data_ptr == nullptr) {
    viable_size = 0;
  } else {
    std::memcpy(data_ptr, str_.data(), str_.size());
  }
}
// note: do pass std::string_view arguments by value
inline padded_string::padded_string(std::string_view sv_) noexcept
    : viable_size(sv_.size()), data_ptr(internal::allocate_padded_buffer(sv_.size())) {
  if(simdjson_unlikely(!data_ptr)) {
    //allocation failed or zero size
    viable_size = 0;
    return;
  }
  if (sv_.size()) {
    std::memcpy(data_ptr, sv_.data(), sv_.size());
  }
}
inline padded_string::padded_string(padded_string &&o) noexcept
    : viable_size(o.viable_size), data_ptr(o.data_ptr) {
  o.data_ptr = nullptr; // we take ownership
  o.viable_size = 0;
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

inline bool padded_string::append(const char *data, size_t length) noexcept {
  if (length == 0) {
    return true; // Nothing to append
  }
  size_t new_size = viable_size + length;
  if (new_size < viable_size) {
    // Overflow, cannot append
    return false;
  }
  char *new_data_ptr = internal::allocate_padded_buffer(new_size);
  if (new_data_ptr == nullptr) {
    // Allocation failed, cannot append
    return false;
  }
  // Copy existing data
  if (viable_size > 0) {
    std::memcpy(new_data_ptr, data_ptr, viable_size);
  }
  // Copy new data
  std::memcpy(new_data_ptr + viable_size, data, length);
  // Update
  delete[] data_ptr;
  data_ptr = new_data_ptr;
  viable_size = new_size;
  return true;
}

inline padded_string::operator std::string_view() const simdjson_lifetime_bound { return std::string_view(data(), length()); }

inline padded_string::operator padded_string_view() const noexcept simdjson_lifetime_bound {
  return padded_string_view(data(), length(), length() + SIMDJSON_PADDING);
}

inline simdjson_result<padded_string> padded_string::load(std::string_view filename) noexcept {
  // Open the file
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = std::fopen(filename.data(), "rb");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (fp == nullptr) {
    return IO_ERROR;
  }

  // Get the file size
  int ret;
#if SIMDJSON_VISUAL_STUDIO && !SIMDJSON_IS_32BITS
  ret = _fseeki64(fp, 0, SEEK_END);
#else
  ret = std::fseek(fp, 0, SEEK_END);
#endif // _WIN64
  if(ret < 0) {
    std::fclose(fp);
    return IO_ERROR;
  }
#if SIMDJSON_VISUAL_STUDIO && !SIMDJSON_IS_32BITS
  __int64 llen = _ftelli64(fp);
  if(llen == -1L) {
    std::fclose(fp);
    return IO_ERROR;
  }
#else
  long llen = std::ftell(fp);
  if((llen < 0) || (llen == LONG_MAX)) {
    std::fclose(fp);
    return IO_ERROR;
  }
#endif

  // Allocate the padded_string
  size_t len = static_cast<size_t>(llen);
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

#if defined(_WIN32) && SIMDJSON_CPLUSPLUS17
inline simdjson_result<padded_string> padded_string::load(std::wstring_view filename) noexcept {
  // Open the file using the wide characters
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = _wfopen(filename.data(), L"rb");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (fp == nullptr) {
    return IO_ERROR;
  }

  // Get the file size
  int ret;
#if SIMDJSON_VISUAL_STUDIO && !SIMDJSON_IS_32BITS
  ret = _fseeki64(fp, 0, SEEK_END);
#else
  ret = std::fseek(fp, 0, SEEK_END);
#endif // _WIN64
  if(ret < 0) {
    std::fclose(fp);
    return IO_ERROR;
  }
#if SIMDJSON_VISUAL_STUDIO && !SIMDJSON_IS_32BITS
  __int64 llen = _ftelli64(fp);
  if(llen == -1L) {
    std::fclose(fp);
    return IO_ERROR;
  }
#else
  long llen = std::ftell(fp);
  if((llen < 0) || (llen == LONG_MAX)) {
    std::fclose(fp);
    return IO_ERROR;
  }
#endif

  // Allocate the padded_string
  size_t len = static_cast<size_t>(llen);
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
#endif

// padded_string_builder implementations

inline padded_string_builder::padded_string_builder() noexcept = default;

inline padded_string_builder::padded_string_builder(size_t new_capacity) noexcept {
  if (new_capacity > 0) {
    data = internal::allocate_padded_buffer(new_capacity);
    if (data != nullptr) {
      this->capacity = new_capacity;
    }
  }
}

inline padded_string_builder::padded_string_builder(padded_string_builder &&o) noexcept
    : size(o.size), capacity(o.capacity), data(o.data) {
  o.size = 0;
  o.capacity = 0;
  o.data = nullptr;
}

inline padded_string_builder &padded_string_builder::operator=(padded_string_builder &&o) noexcept {
  if (this != &o) {
    delete[] data;
    size = o.size;
    capacity = o.capacity;
    data = o.data;
    o.size = 0;
    o.capacity = 0;
    o.data = nullptr;
  }
  return *this;
}

inline padded_string_builder::~padded_string_builder() noexcept {
  delete[] data;
}

inline bool padded_string_builder::append(const char *newdata, size_t length) noexcept {
  if (length == 0) {
    return true;
  }
  if (!reserve(length)) {
    return false;
  }
  std::memcpy(data + size, newdata, length);
  size += length;
  return true;
}

inline bool padded_string_builder::append(std::string_view sv) noexcept {
  return append(sv.data(), sv.size());
}

inline size_t padded_string_builder::length() const noexcept {
  return size;
}

inline padded_string padded_string_builder::build() const noexcept {
  return padded_string(data, size);
}

inline padded_string padded_string_builder::convert() noexcept {
  padded_string result{};
  result.data_ptr = data;
  result.viable_size = size;
  data = nullptr;
  size = 0;
  capacity = 0;
  return result;
}

inline bool padded_string_builder::reserve(size_t additional) noexcept {
  size_t needed = size + additional;
  if (needed <= capacity) {
    return true;
  }
  size_t new_capacity = needed;
  // We are going to grow the capacity exponentially to avoid
  // repeated allocations.
  if (new_capacity < 4096) {
    new_capacity *= 2;
  } else {
    new_capacity += new_capacity/2; // grow by 1.5x
  }
  char *new_data = internal::allocate_padded_buffer(new_capacity);
  if (new_data == nullptr) {
    return false; // Allocation failed
  }
  if (size > 0) {
    std::memcpy(new_data, data, size);
  }
  delete[] data;
  data = new_data;
  capacity = new_capacity;
  return true;
}

} // namespace simdjson

inline simdjson::padded_string operator ""_padded(const char *str, size_t len) {
  return simdjson::padded_string(str, len);
}
#ifdef __cpp_char8_t
inline simdjson::padded_string operator ""_padded(const char8_t *str, size_t len) {
  return simdjson::padded_string(reinterpret_cast<const char *>(str), len);
}
#endif
#endif // SIMDJSON_PADDED_STRING_INL_H
