#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  simdjson::dom::parser parser;
  UNUSED simdjson::error_code error;
  UNUSED simdjson::dom::element elem;
  parser.parse(Data, Size).tie(elem, error);
  return 0;
}
