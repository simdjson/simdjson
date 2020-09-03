#pragma once

#if SIMDJSON_EXCEPTIONS

#include "kostya.h"

namespace kostya {

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

  for (auto point : parser.parse(json)["coordinates"]) {
    container.emplace_back(my_point{point["x"], point["y"], point["z"]});
  }

  return true;
}

BENCHMARK_TEMPLATE(Kostya, Dom);

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

  for (auto coord : parser.parse(json)["coordinates"]) {
    sum.x += double(coord["x"]);
    sum.y += double(coord["y"]);
    sum.z += double(coord["z"]);
    count++;
  }

  return true;
}

BENCHMARK_TEMPLATE(KostyaSum, Dom);

} // namespace sum
} // namespace kostya

#endif // SIMDJSON_EXCEPTIONS