#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "NullBuffer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

  try {
    auto pj = simdjson::build_parsed_json(Data, Size);
    NulOStream os;
    UNUSED bool ignored=pj.dump_raw_tape(os);
  } catch (...) {
  }
  return 0;
}
