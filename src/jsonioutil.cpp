#include "simdjson/jsonioutil.h"
#include <cstring>
#include <cstdlib>

char * allocate_padded_buffer(size_t length) {
    // we could do a simple malloc
    //return (char *) malloc(length + SIMDJSON_PADDING);
    // However, we might as well align to cache lines...
    size_t totalpaddedlength = length + SIMDJSON_PADDING;
    char *padded_buffer = aligned_malloc_char(64, totalpaddedlength);
    return padded_buffer;
}

padded_string get_corpus(const std::string& filename) {
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  if (fp != nullptr) {
    std::fseek(fp, 0, SEEK_END);
    size_t len = std::ftell(fp);
    padded_string s(len);
    if(s.data() == nullptr) {
      std::fclose(fp);
      throw  std::runtime_error("could not allocate memory");
    }
    std::rewind(fp);
    size_t readb = std::fread(s.data(), 1, len, fp);
    std::fclose(fp);
    if(readb != len) {
      throw  std::runtime_error("could not read the data");
    }
    return s;
  }
  throw  std::runtime_error("could not load corpus");
}
