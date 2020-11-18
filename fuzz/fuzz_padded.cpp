#include "FuzzUtils.h"
#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  FuzzData fd(Data, Size);

  const int action = fd.getInt<0, 10>();

  const auto s1 = fd.get<size_t>();
  const auto s2 = fd.get<size_t>();
  switch (action) {
  case 0: {
    simdjson_unused simdjson::padded_string p(s1);
  } break;
  case 1: {
    // operator== with temp value
    simdjson_unused simdjson::padded_string p1(s1);
    simdjson_unused simdjson::padded_string p2(s2);
    p1 = std::move(p2);
  } break;
  case 2: {
    // swap
    simdjson_unused simdjson::padded_string p1(s1);
    simdjson_unused simdjson::padded_string p2(s2);
    p1.swap(p2);
  } break;
  case 3: {
    simdjson_unused simdjson::padded_string p(s1);
    p.data();
  } break;
  case 4: {
    simdjson::padded_string p(s1);
    simdjson_unused auto sv = static_cast<std::string_view>(p);
  } break;
  case 5: {
    // load from file.
    const std::string filename = "/dev/shm/fuzz_padded.tmp";
    {
      std::ofstream file(filename);
      assert(file);
      const long ssize = static_cast<long>(fd.Size);
      file.write(fd.chardata(), ssize);
      assert(file.tellp() == ssize);
    }
    simdjson_unused auto data = simdjson::padded_string::load(filename);
  } break;
  default:;
  }
  return 0;
}
