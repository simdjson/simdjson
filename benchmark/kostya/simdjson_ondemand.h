#pragma once

#if SIMDJSON_EXCEPTIONS

#include "kostya.h"

namespace kostya {

using namespace simdjson;

struct simdjson_ondemand {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto doc = parser.iterate(json);
    for (ondemand::object point : doc.find_field("coordinates")) {
      result.emplace_back(json_benchmark::point{point.find_field("x"), point.find_field("y"), point.find_field("z")});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(kostya, simdjson_ondemand)->UseManualTime();

} // namespace kostya

#endif // SIMDJSON_EXCEPTIONS
