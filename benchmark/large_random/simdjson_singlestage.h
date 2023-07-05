#pragma once

#if SIMDJSON_EXCEPTIONS

#include "large_random.h"

namespace large_random {

using namespace simdjson;

struct simdjson_singlestage {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  singlestage::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    auto doc = parser.iterate(json);
    for (singlestage::object coord : doc) {
      result.emplace_back(json_benchmark::point{coord.find_field("x"), coord.find_field("y"), coord.find_field("z")});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, simdjson_singlestage)->UseManualTime();

} // namespace large_random

#endif // SIMDJSON_EXCEPTIONS
