#pragma once

#if SIMDJSON_EXCEPTIONS

#include "large_random.h"

namespace large_random {

using namespace simdjson;
using namespace simdjson::builtin;

class simdjson_ondemand {
  ondemand::parser parser{};
public:
  bool run(const simdjson::padded_string &json, std::vector<point> &points) {
    auto doc = parser.iterate(json);
    for (ondemand::object coord : doc) {
      points.emplace_back(point{coord.find_field("x"), coord.find_field("y"), coord.find_field("z")});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, simdjson_ondemand);

} // namespace large_random

#endif // SIMDJSON_EXCEPTIONS
