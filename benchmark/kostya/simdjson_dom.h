#pragma once

#if SIMDJSON_EXCEPTIONS

#include "kostya.h"

namespace kostya {

using namespace simdjson;

struct simdjson_dom {
  dom::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    for (auto point : parser.parse(json)["coordinates"]) {
      result.emplace_back(kostya::point{point["x"], point["y"], point["z"]});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(kostya, simdjson_dom)->UseManualTime();

} // namespace kostya

#endif // SIMDJSON_EXCEPTIONS