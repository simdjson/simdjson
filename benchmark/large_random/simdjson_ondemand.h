#pragma once

#if SIMDJSON_EXCEPTIONS

#include "large_random.h"

namespace large_random {

using namespace simdjson;

struct simdjson_ondemand {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto doc = parser.iterate(json);
    for (ondemand::object coord : doc) {
      result.emplace_back(json_benchmark::point{coord.find_field("x"), coord.find_field("y"), coord.find_field("z")});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, simdjson_ondemand)->UseManualTime();

// Same workload, but parsed with iterate_unpadded (the bounds-safe value parsers
// used for input buffers without SIMDJSON_PADDING). Run on the same buffer so the
// measured difference is purely the cost of the unpadded code paths.
struct simdjson_ondemand_unpadded {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto doc = parser.iterate_unpadded(json.data(), json.size());
    for (ondemand::object coord : doc) {
      result.emplace_back(json_benchmark::point{coord.find_field("x"), coord.find_field("y"), coord.find_field("z")});
    }
    return true;
  }
};
BENCHMARK_TEMPLATE(large_random, simdjson_ondemand_unpadded)->UseManualTime();
#if SIMDJSON_STATIC_REFLECTION

struct simdjson_ondemand_static_reflect {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto doc = parser.iterate(json);
    if(auto e = doc.get_array().get<std::vector<point>>(result); e) { return false; }
    // We can also do it like so:
    //for (ondemand::object coord : doc) {
    //  result.emplace_back(coord.get<point>());
    //}
    // It seems that doing the reflection is slower than doing the manual lookup.
    // E.g., it is faster if we do result.emplace_back(coord["x"], coord["y"], coord["z"]);
    return true;
  }
};
BENCHMARK_TEMPLATE(large_random, simdjson_ondemand_static_reflect)->UseManualTime();


#endif
} // namespace large_random

#endif // SIMDJSON_EXCEPTIONS
