#pragma once

#if SIMDJSON_EXCEPTIONS

#include "large_random.h"

namespace large_random {

using namespace simdjson;
using namespace simdjson::builtin;

struct simdjson_ondemand_unordered {
  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto doc = parser.iterate(json);
    for (ondemand::object coord : doc) {
      result.emplace_back(large_random::point{coord["x"], coord["y"], coord["z"]});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, simdjson_ondemand_unordered)->UseManualTime();

} // namespace large_random

#endif // SIMDJSON_EXCEPTIONS
