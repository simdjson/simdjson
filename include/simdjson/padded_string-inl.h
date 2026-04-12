#ifndef SIMDJSON_PADDED_STRING_INL_H
#define SIMDJSON_PADDED_STRING_INL_H

#include "simdjson/padded_string.h"
#include "simdjson/padded_string_view.h"

#include "simdjson/error-inl.h"
#include "simdjson/padded_string_view-inl.h"

#include <climits>
#include <cwchar>

#if SIMDJSON_HAS_UNISTD_H
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
// On Windows, `padded_memory_map` (when it is enabled) depends on types and
// functions declared in <windows.h>. We deliberately do NOT include that
// header here: users of simdjson who want `padded_memory_map` on Windows
// must include <windows.h> themselves *before* including this header. See
// padded_string.h for the detection logic.

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
  // std::string_view is not guaranteed to be null-terminated, but std::fopen requires
  // a null-terminated C string. Construct a temporary std::string to ensure null-termination.
  const std::string null_terminated_filename(filename);
  // Open the file
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = std::fopen(null_terminated_filename.c_str(), "rb");
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
  // std::wstring_view is not guaranteed to be null-terminated, but _wfopen requires
  // a null-terminated wide C string. Construct a temporary std::wstring to ensure null-termination.
  const std::wstring null_terminated_filename(filename);
  // Open the file using the wide characters
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = _wfopen(null_terminated_filename.c_str(), L"rb");
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
  if (simdjson_unlikely(additional + size < size)) {
    return false; // overflow: cannot satisfy request
  }
  size_t needed = size + additional;
  if (needed <= capacity) {
    return true;
  }
  size_t new_capacity = needed;
  // We are going to grow the capacity exponentially to avoid
  // repeated allocations.
  if (new_capacity < 4096) {
    new_capacity *= 2;
    // overflow guard: ensure new_capacity + new_capacity/2 does not overflow
  } else if (new_capacity + new_capacity / 2 > new_capacity) {
    new_capacity += new_capacity / 2; // grow by 1.5x
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


#if SIMDJSON_HAS_PADDED_MEMORY_MAP

#if SIMDJSON_HAS_UNISTD_H
simdjson_inline padded_memory_map::padded_memory_map(const char *filename) noexcept {

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
      return; // file not found or cannot be opened, data will be nullptr
    }
    struct stat st;
    if (fstat(fd, &st) == -1) {
      close(fd);
      return; // failed to get file size, data will be nullptr
    }
    size = static_cast<size_t>(st.st_size);
    size_t total_size = size + simdjson::SIMDJSON_PADDING;
    void *anon_map =
        mmap(NULL, total_size, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (anon_map == MAP_FAILED) {
      close(fd);
      return; // failed to create anonymous mapping, data will be nullptr
    }
    void *file_map =
        mmap(anon_map, size, PROT_READ, MAP_SHARED | MAP_FIXED, fd, 0);
    if (file_map == MAP_FAILED) {
      munmap(anon_map, total_size);
      close(fd);
      return; // failed to mmap file, data will be nullptr
    }
    data = static_cast<const char *>(file_map);
    close(fd); // no longer needed after mapping
}

simdjson_inline padded_memory_map::~padded_memory_map() noexcept {
  if (data != nullptr) {
    munmap(const_cast<char *>(data), size + simdjson::SIMDJSON_PADDING);
  }
}
#elif defined(_WIN32)
// Windows zero-copy implementation using placeholder virtual memory.
//
// We use the modern Windows memory APIs (VirtualAlloc2, CreateFileMapping2,
// MapViewOfFile3 — available since Windows 10 1803) to map the file into a
// contiguous virtual address range that includes at least SIMDJSON_PADDING
// zero bytes after the file content, with no data copies.
//
// Strategy:
//   1. If rounding the file size up to the allocation granularity already
//      exceeds file_size + SIMDJSON_PADDING, the OS page zero-fill provides
//      the padding and we use a simple MapViewOfFile3 call.
//   2. Otherwise we reserve a contiguous placeholder region via VirtualAlloc2,
//      split it at the granularity-aligned file boundary, map the file into
//      the first part, and commit zero pages for the second part (padding).
simdjson_inline padded_memory_map::padded_memory_map(const char *filename) noexcept {
  HANDLE file_handle = ::CreateFileA(
      filename, GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file_handle == INVALID_HANDLE_VALUE) {
    return;
  }
  LARGE_INTEGER file_size_li;
  if (!::GetFileSizeEx(file_handle, &file_size_li) || file_size_li.QuadPart < 0) {
    ::CloseHandle(file_handle);
    return;
  }
#if SIMDJSON_IS_32BITS
  if (static_cast<unsigned long long>(file_size_li.QuadPart) >
      static_cast<unsigned long long>(SIZE_MAX - simdjson::SIMDJSON_PADDING)) {
    ::CloseHandle(file_handle);
    return;
  }
#endif
  size = static_cast<size_t>(file_size_li.QuadPart);
  if (size == 0) {
    ::CloseHandle(file_handle);
    return;
  }

  HANDLE section = ::CreateFileMapping2(
      file_handle, NULL, FILE_MAP_READ, PAGE_READONLY,
      0, 0, NULL, NULL, 0);
  ::CloseHandle(file_handle);
  if (section == NULL) {
    return;
  }

  SYSTEM_INFO si;
  ::GetSystemInfo(&si);
  const size_t granularity = static_cast<size_t>(si.dwAllocationGranularity);
  const size_t file_region = (size + granularity - 1) & ~(granularity - 1);
  const size_t total_needed = size + simdjson::SIMDJSON_PADDING;

  if (file_region >= total_needed) {
    // The zero-fill in the last page already covers the padding.
    PVOID view = ::MapViewOfFile3(
        section, ::GetCurrentProcess(), NULL, 0, 0,
        0, PAGE_READONLY, NULL, 0);
    ::CloseHandle(section);
    if (view != NULL) {
      data = static_cast<const char *>(view);
    }
    return;
  }

  // We need extra zero pages beyond the file region. Use the placeholder API
  // to get a contiguous virtual address range spanning both the file mapping
  // and the zero-filled padding.
  const size_t padding_region =
      ((total_needed - file_region) + granularity - 1) & ~(granularity - 1);
  const size_t reserve_size = file_region + padding_region;

  // Reserve a contiguous placeholder.
  PVOID placeholder = ::VirtualAlloc2(
      ::GetCurrentProcess(), NULL, reserve_size,
      MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
  if (placeholder == NULL) {
    ::CloseHandle(section);
    return;
  }

  // Split into two placeholders at the file_region boundary.
  if (!::VirtualFree(placeholder, file_region,
                     MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
    ::VirtualFree(placeholder, 0, MEM_RELEASE);
    ::CloseHandle(section);
    return;
  }

  // Map the file into the first placeholder.
  PVOID file_view = ::MapViewOfFile3(
      section, ::GetCurrentProcess(), placeholder, 0, file_region,
      MEM_REPLACE_PLACEHOLDER, PAGE_READONLY, NULL, 0);
  ::CloseHandle(section);
  if (file_view == NULL) {
    ::VirtualFree(placeholder, 0, MEM_RELEASE);
    ::VirtualFree(static_cast<char *>(placeholder) + file_region,
                  0, MEM_RELEASE);
    return;
  }

  // Commit zero pages in the second placeholder (the padding).
  void *pad = static_cast<char *>(placeholder) + file_region;
  PVOID padding_ptr = ::VirtualAlloc2(
      ::GetCurrentProcess(), pad, padding_region,
      MEM_REPLACE_PLACEHOLDER | MEM_COMMIT, PAGE_READONLY, NULL, 0);
  if (padding_ptr == NULL) {
    ::UnmapViewOfFile(file_view);
    ::VirtualFree(pad, 0, MEM_RELEASE);
    return;
  }

  data = static_cast<const char *>(file_view);
  padding_view_ = padding_ptr;
}

simdjson_inline padded_memory_map::~padded_memory_map() noexcept {
  if (data == nullptr) { return; }
  ::UnmapViewOfFile(data);
  if (padding_view_ != nullptr) {
    ::VirtualFree(padding_view_, 0, MEM_RELEASE);
  }
}
#endif // POSIX or _WIN32

simdjson_inline simdjson::padded_string_view padded_memory_map::view() const noexcept simdjson_lifetime_bound {
  if(!is_valid()) {
    return simdjson::padded_string_view(); // return an empty view if mapping failed
  }
  return simdjson::padded_string_view(data, size, size + simdjson::SIMDJSON_PADDING);
}

simdjson_inline bool padded_memory_map::is_valid() const noexcept {
  return data != nullptr;
}

#endif // SIMDJSON_HAS_PADDED_MEMORY_MAP

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
