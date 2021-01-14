#pragma once

#if SIMDJSON_EXCEPTIONS

#include "large_random.h"

namespace large_random {

using namespace simdjson;

struct simdjson_ondemand_unordered {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto doc = parser.iterate(json);
    for (ondemand::object coord : doc) {
      result.emplace_back(json_benchmark::point{coord["x"], coord["y"], coord["z"]});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, simdjson_ondemand_unordered)->UseManualTime();

} // namespace large_random

#endif // SIMDJSON_EXCEPTIONS
