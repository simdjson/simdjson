#pragma once

#ifdef SIMDJSON_COMPETITION_YYJSON

#include "large_random.h"

namespace large_random {

class yyjson {
  ondemand::parser parser{};

  simdjson_really_inline double get_double(yyjson_val *obj, std::string_view key) {
    yyjson_val *val = yyjson_obj_getn(obj, key.data(), key.length());
    if (yyjson_get_type(val) != YYJSON_TYPE_NUM) { return 0; }

    switch (yyjson_get_subtype(val)) {
      case YYJSON_SUBTYPE_UINT:
        return yyjson_get_uint(val);
      case YYJSON_SUBTYPE_SINT:
        return yyjson_get_sint(val);
      case YYJSON_SUBTYPE_REAL:
        return yyjson_get_real(val);
      default:
        return 0;
    }
  }

public:
  bool run(const simdjson::padded_string &json, std::vector<point> &points) {
    // Walk the document, parsing the tweets as we go
    yyjson_doc *doc = yyjson_read(json.data(), json.size(), 0);
    if (!doc) { return false; }
    yyjson_val *coords = yyjson_doc_get_root(doc);

    size_t idx, max;
    yyjson_val *coord;
    yyjson_arr_foreach(coords, idx, max, coord) {
      points.emplace_back(point{get_double(coord, "x"), get_double(coord, "y"), get_double(coord, "z")});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(large_random, yyjson);

} // namespace large_random

#endif // SIMDJSON_COMPETITION_YYJSON
