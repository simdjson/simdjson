#pragma once

#if SIMDJSON_EXCEPTIONS

#include "accessor_benchmark.h"

namespace accessor_performance {

using namespace simdjson;

struct runtime_at_path_simple {
  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::string &result_str, int64_t&, double&, bool&) {
    auto doc = parser.iterate(json);
    std::string_view name;
    if (doc.at_path(".name").get(name) != SUCCESS) return false;
    result_str = name;
    return true;
  }
};

struct runtime_at_path_nested {
  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::string &result_str, int64_t&, double&, bool&) {
    auto doc = parser.iterate(json);
    std::string_view city;
    if (doc.at_path(".address.city").get(city) != SUCCESS) return false;
    result_str = city;
    return true;
  }
};

struct runtime_at_path_deep {
  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::string&, int64_t&, double &result_dbl, bool&) {
    auto doc = parser.iterate(json);
    double lat;
    if (doc.at_path(".address.coordinates.lat").get(lat) != SUCCESS) return false;
    result_dbl = lat;
    return true;
  }
};

BENCHMARK_TEMPLATE(accessor_simple, runtime_at_path_simple)->UseManualTime();
BENCHMARK_TEMPLATE(accessor_nested, runtime_at_path_nested)->UseManualTime();
BENCHMARK_TEMPLATE(accessor_deep, runtime_at_path_deep)->UseManualTime();

} // namespace accessor_performance

#endif // SIMDJSON_EXCEPTIONS
