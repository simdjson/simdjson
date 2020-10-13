#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  simdjson::dom::parser parser;
  simdjson_unused simdjson::dom::element elem;
  simdjson_unused auto error = parser.parse(Data, Size).get(elem);
  return 0;
}
