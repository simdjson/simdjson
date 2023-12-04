#pragma once

#ifdef SIMDJSON_COMPETITION_BOOSTJSON

#include "kostya.h"

namespace kostya {

struct boostjson {
  static constexpr diff_flags DiffFlags = diff_flags::IMPRECISE_FLOATS;

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto root = boost::json::parse(json);
    for (const auto &point : root.at("coordinates").as_array()) {
      result.emplace_back(json_benchmark::point{
        point.at("x").to_number<double>(),
        point.at("y").to_number<double>(),
        point.at("z").to_number<double>()
      });
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(kostya, boostjson)->UseManualTime();

} // namespace kostya

#endif // SIMDJSON_COMPETITION_BOOSTJSON
