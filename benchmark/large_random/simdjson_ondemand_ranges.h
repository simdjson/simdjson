#pragma once

#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_RANGES

#include "large_random.h"

namespace large_random {

using namespace simdjson;

// Identical to simdjson_ondemand but uses get_range() for iteration.
// Demonstrates that the ranges wrapper has zero per-element overhead.
struct simdjson_ondemand_ranges {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto doc = parser.iterate(json);
    for (auto coord_result : ondemand::get_range(doc.get_array())) {
      ondemand::object coord = coord_result;
      result.emplace_back(json_benchmark::point{coord.find_field("x"), coord.find_field("y"), coord.find_field("z")});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, simdjson_ondemand_ranges)->UseManualTime();

} // namespace large_random

#endif // SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_RANGES
