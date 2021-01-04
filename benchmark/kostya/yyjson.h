#pragma once

#ifdef SIMDJSON_COMPETITION_YYJSON

#include "kostya.h"

namespace kostya {

class yyjson {
public:
  simdjson_really_inline double get_double(yyjson_val *obj, std::string_view key) {
    yyjson_val *val = yyjson_obj_getn(obj, key.data(), key.length());
    return yyjson_get_real(val);
  }

  bool run(const simdjson::padded_string &json, std::vector<point> &points) {
    yyjson_doc *doc = yyjson_read(json.data(), json.size(), 0);
    if (!doc) { return false; }
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *coords = yyjson_obj_get(root, "coordinates");

    size_t idx, max;
    yyjson_val *coord;
    yyjson_arr_foreach(coords, idx, max, coord) {
      points.emplace_back(point{get_double(coord, "x"), get_double(coord, "y"), get_double(coord, "z")});
    }

    return true;
  }

};

BENCHMARK_TEMPLATE(kostya, yyjson);

} // namespace kostya

#endif // SIMDJSON_COMPETITION_YYJSON
