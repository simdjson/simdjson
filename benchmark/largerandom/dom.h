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

  for (auto point : parser.parse(json)) {
    container.emplace_back(my_point{point["x"], point["y"], point["z"]});
  }

  return true;
}

BENCHMARK_TEMPLATE(LargeRandom, Dom);

namespace sum {

class Dom {
public:
  simdjson_really_inline bool Run(const padded_string &json);

  simdjson_really_inline my_point &Result() { return sum; }
  simdjson_really_inline size_t ItemCount() { return count; }

private:
  dom::parser parser{};
  my_point sum{};
  size_t count{};
};

simdjson_really_inline bool Dom::Run(const padded_string &json) {
  sum = { 0, 0, 0 };
  count = 0;

  for (auto coord : parser.parse(json)) {
    sum.x += double(coord["x"]);
    sum.y += double(coord["y"]);
    sum.z += double(coord["z"]);
    count++;
  }

  return true;
}

BENCHMARK_TEMPLATE(LargeRandomSum, Dom);

} // namespace sum
} // namespace largerandom

#endif // SIMDJSON_EXCEPTIONS