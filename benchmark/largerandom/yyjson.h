#pragma once

#include "largerandom.h"

namespace largerandom {

class Yyjson {
public:
  simdjson_really_inline const std::vector<my_point> &Result() { return container; }
  simdjson_really_inline size_t ItemCount() { return container.size(); }

private:
  ondemand::parser parser{};
  std::vector<my_point> container{};

  simdjson_really_inline double get_double(yyjson_val *obj, std::string_view key) {
    yyjson_val *val = yyjson_obj_getn(obj, key.data(), key.length());
    return yyjson_get_real(val);
  }

public:
  simdjson_really_inline bool Run(const padded_string &json) {
    container.clear();

    // Walk the document, parsing the tweets as we go
    yyjson_doc *doc = yyjson_read(json.data(), json.size(), 0);
    if (!doc) { return false; }
    yyjson_val *coords = yyjson_doc_get_root(doc);

    size_t idx, max;
    yyjson_val *coord;
    yyjson_arr_foreach(coords, idx, max, coord) {
      container.emplace_back(my_point{get_double(coord, "x"), get_double(coord, "y"), get_double(coord, "z")});
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(LargeRandom, Yyjson);

} // namespace kostya
