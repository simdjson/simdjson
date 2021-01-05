#pragma once

#if SIMDJSON_EXCEPTIONS

#include "large_random.h"

namespace large_random {

using namespace simdjson;

struct simdjson_dom {
  dom::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    for (auto point : parser.parse(json)) {
      result.emplace_back(large_random::point{point["x"], point["y"], point["z"]});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, simdjson_dom)->UseManualTime();

} // namespace large_random

#endif // SIMDJSON_EXCEPTIONS