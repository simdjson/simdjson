#pragma once

#include "diff_results.h"

namespace json_benchmark {

struct point {
  double x;
  double y;
  double z;
};

template<>
struct result_differ<point, point> {
  static bool diff(benchmark::State &state, const point &result, const point &reference, diff_flags flags) {
    return diff_results(state, result.x, reference.x, flags)
        && diff_results(state, result.y, reference.y, flags)
        && diff_results(state, result.z, reference.z, flags);
  }
};

static simdjson_unused std::ostream &operator<<(std::ostream &o, const point &p) {
  return o << p.x << "," << p.y << "," << p.z << std::endl;
}

} // namespace json_benchmark
