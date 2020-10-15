#pragma once

#if SIMDJSON_EXCEPTIONS

#include "kostya.h"

namespace kostya {

using namespace simdjson;
using namespace simdjson::builtin;

class OnDemand {
public:
  simdjson_really_inline bool Run(const padded_string &json);
  simdjson_really_inline const std::vector<my_point> &Result() { return container; }
  simdjson_really_inline size_t ItemCount() { return container.size(); }

private:
  ondemand::parser parser{};
  std::vector<my_point> container{};
};

simdjson_really_inline bool OnDemand::Run(const padded_string &json) {
  container.clear();

  using std::cout;
  using std::endl;

  auto doc = parser.iterate(json);
  for (ondemand::object coord : doc["coordinates"]) {
    container.emplace_back(my_point{coord["x"], coord["y"], coord["z"]});
  }

  return true;
}

BENCHMARK_TEMPLATE(Kostya, OnDemand);


namespace sum {

class OnDemand {
public:
  simdjson_really_inline bool Run(const padded_string &json);
  simdjson_really_inline my_point &Result() { return sum; }
  simdjson_really_inline size_t ItemCount() { return count; }

private:
  ondemand::parser parser{};
  my_point sum{};
  size_t count{};
};

simdjson_really_inline bool OnDemand::Run(const padded_string &json) {
  sum = {0,0,0};
  count = 0;

  auto doc = parser.iterate(json);
  for (ondemand::object coord : doc["coordinates"]) {
    sum.x += double(coord["x"]);
    sum.y += double(coord["y"]);
    sum.z += double(coord["z"]);
    count++;
  }

  return true;
}

BENCHMARK_TEMPLATE(KostyaSum, OnDemand);

} // namespace sum
} // namespace kostya

#endif // SIMDJSON_EXCEPTIONS
