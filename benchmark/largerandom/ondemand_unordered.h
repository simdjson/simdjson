#pragma once

#if SIMDJSON_EXCEPTIONS

#include "largerandom.h"

namespace largerandom {

using namespace simdjson;
using namespace simdjson::builtin;

class OnDemandUnordered {
public:
  simdjson_really_inline bool Run(const padded_string &json);
  simdjson_really_inline const std::vector<my_point> &Result() { return container; }
  simdjson_really_inline size_t ItemCount() { return container.size(); }

private:
  ondemand::parser parser{};
  std::vector<my_point> container{};
};

simdjson_really_inline bool OnDemandUnordered::Run(const padded_string &json) {
  container.clear();

  auto doc = parser.iterate(json);
  for (ondemand::object coord : doc) {
    container.emplace_back(my_point{coord["x"], coord["y"], coord["z"]});
  }

  return true;
}

BENCHMARK_TEMPLATE(LargeRandom, OnDemandUnordered);

} // namespace largerandom

#endif // SIMDJSON_EXCEPTIONS
