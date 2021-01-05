#pragma once

#ifdef SIMDJSON_COMPETITION_YYJSON

#include "kostya.h"

namespace kostya {

struct yyjson {
  simdjson_really_inline double get_double(yyjson_val *obj, std::string_view key) {
    yyjson_val *val = yyjson_obj_getn(obj, key.data(), key.length());
    if (!val) { throw "missing point field!"; }
    if (yyjson_get_type(val) != YYJSON_TYPE_NUM) { throw "Number is not a type!"; }

    switch (yyjson_get_subtype(val)) {
      case YYJSON_SUBTYPE_UINT:
        return yyjson_get_uint(val);
      case YYJSON_SUBTYPE_SINT:
        return yyjson_get_sint(val);
      case YYJSON_SUBTYPE_REAL:
        return yyjson_get_real(val);
      default:
        SIMDJSON_UNREACHABLE();
    }
  }

  bool run(const simdjson::padded_string &json, std::vector<point> &points) {
    yyjson_doc *doc = yyjson_read(json.data(), json.size(), 0);
    if (!doc) { return false; }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) { return false; }
    yyjson_val *coords = yyjson_obj_get(root, "coordinates");
    if (!yyjson_is_arr(coords)) { return false; }

    size_t idx, max;
    yyjson_val *coord;
    yyjson_arr_foreach(coords, idx, max, coord) {
      if (!yyjson_is_obj(coord)) { return false; }
      points.emplace_back(point{get_double(coord, "x"), get_double(coord, "y"), get_double(coord, "z")});
    }

    return true;
  }

};

BENCHMARK_TEMPLATE(kostya, yyjson);

} // namespace kostya

#endif // SIMDJSON_COMPETITION_YYJSON
