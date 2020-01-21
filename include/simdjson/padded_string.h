#ifndef SIMDJSON_PADDING_STRING_H
#define SIMDJSON_PADDING_STRING_H
#include "simdjson/portability.h"
#include <cstring>
#include <memory>
#include <string>

// required by GNUC < 8
#include <fstream>

namespace simdjson {
// low-level function to allocate memory with padding so we can read passed the
// "length" bytes safely. if you must provide a pointer to some data, create it
// with this function: length is the max. size in bytes of the string caller is
// responsible to free the memory (free(...))
inline char *allocate_padded_buffer(size_t length) noexcept {
  // we could do a simple malloc
  // return (char *) malloc(length + SIMDJSON_PADDING);
  // However, we might as well align to cache lines...
  size_t totalpaddedlength = length + SIMDJSON_PADDING;
  char *padded_buffer = aligned_malloc_char(64, totalpaddedlength);
#ifndef NDEBUG
  if (padded_buffer == nullptr) {
    errno = EINVAL;
    perror("simdjson::allocate_padded_buffer() aligned_malloc_char() failed");
    return nullptr;
  }
#endif // NDEBUG
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
  }

  explicit padded_string(const char *data, size_t length) noexcept
      : viable_size(length), data_ptr(allocate_padded_buffer(length)) {
    if ((data != nullptr) and (data_ptr != nullptr)) {
      memcpy(data_ptr, data, length);
      data_ptr[length] = '\0'; // easier when you need a c_str
    }
  }
  // free the old payload
  // allocate for the new size
  // return false on ENOMEM
  bool reset(size_t new_size_) noexcept {
    this->~padded_string();
    viable_size = new_size_;
    data_ptr = allocate_padded_buffer(new_size_);
    if (data_ptr != nullptr) {
      data_ptr[new_size_] = '\0'; // easier when you need a c_str
      return true;
    }
    // failed to allocate
    viable_size = 0;
    return false;
  }

  // reset the ps_
  // return the result of fread()
  // return 0 on error
  static errno_t load(padded_string &ps_, FILE *fp) noexcept {

    if (std::fseek(fp, 0, SEEK_END) < 0) {
      std::fclose(fp);
      return errno;
    }
    long llen = std::ftell(fp);
    if ((llen < 0) || (llen == LONG_MAX)) {
      std::fclose(fp);
      return errno;
    }

    // now make the temporary p string
    size_t length_ = (size_t)llen;

    if (!ps_.reset(length_))
      return ENOMEM;

    std::rewind(fp);
    size_t readb = std::fread(ps_.data(), 1, length_, fp);
    std::fclose(fp);

    if (readb != length_) {
      // free mem before bad return
      ps_.~padded_string();
      return errno;
    }
    return 0;
  }

  // note: do pass std::string_view arguments by value
  padded_string(std::string_view sv_) noexcept
      : padded_string(sv_.data(), sv_.size()) {}

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

  /*
  Makes padded_string 'Swappable'. This implementation is also ADL friendly.

  using namespace simjson ;
  padded_string a("A");
  padded_string b("B");
    swap(a,b) ;
      printf("%s", a.data() ); // "B"
  */
  friend void swap(padded_string &left_, padded_string &right_) {

    using std::swap; // bring in swap for built-in types

    swap(left_.viable_size, right_.viable_size);
    swap(left_.data_ptr, right_.data_ptr);
  }

  /*
  padded_string a("A");
  padded_string b("B");
       a.swap(b) ;
         printf("%s", a.data() ); // "B"
  */
  void swap(padded_string &right_) {
    auto &left_ = *this;
    std::swap(left_.viable_size, right_.viable_size);
    std::swap(left_.data_ptr, right_.data_ptr);
  }

  /*
     padded_string a("A");
     a.~padded_string() ;
    */
  ~padded_string() {
    aligned_free_char(data_ptr);
    viable_size = size_t(0);
  }

  size_t size() const { return viable_size; }

  size_t length() const { return viable_size; }

  char *data() const { return data_ptr; }

private:
  padded_string &operator=(const padded_string &o) = delete;
  padded_string(const padded_string &o) = delete;

  size_t viable_size{};
  char *data_ptr{nullptr};

}; // padded_string

} // namespace simdjson

#endif
