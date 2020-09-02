#pragma once

#ifdef SIMDJSON_IMPLEMENTATION
#if SIMDJSON_EXCEPTIONS

#include "largerandom.h"

namespace largerandom {

using namespace simdjson;
using namespace SIMDJSON_IMPLEMENTATION;
using namespace SIMDJSON_IMPLEMENTATION::stage2;

class Iter {
public:
  simdjson_really_inline bool Run(const padded_string &json);
  simdjson_really_inline bool SetUp() { return true; }
  simdjson_really_inline bool TearDown() { return true; }
  simdjson_really_inline const std::vector<my_point> &Records() { return container; }

private:
  ondemand::parser parser;
  std::vector<my_point> container;

  simdjson_really_inline double first_double(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &iter) {
    if (iter.start_object().error() || iter.field_key().error() || iter.field_value()) { throw "Invalid field"; }
    return iter.get_double();
  }

  simdjson_really_inline double next_double(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &iter) {
    if (!iter.has_next_field() || iter.field_key().error() || iter.field_value()) { throw "Invalid field"; }
    return iter.get_double();
  }

};

simdjson_really_inline bool Iter::Run(const padded_string &json) {
  container.clear();

  auto iter = parser.iterate_raw(json).value();
  if (iter.start_array()) {
    do {
      container.emplace_back(my_point{first_double(iter), next_double(iter), next_double(iter)});
      if (iter.has_next_field()) { throw "Too many fields"; }
    } while (iter.has_next_element());
  }

  return true;
}

BENCHMARK_TEMPLATE(LargeRandom, Iter);

} // namespace largerandom

#endif // SIMDJSON_EXCEPTIONS
#endif // SIMDJSON_IMPLEMENTATION
