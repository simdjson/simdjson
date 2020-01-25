#ifndef SIMDJSON_PADDING_STRING_H
#define SIMDJSON_PADDING_STRING_H
#include "simdjson/portability.h"
#include "common_defs.h" // SIMDJSON_PADDING
#include <cstring>
#include <memory>
#include <string>

#ifndef LONG_MAX
#include <limits.h>
#endif // LONG_MAX

// https://stackoverflow.com/a/24207339
#ifndef _MSC_VER
typedef int errno_t ;
#endif // _MSC_VER

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

    using type = padded_string;
    using value_type = char *;


  explicit padded_string() noexcept : viable_size(0), data_ptr(nullptr) {}

  explicit padded_string(size_t length) noexcept 
      : data_ptr(allocate_padded_buffer(length)), viable_size(length)
  {
    #ifndef NDEBUG
    if (data_ptr != nullptr) {
      /* do what? */
    }
    #endif
  }

  explicit padded_string(const char *data, size_t length) noexcept
      : padded_string(length) 
  {
    if ((data != nullptr) and (data_ptr != nullptr)) {
      memcpy(data_ptr, data, length);
      // probably not necessary
      data_ptr[length] = '\0'; 
    } else {
      /* do what? */
    }
  }

    // note: do pass std::string_view arguments by value
  padded_string(std::string_view sv_) noexcept
      : padded_string(sv_.data(), sv_.size()) {}

  // move ctor
  padded_string(padded_string &&other_) noexcept {
    // calling move assignment
    *this = std::move(other_);
  }

  // move assignment
  padded_string &operator=(padded_string && other_ ) {
    using namespace std;
    std::swap(data_ptr, other_.data_ptr);
    std::swap(viable_size, other_.viable_size);
    // free the payload
    // that other_.data_ptr is pointing to
    simdjson::aligned_free_char(other_.data_ptr);
    // technicaly not needed
    // other_ is moved after this operator exits
    // and it not to be used any more
    other_.data_ptr = nullptr;
    other_.viable_size = 0;
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
  void swap( padded_string &right_) {
    std::swap(*this, right_);
  }

  /*
     padded_string a("A");
     a.~padded_string() ;
    */
  ~padded_string() {
      // in case something calls 
      // the dtor excplicitly
    if (data_ptr != nullptr ) {
      aligned_free_char(data_ptr);
      data_ptr = nullptr;
      viable_size = size_t(0);
    }
  }

  size_t size() const { return viable_size; }

  size_t length() const { return viable_size; }

  char *data() { return data_ptr; }
  const char *data() const { return data_ptr; }

private:
    // no copying
  padded_string &operator=(const padded_string &o) = delete;
  padded_string(const padded_string &o) = delete;

  size_t viable_size{};
  char *data_ptr{nullptr};

}; // padded_string

} // namespace simdjson

#endif
