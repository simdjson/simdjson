#include "FuzzUtils.h"
#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  FuzzData fd(Data, Size);
  const int action = fd.getInt<0, 12>();

  // split the remainder of the document into strings
  auto strings = fd.splitIntoStrings();
  while (strings.size() < 1) {
    strings.emplace_back();
  }
#if SIMDJSON_EXCEPTIONS
  try {
#endif
    simdjson::ondemand::parser parser;
    simdjson::padded_string padded(strings[0]);
    auto doc = parser.iterate(padded);
    if (doc.error()) {
      return 0;
    }
    for (auto item : doc) {
      switch (action) {
      case 0: {
        simdjson_unused auto x = item.get_string();
      } break;
      case 1: {
        simdjson_unused auto x = item.get_bool();
      } break;
      case 2: {
        simdjson_unused auto x = item.get_array();
      } break;
      case 3: {
        simdjson_unused auto x = item.get_int64();
      } break;
      case 4: {
        simdjson_unused auto x = item.get_double();
      } break;
      case 5: {
        simdjson_unused auto x = item.get_object();
      } break;
      case 6: {
        simdjson_unused auto x = item.get_uint64();
      } break;
      case 7: {
        simdjson_unused auto x = item.get_raw_json_string();
      } break;
      case 8: {
        simdjson_unused auto x = item.is_null();
      } break;
      case 9: {
        simdjson_unused auto x = item.begin();
      } break;
      case 10: {
        simdjson_unused auto x = item.end();
      } break;
      case 11: {
        for (auto e : item) {
          simdjson_unused auto x = e.is_null();
        }
      } break;
      default:;
      }
    }
#if SIMDJSON_EXCEPTIONS
  } catch (...) {
  }
#endif

  return 0;
}
