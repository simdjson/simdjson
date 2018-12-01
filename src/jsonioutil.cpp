#include "simdjson/jsonioutil.h"
#include <cstring>


char * allocate_aligned_buffer(size_t length) {
    char *aligned_buffer;
    size_t paddedlength = ROUNDUP_N(length, 64);
    // allocate an extra sizeof(__m256i) just so we can always use AVX safely
    size_t totalpaddedlength = paddedlength + 1 + sizeof(__m256i);
    if (posix_memalign((void **)&aligned_buffer, 64, totalpaddedlength)) {
      throw std::runtime_error("Could not allocate sufficient memory");
    };
    return aligned_buffer;
}

std::string_view get_corpus(std::string filename) {
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  if (fp) {
    std::fseek(fp, 0, SEEK_END);
    size_t len = std::ftell(fp);
    char * buf = allocate_aligned_buffer(len);
    if(buf == NULL) {
      std::fclose(fp);
      throw  std::runtime_error("could not allocate memory");
    }
    std::rewind(fp);
    std::fread(buf, 1, len, fp);
    buf[len] = '\0';
    std::fclose(fp);
    return std::string_view(buf,len);
  }
  throw  std::runtime_error("could not load corpus");
}
