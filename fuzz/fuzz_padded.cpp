#include "FuzzUtils.h"
#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  FuzzData fd(Data,Size);

  const int action=fd.getInt<0,10>();

  const auto s1=fd.get<size_t>();
  const auto s2=fd.get<size_t>();
  switch(action) {
  case 0: {
    simdjson_unused simdjson::padded_string p(s1);
  }break;
  case 1: {
    // operator== with temp value
    simdjson_unused simdjson::padded_string p1(s1);
    simdjson_unused simdjson::padded_string p2(s2);
    p1=std::move(p2);
  }break;
  default:
    ;
  }
  return  0;
}
