#pragma once

#if SIMDJSON_EXCEPTIONS && SIMDJSON_STATIC_REFLECTION

#include "accessor_benchmark.h"

namespace accessor_performance {

using namespace simdjson;

struct compile_time_at_path_simple {
  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::string &result_str, int64_t&, double&, bool&) {
    auto doc = parser.iterate(json);
    std::string_view name;
    auto r = ondemand::json_path::at_path_compiled<TestData, ".name">(doc);
    if (r.get(name) != SUCCESS) return false;
    result_str = name;
    return true;
  }
};

struct compile_time_at_path_nested {
  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::string &result_str, int64_t&, double&, bool&) {
    auto doc = parser.iterate(json);
    std::string_view city;
    auto r = ondemand::json_path::at_path_compiled<TestData, ".address.city">(doc);
    if (r.get(city) != SUCCESS) return false;
    result_str = city;
    return true;
  }
};

struct compile_time_at_path_deep {
  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::string&, int64_t&, double &result_dbl, bool&) {
    auto doc = parser.iterate(json);
    double lat;
    auto r = ondemand::json_path::at_path_compiled<TestData, ".address.coordinates.lat">(doc);
    if (r.get(lat) != SUCCESS) return false;
    result_dbl = lat;
    return true;
  }
};

BENCHMARK_TEMPLATE(accessor_simple, compile_time_at_path_simple)->UseManualTime();
BENCHMARK_TEMPLATE(accessor_nested, compile_time_at_path_nested)->UseManualTime();
BENCHMARK_TEMPLATE(accessor_deep, compile_time_at_path_deep)->UseManualTime();

} // namespace accessor_performance

#endif // SIMDJSON_EXCEPTIONS && SIMDJSON_STATIC_REFLECTION
