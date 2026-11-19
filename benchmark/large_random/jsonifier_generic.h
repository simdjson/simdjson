#pragma once

#ifdef SIMDJSON_COMPETITION_JSONIFIER

#include "large_random.h"

namespace large_random {

namespace jsonifier_api = ::jsonifier::generic;

struct jsonifier_generic {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  ::jsonifier::jsonifier_core<> parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    jsonifier_api::document doc = parser.iterate(json);
    jsonifier_api::array coordinates = doc.getArray();
    if (coordinates.error() != jsonifier_api::error_code::success) { return false; }
    for (jsonifier_api::value element : coordinates) {
      jsonifier_api::object coordinate = element.getObject();
      double x{}, y{}, z{};
      if (coordinate.findField("x").getDouble(x) != jsonifier_api::error_code::success) { return false; }
      if (coordinate.findField("y").getDouble(y) != jsonifier_api::error_code::success) { return false; }
      if (coordinate.findField("z").getDouble(z) != jsonifier_api::error_code::success) { return false; }
      result.emplace_back(json_benchmark::point{x, y, z});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, jsonifier_generic)->UseManualTime();

} // namespace large_random

#endif // SIMDJSON_COMPETITION_JSONIFIER
