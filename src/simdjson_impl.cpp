#include "simdjson.h"

namespace simdjson {

simdjson_result<padded_string> padded_string::load(const std::string &filename) noexcept {
  // Open the file
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
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

namespace dom {

simdjson_result<size_t> parser::read_file(const std::string &path) noexcept {
  // Open the file
  std::FILE *fp = std::fopen(path.c_str(), "rb");
  if (fp == nullptr) {
    return IO_ERROR;
  }

  // Get the file size
  if(std::fseek(fp, 0, SEEK_END) < 0) {
    std::fclose(fp);
    return IO_ERROR;
  }
  long len = std::ftell(fp);
  if((len < 0) || (len == LONG_MAX)) {
    std::fclose(fp);
    return IO_ERROR;
  }

  // Make sure we have enough capacity to load the file
  if (_loaded_bytes_capacity < size_t(len)) {
    loaded_bytes.reset( internal::allocate_padded_buffer(len) );
    if (!loaded_bytes) {
      std::fclose(fp);
      return MEMALLOC;
    }
    _loaded_bytes_capacity = len;
  }

  // Read the string
  std::rewind(fp);
  size_t bytes_read = std::fread(loaded_bytes.get(), 1, len, fp);
  if (std::fclose(fp) != 0 || bytes_read != size_t(len)) {
    return IO_ERROR;
  }

  return bytes_read;
}

} // namespace dom
} // namespace simdjson
