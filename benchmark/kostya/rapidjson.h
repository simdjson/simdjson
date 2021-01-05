#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "kostya.h"

namespace kostya {

using namespace rapidjson;

struct rapidjson_base {
  Document doc;

  simdjson_really_inline double get_double(Value &object, std::string_view key) {
    auto field = object.FindMember(key.data());
    if (field == object.MemberEnd()) { throw "Missing double field"; }
    if (!field->value.IsNumber()) { throw "Field is not double"; }
    return field->value.GetDouble();
  }

  bool run(Document &root, std::vector<point> &points) {
    if (root.HasParseError()) { return false; }
    if (!root.IsObject()) { return false; }
    auto coords = root.FindMember("coordinates");
    if (coords == root.MemberEnd()) { return false; }
    if (!coords->value.IsArray()) { return false; }
    for (auto &coord : coords->value.GetArray()) {
      if (!coord.IsObject()) { return false; }
      points.emplace_back(point{get_double(coord, "x"), get_double(coord, "y"), get_double(coord, "z")});
    }

    return true;
  }
};

struct rapidjson : public rapidjson_base {
  bool run(const padded_string &json, std::vector<point> &points) {
    return rapidjson_base::run(doc.Parse<kParseValidateEncodingFlag>(json.data()), points);
  }
};
BENCHMARK_TEMPLATE(kostya, rapidjson);

struct rapidjson_lossless : public rapidjson_base {
  bool run(const padded_string &json, std::vector<point> &points) {
    return rapidjson_base::run(doc.Parse<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(json.data()), points);
  }
};
BENCHMARK_TEMPLATE(kostya, rapidjson_lossless);

struct rapidjson_insitu : public rapidjson_base {
  bool run(const padded_string &json, std::vector<point> &points) {
    padded_string json_copy{json.data(), json.size()};
    return rapidjson_base::run(doc.ParseInsitu<kParseValidateEncodingFlag>(json_copy.data()), points);
  }
};
BENCHMARK_TEMPLATE(kostya, rapidjson_insitu);

} // namespace kostya

#endif // SIMDJSON_COMPETITION_RAPIDJSON
