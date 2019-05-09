#ifndef SIMDJSON_PADDING_STRING_H
#define SIMDJSON_PADDING_STRING_H
#include "simdjson/portability.h"
#include <memory>
#include <cstring>
// low-level function to allocate memory with padding so we can read passed the
// "length" bytes safely. if you must provide a pointer to some data, create it
// with this function: length is the max. size in bytes of the string caller is
// responsible to free the memory (free(...))
char *allocate_padded_buffer(size_t length);

// Simple string with padded allocation.
// We deliberately forbid copies, users should rely on swap or move
// constructors.
class padded_string {
public:
  explicit padded_string() noexcept : viable_size(0), data_ptr(nullptr) {}
  explicit padded_string(size_t length) noexcept
      : viable_size(length), data_ptr(allocate_padded_buffer(length)) {

    if (data_ptr != nullptr)
      data_ptr[length] = '\0'; // easier when you need a c_str
  }
  explicit padded_string(char *data, size_t length) noexcept
      : viable_size(length), data_ptr(allocate_padded_buffer(length)) {
    if (data_ptr != nullptr) {
      memcpy(data_ptr, data, length);
      data_ptr[length] = '\0'; // easier when you need a c_str
    }
  }
  padded_string(std::string s) noexcept
      : viable_size(s.size()), data_ptr(allocate_padded_buffer(s.size())) {
    if (data_ptr != nullptr) {
      memcpy(data_ptr, s.data(), s.size());
      data_ptr[s.size()] = '\0'; // easier when you need a c_str
    }
  }
  padded_string(padded_string &&o) noexcept
      : viable_size(o.viable_size), data_ptr(o.data_ptr) {
    o.data_ptr = nullptr; // we take ownership
  }
  void swap(padded_string &o) {
    size_t tmp_viable_size = viable_size;
    char *tmp_data_ptr = data_ptr;
    viable_size = o.viable_size;
    data_ptr = o.data_ptr;
    o.data_ptr = tmp_data_ptr;
    o.viable_size = tmp_viable_size;
  }

  ~padded_string() { aligned_free_char(data_ptr); }

  size_t size() const { return viable_size; }

  size_t length() const { return viable_size; }

  char *data() const { return data_ptr; }

private:
  padded_string &operator=(const padded_string &o) = delete;
  padded_string(const padded_string &o) = delete;

  size_t viable_size;
  char *data_ptr;
};

#endif
