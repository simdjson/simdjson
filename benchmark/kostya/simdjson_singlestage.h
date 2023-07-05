#pragma once

#if SIMDJSON_EXCEPTIONS

#include "kostya.h"

namespace kostya {

using namespace simdjson;

struct simdjson_singlestage {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  singlestage::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto doc = parser.iterate(json);
    for (singlestage::object point : doc.find_field("coordinates")) {
      result.emplace_back(json_benchmark::point{point.find_field("x"), point.find_field("y"), point.find_field("z")});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(kostya, simdjson_singlestage)->UseManualTime();

} // namespace kostya

#endif // SIMDJSON_EXCEPTIONS
