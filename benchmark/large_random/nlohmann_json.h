#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "large_random.h"

namespace large_random {

struct nlohmann_json {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    for (auto point : nlohmann::json::parse(json.data(), json.data() + json.size())) {
      result.emplace_back(json_benchmark::point{point["x"], point["y"], point["z"]});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, nlohmann_json)->UseManualTime();

} // namespace large_random

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON
