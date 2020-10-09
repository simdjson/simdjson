#pragma once

#if SIMDJSON_EXCEPTIONS

#include "largerandom.h"

namespace largerandom {

using namespace simdjson;
using namespace simdjson::builtin;

class Iter {
public:
  simdjson_really_inline bool Run(const padded_string &json);

  simdjson_really_inline const std::vector<my_point> &Result() { return container; }
  simdjson_really_inline size_t ItemCount() { return container.size(); }

private:
  ondemand::parser parser{};
  std::vector<my_point> container{};

  simdjson_really_inline double first_double(ondemand::json_iterator &iter) {
    if (iter.start_object().error() || iter.field_key().error() || iter.field_value()) { throw "Invalid field"; }
    return iter.consume_double();
  }

  simdjson_really_inline double next_double(ondemand::json_iterator &iter) {
    if (!iter.has_next_field() || iter.field_key().error() || iter.field_value()) { throw "Invalid field"; }
    return iter.consume_double();
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


namespace sum {

class Iter {
public:
  simdjson_really_inline bool Run(const padded_string &json);

  simdjson_really_inline my_point &Result() { return sum; }
  simdjson_really_inline size_t ItemCount() { return count; }

private:
  ondemand::parser parser{};
  my_point sum{};
  size_t count{};
};

simdjson_really_inline bool Iter::Run(const padded_string &json) {
  sum = {0,0,0};
  count = 0;

  auto iter = parser.iterate_raw(json).value();
  if (!iter.start_array()) { return false; }
  do {
    if (!iter.start_object()   || iter.field_key().value() != "x" || iter.field_value()) { return false; }
    sum.x += iter.consume_double();
    if (!iter.has_next_field() || iter.field_key().value() != "y" || iter.field_value()) { return false; }
    sum.y +=  iter.consume_double();
    if (!iter.has_next_field() || iter.field_key().value() != "z" || iter.field_value()) { return false; }
    sum.z +=  iter.consume_double();
    if (*iter.advance() != '}') { return false; }
    count++;
  } while (iter.has_next_element());

  return true;
}

BENCHMARK_TEMPLATE(LargeRandomSum, Iter);

} // namespace sum
} // namespace largerandom

#endif // SIMDJSON_EXCEPTIONS
