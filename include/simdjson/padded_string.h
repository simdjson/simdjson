#ifndef SIMDJSON_PADDING_STRING_H
#define SIMDJSON_PADDING_STRING_H
#include "simdjson/portability.h"
#include <cstring>
#include <memory>
#include <string>

namespace simdjson {
// low-level function to allocate memory with padding so we can read passed the
// "length" bytes safely. if you must provide a pointer to some data, create it
// with this function: length is the max. size in bytes of the string caller is
// responsible to free the memory (free(...))
inline char *allocate_padded_buffer(size_t length) noexcept {

#ifndef NDEBUG
  if (length < 1) {
    errno = EINVAL;
    perror("simdjson::allocate_padded_buffer() length argument is less than 1");
    return nullptr;
  }
#endif // NDEBUG

  // we could do a simple malloc
  // return (char *) malloc(length + SIMDJSON_PADDING);
  // However, we might as well align to cache lines...
  size_t totalpaddedlength = length + SIMDJSON_PADDING;
  char *padded_buffer = aligned_malloc_char(64, totalpaddedlength);
  memset(padded_buffer + length, 0, totalpaddedlength - length);
  return padded_buffer;
} // allocate_padded_buffer

// Simple string with padded allocation.
// We deliberately forbid copies, users should rely on swap or move
// constructors.
struct padded_string final {

  explicit padded_string() noexcept : viable_size(0), data_ptr(nullptr) {}

  explicit padded_string(size_t length) noexcept
      : viable_size(length), data_ptr(allocate_padded_buffer(length)) {

    if (data_ptr != nullptr)
      data_ptr[length] = '\0'; // easier when you need a c_str
#ifndef NDEBUG
    else {
      errno = EINVAL;
      perror("simdjson::padded_string() length argument is less than 1");
      // what to do here? exit() ?
    }
#endif
  }

  explicit padded_string(char *data, size_t length) noexcept
      : viable_size(length), data_ptr(allocate_padded_buffer(length)) {
    if (data_ptr != nullptr) {
      memcpy(data_ptr, data, length);
      data_ptr[length] = '\0'; // easier when you need a c_str
    }
#ifndef NDEBUG
    else {
      errno = EINVAL;
      perror("simdjson::padded_string() data argument is null");
      // what to do here? exit() ?
    }
#endif
  }

  // note: do not pass std::string arguments by value
  padded_string(const std::string & str_ ) noexcept
      : viable_size(str_.size()), data_ptr(allocate_padded_buffer(str_.size())) {
    if (data_ptr != nullptr) {
      memcpy(data_ptr, str_.data(), str_.size());
      data_ptr[str_.size()] = '\0'; // easier when you need a c_str
    }
#ifndef NDEBUG
    else {
      errno = EINVAL;
      perror("simdjson::padded_string() std::string argument is empty");
      // what to do here? exit() ?
    }
#endif
  }

  padded_string(std::string_view sv_) noexcept
      : viable_size(sv_.size()), data_ptr(allocate_padded_buffer(sv_.size())) {
    if (data_ptr != nullptr) {
      memcpy(data_ptr, sv_.data(), sv_.size());
      data_ptr[sv_.size()] = '\0'; // easier when you need a c_str
    }
#ifndef NDEBUG
    else {
      errno = EINVAL;
      perror("simdjson::padded_string() std::string_view argument is empty");
      // what to do here? exit() ?
    }
#endif
  }

  padded_string(padded_string &&o) noexcept
      : viable_size(o.viable_size), data_ptr(o.data_ptr) {
    o.data_ptr = nullptr; // we take ownership
  }

  padded_string &operator=(padded_string &&o) {
    data_ptr = o.data_ptr;
    viable_size = o.viable_size;
    o.data_ptr = nullptr; // we take ownership
    o.viable_size = 0;
    return *this;
  }

  void swap(padded_string &o) {
    size_t tmp_viable_size = viable_size;
    char *tmp_data_ptr = data_ptr;
    viable_size = o.viable_size;
    data_ptr = o.data_ptr;
    o.data_ptr = tmp_data_ptr;
    o.viable_size = tmp_viable_size;
  }

  ~padded_string() {
      aligned_free_char(data_ptr);
      this->data_ptr = nullptr;
  }

  size_t size() const  { return viable_size; }

  size_t length() const  { return viable_size; }

  char *data() const  { return data_ptr; }

private:
  padded_string &operator=(const padded_string &o) = delete;
  padded_string(const padded_string &o) = delete;

  size_t viable_size ;
  char *data_ptr{nullptr};

}; // padded_string

} // namespace simdjson

#endif
