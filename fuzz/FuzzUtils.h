#ifndef SIMDJSON_FUZZUTILS_H
#define SIMDJSON_FUZZUTILS_H

#include <cstdint>

// view data as a byte pointer
template <typename T> inline const std::uint8_t* as_bytes(const T* data) {
  return static_cast<const std::uint8_t*>(static_cast<const void*>(data));
}

// view data as a char pointer
template <typename T> inline const char* as_chars(const T* data) {
  return static_cast<const char*>(static_cast<const void*>(data));
}


#endif // SIMDJSON_FUZZUTILS_H
