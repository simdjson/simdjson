#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "large_random.h"

namespace large_random {

using namespace rapidjson;

template<int F>
class rapidjson_base {
  Document doc;

  simdjson_really_inline double get_double(Value &object, std::string_view key) {
    auto field = object.FindMember(key.data());
    if (field == object.MemberEnd()) { throw "Missing double field"; }
    if (!field->value.IsNumber()) { throw "Field is not double"; }
    return field->value.GetDouble();
  }

public:
  bool run(const simdjson::padded_string &json, std::vector<point> &points) {
    auto &coords = doc.Parse<F>(json.data());
    if (coords.HasParseError()) { return false; }
    if (!coords.IsArray()) { return false; }
    for (auto &coord : coords.GetArray()) {
      if (!coord.IsObject()) { return false; }
      points.emplace_back(point{get_double(coord, "x"), get_double(coord, "y"), get_double(coord, "z")});
    }

    return true;
  }
};

class rapidjson : public rapidjson_base<kParseValidateEncodingFlag> {};
class rapidjson_lossless : public rapidjson_base<kParseValidateEncodingFlag | kParseFullPrecisionFlag> {};
BENCHMARK_TEMPLATE(large_random, rapidjson);
BENCHMARK_TEMPLATE(large_random, rapidjson_lossless);

} // namespace large_random

#endif // SIMDJSON_COMPETITION_RAPIDJSON
