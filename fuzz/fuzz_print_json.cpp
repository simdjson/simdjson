#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

#include "NullBuffer.h"


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

  try {
    auto pj = simdjson::build_parsed_json(Data, Size);
    NulOStream os;
    bool ignored=pj.print_json(os);
    (void)ignored;
  } catch (...) {
  }
  return 0;
}
