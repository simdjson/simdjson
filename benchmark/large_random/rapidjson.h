#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "large_random.h"

namespace large_random {

using namespace rapidjson;

struct rapidjson_base {
  static constexpr diff_flags DiffFlags = diff_flags::NONE;

  Document doc;

  simdjson_really_inline double get_double(Value &object, std::string_view key) {
    auto field = object.FindMember(key.data());
    if (field == object.MemberEnd()) { throw "Missing double field"; }
    if (!field->value.IsNumber()) { throw "Field is not double"; }
    return field->value.GetDouble();
  }

  bool run(Document &coords, std::vector<point> &result) {
    if (coords.HasParseError()) { return false; }
    if (!coords.IsArray()) { return false; }
    for (auto &coord : coords.GetArray()) {
      if (!coord.IsObject()) { return false; }
      result.emplace_back(json_benchmark::point{get_double(coord, "x"), get_double(coord, "y"), get_double(coord, "z")});
    }

    return true;
  }
};

struct rapidjson : rapidjson_base {
  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    return rapidjson_base::run(doc.Parse<kParseValidateEncodingFlag>(json.data()), result);
  }
};
BENCHMARK_TEMPLATE(large_random, rapidjson)->UseManualTime();

struct rapidjson_lossless : rapidjson_base {
  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    return rapidjson_base::run(doc.Parse<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(json.data()), result);
  }
};
BENCHMARK_TEMPLATE(large_random, rapidjson_lossless)->UseManualTime();

struct rapidjson_insitu : rapidjson_base {
  bool run(simdjson::padded_string &json, std::vector<point> &result) {
    return rapidjson_base::run(doc.ParseInsitu<kParseValidateEncodingFlag>(json.data()), result);
  }
};
BENCHMARK_TEMPLATE(large_random, rapidjson_insitu)->UseManualTime();


} // namespace large_random

#endif // SIMDJSON_COMPETITION_RAPIDJSON
