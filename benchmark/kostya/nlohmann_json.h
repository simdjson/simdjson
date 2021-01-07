#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "kostya.h"

namespace kostya {

struct nlohmann_json {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto root = nlohmann::json::parse(json.data(), json.data() + json.size());
    for (auto point : root["coordinates"]) {
      result.emplace_back(json_benchmark::point{point["x"], point["y"], point["z"]});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(kostya, nlohmann_json)->UseManualTime();

} // namespace kostya

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON
