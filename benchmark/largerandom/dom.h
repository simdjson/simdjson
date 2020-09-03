#pragma once

#if SIMDJSON_EXCEPTIONS

#include "largerandom.h"

namespace largerandom {

using namespace simdjson;

class Dom {
public:
  simdjson_really_inline bool Run(const padded_string &json);

  simdjson_really_inline const std::vector<my_point> &Result() { return container; }
  simdjson_really_inline size_t ItemCount() { return container.size(); }

private:
  dom::parser parser{};
  std::vector<my_point> container{};
};

simdjson_really_inline bool Dom::Run(const padded_string &json) {
  container.clear();

  dom::element doc = parser.parse(json);
  for (auto point : doc) {
    container.emplace_back(my_point{point["x"], point["y"], point["z"]});
  }

  return true;
}

BENCHMARK_TEMPLATE(LargeRandom, Dom);

} // namespace largerandom

#endif // SIMDJSON_EXCEPTIONS