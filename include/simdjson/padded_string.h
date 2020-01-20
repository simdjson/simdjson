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

  explicit padded_string(const char *data, size_t length) noexcept
      : viable_size(length), data_ptr(allocate_padded_buffer(length)) {
    if ((data != nullptr) and (data_ptr != nullptr)) {
      memcpy(data_ptr, data, length);
      data_ptr[length] = '\0'; // easier when you need a c_str
    }
  }

  // note: do pass std::string_view arguments by value
  padded_string(std::string_view sv_) noexcept
      : padded_string( sv_.data(), sv_.size() ) {
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

  /*
       padded_string ps_ = padded_string::load("big.json");
  */
  static padded_string load (std::string_view filename) {
    using namespace std;

    ifstream ifs_(filename.data());
    string str_(istreambuf_iterator<char>{ifs_}, {});

    return padded_string{str_};
  }
  
  /*
  Makes padded_string 'Swappable'. This implementation is also ADL friendly.

  using namespace simjson ;
  padded_string a("A");
  padded_string b("B");
    swap(a,b) ;
      printf("%s", a.data() ); // "B"
  */
  friend void swap(padded_string & left_ , padded_string & right_ ) {

    using std::swap; // bring in swap for built-in types

    swap( left_.viable_size , right_.viable_size) ;
    swap(left_.data_ptr, right_.data_ptr) ;
  }

  /*
  padded_string a("A");
  padded_string b("B");
       a.swap(b) ;
         printf("%s", a.data() ); // "B"
  */
    void swap(padded_string &right_) {
    auto & left_ = *this;
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

  size_t size() const  { return viable_size; }

  size_t length() const  { return viable_size; }

  char *data() const  { return data_ptr; }

private:
  padded_string &operator=(const padded_string &o) = delete;
  padded_string(const padded_string &o) = delete;

  size_t viable_size{};
  char *data_ptr{nullptr};

}; // padded_string

} // namespace simdjson

#endif
