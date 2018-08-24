#include "jsonparser/jsonioutil.h"
#include <cstring>
std::pair<u8 *, size_t> get_corpus(std::string filename) {
  std::ifstream is(filename, std::ios::binary);
  if (is) {
    std::stringstream buffer;
    buffer << is.rdbuf();
    size_t length = buffer.str().size(); // +1 for null
    char *aligned_buffer;
    size_t paddedlength = ROUNDUP_N(length, 64);
    if (posix_memalign((void **)&aligned_buffer, 64, paddedlength + 1)) {
      throw std::runtime_error("Could not allocate sufficient memory");
    };
    //memset(aligned_buffer, 0x20, ROUNDUP_N(length + 1, 64));
    memcpy(aligned_buffer, buffer.str().c_str(), length);
    memset(aligned_buffer + length, 0x20, paddedlength - length);
    aligned_buffer[paddedlength] = '\0';
    is.close();
    return std::make_pair((u8 *)aligned_buffer, length);
  }
  throw std::runtime_error("could not load corpus");
  return std::make_pair((u8 *)0, (size_t)0);
}
