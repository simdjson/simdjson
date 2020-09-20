#pragma once

#if SIMDJSON_EXCEPTIONS

#include "kostya.h"

namespace kostya {

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

  simdjson_really_inline simdjson_result<double> first_double(ondemand::json_iterator &iter, const char *key) {
    if (!iter.start_object() || ondemand::raw_json_string(iter.field_key()) != key || iter.field_value()) { throw "Invalid field"; }
    return iter.consume_double();
  }

  simdjson_really_inline simdjson_result<double> next_double(ondemand::json_iterator &iter, const char *key) {
    if (!iter.has_next_field() || ondemand::raw_json_string(iter.field_key()) != key || iter.field_value()) { throw "Invalid field"; }
    return iter.consume_double();
  }

};

simdjson_really_inline bool Iter::Run(const padded_string &json) {
  container.clear();

  using std::cerr;
  using std::endl;
  auto iter = parser.iterate_raw(json).value();
  if (!iter.start_object() || !iter.find_field_raw("coordinates")) { cerr << "find coordinates field failed" << endl; return false; }
  if (iter.start_array()) {
    do {
      container.emplace_back(my_point{first_double(iter, "x"), next_double(iter, "y"), next_double(iter, "z")});
      if (iter.skip_container()) { return false; } // Skip the rest of the coordinates object
    } while (iter.has_next_element());
  }

  return true;
}

BENCHMARK_TEMPLATE(Kostya, Iter);


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
  if (!iter.start_object() || !iter.find_field_raw("coordinates")) { return false; }
  if (!iter.start_array()) { return false; }
  do {
    if (!iter.start_object()   || !iter.find_field_raw("x")) { return false; }
    sum.x += iter.consume_double();
    if (!iter.has_next_field() || !iter.find_field_raw("y")) { return false; }
    sum.y +=  iter.consume_double();
    if (!iter.has_next_field() || !iter.find_field_raw("z")) { return false; }
    sum.z +=  iter.consume_double();
    if (iter.skip_container()) { return false; } // Skip the rest of the coordinates object
    count++;
  } while (iter.has_next_element());

  return true;
}

BENCHMARK_TEMPLATE(KostyaSum, Iter);

} // namespace sum
} // namespace kostya

#endif // SIMDJSON_EXCEPTIONS
