#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "kostya.h"

namespace kostya {

using namespace rapidjson;

struct rapidjson_base {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  Document doc;

  simdjson_inline double get_double(Value &object, std::string_view key) {
    auto field = object.FindMember(key.data());
    if (field == object.MemberEnd()) { throw "Missing double field"; }
    if (!field->value.IsNumber()) { throw "Field is not double"; }
    return field->value.GetDouble();
  }

  bool run(Document &root, std::vector<point> &result) {
    if (root.HasParseError()) { return false; }
    if (!root.IsObject()) { return false; }
    auto coords = root.FindMember("coordinates");
    if (coords == root.MemberEnd()) { return false; }
    if (!coords->value.IsArray()) { return false; }
    for (auto &coord : coords->value.GetArray()) {
      if (!coord.IsObject()) { return false; }
      result.emplace_back(json_benchmark::point{get_double(coord, "x"), get_double(coord, "y"), get_double(coord, "z")});
    }

    return true;
  }
};
#if SIMDJSON_COMPETITION_ONDEMAND_APPROX
struct rapidjson_approx : rapidjson_base {
  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    return rapidjson_base::run(doc.Parse<kParseValidateEncodingFlag>(json.data()), result);
  }
};
BENCHMARK_TEMPLATE(kostya, rapidjson_approx)->UseManualTime();
#endif // SIMDJSON_COMPETITION_ONDEMAND_APPROX

struct rapidjson : rapidjson_base {
  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    return rapidjson_base::run(doc.Parse<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(json.data()), result);
  }
};
BENCHMARK_TEMPLATE(kostya, rapidjson)->UseManualTime();
#if SIMDJSON_COMPETITION_ONDEMAND_INSITU
struct rapidjson_insitu : rapidjson_base {
  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    return rapidjson_base::run(doc.ParseInsitu<kParseValidateEncodingFlag|kParseInsituFlag>(json.data()), result);
  }
};
BENCHMARK_TEMPLATE(kostya, rapidjson_insitu)->UseManualTime();
#endif // SIMDJSON_COMPETITION_ONDEMAND_INSITU
} // namespace kostya

#endif // SIMDJSON_COMPETITION_RAPIDJSON
