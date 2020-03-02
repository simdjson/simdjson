#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

  auto begin = (const char *)Data;
  auto end = begin + Size;

  std::string str(begin, end);

  simdjson::json_minify(str.data(), str.size(), str.data());
  return 0;
}
