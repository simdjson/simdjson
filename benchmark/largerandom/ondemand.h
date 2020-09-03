#pragma once

#ifdef SIMDJSON_IMPLEMENTATION
#if SIMDJSON_EXCEPTIONS

#include "largerandom.h"

namespace largerandom {
namespace {

using namespace simdjson;
using namespace SIMDJSON_IMPLEMENTATION;
using namespace SIMDJSON_IMPLEMENTATION::stage2;

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

  auto doc = parser.iterate(json);
  // TODO you should be able to just say for ( ... : doc)
  auto array = doc.get_array();
  for (ondemand::object point_object : array) {
    container.emplace_back(my_point{point_object["x"], point_object["y"], point_object["z"]});
  }

  return true;
}

BENCHMARK_TEMPLATE(LargeRandom, OnDemand);

} // unnamed namespace

namespace sum {
namespace {

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
  // TODO this sucks, you should be able to just say for ( ... : doc)
  auto array = doc.get_array();
  for (ondemand::object coord : array) {
    sum.x += double(coord["x"]);
    sum.y += double(coord["y"]);
    sum.z += double(coord["z"]);
    count++;
  }

  return true;
}

BENCHMARK_TEMPLATE(LargeRandomSum, OnDemand);

} // unnamed namespace
} // namespace sum
} // namespace largerandom

#endif // SIMDJSON_EXCEPTIONS
#endif // SIMDJSON_IMPLEMENTATION
