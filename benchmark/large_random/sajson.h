#pragma once

#ifdef SIMDJSON_COMPETITION_SAJSON

#include "large_random.h"

namespace large_random {

struct sajson {
  static constexpr diff_flags DiffFlags = diff_flags::IMPRECISE_FLOATS;

  size_t ast_buffer_size{0};
  size_t *ast_buffer{nullptr};
  ~sajson() { free(ast_buffer); }

  simdjson_inline double get_double(const ::sajson::value &obj, std::string_view key) {
    using namespace sajson;

    auto val = obj.get_value_of_key({key.data(), key.length()});
    switch (val.get_type()) {
      case TYPE_INTEGER:
      case TYPE_DOUBLE:
        return val.get_number_value();
      default:
        throw "field not double";
    }
  }

  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    using namespace sajson;

    if (!ast_buffer) {
      ast_buffer_size = json.size();
      ast_buffer = (size_t *)std::malloc(ast_buffer_size * sizeof(size_t));
    }
    auto doc = parse(
      bounded_allocation(ast_buffer, ast_buffer_size),
      mutable_string_view(json.size(), json.data())
    );
    if (!doc.is_valid()) { return false; }

    auto points = doc.get_root();
    if (points.get_type() != TYPE_ARRAY) { return false; }

    for (size_t i=0; i<points.get_length(); i++) {
      auto point = points.get_array_element(i);
      if (point.get_type() != TYPE_OBJECT) { return false; }
      result.emplace_back(json_benchmark::point{
        get_double(point, "x"),
        get_double(point, "y"),
        get_double(point, "z")
      });
    }

    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, sajson)->UseManualTime();

} // namespace large_random

#endif // SIMDJSON_COMPETITION_SAJSON

