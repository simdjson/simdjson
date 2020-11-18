#include "FuzzUtils.h"
#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  FuzzData fd(Data,Size);

  const auto s=fd.get<size_t>();
  simdjson_unused simdjson::padded_string p(s);
  return  0;
}
