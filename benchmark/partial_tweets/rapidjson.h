#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "partial_tweets.h"

namespace partial_tweets {

using namespace rapidjson;

struct rapidjson_base {
  using StringType=std::string_view;

  Document doc{};

  simdjson_inline std::string_view get_string_view(Value &object, std::string_view key) {
    // TODO use version that supports passing string length?
    auto field = object.FindMember(key.data());
    if (field == object.MemberEnd()) { throw "Missing object field"; }
    if (!field->value.IsString()) { throw "Field is not a string"; }
    return { field->value.GetString(), field->value.GetStringLength() };
  }
  simdjson_inline uint64_t get_uint64(Value &object, std::string_view key) {
    auto field = object.FindMember(key.data());
    if (field == object.MemberEnd()) { throw "Missing object field"; }
    if (!field->value.IsUint64()) { throw "Field is not uint64"; }
    return field->value.GetUint64();
  }
  simdjson_inline uint64_t get_nullable_uint64(Value &object, std::string_view key) {
    auto field = object.FindMember(key.data());
    if (field == object.MemberEnd()) { throw "Missing nullable uint64 field"; }
    if (field->value.IsNull()) { return 0; }
    if (!field->value.IsUint64()) { throw "Field is not nullable uint64"; }
    return field->value.GetUint64();
  }
  simdjson_inline partial_tweets::twitter_user<std::string_view> get_user(Value &object, std::string_view key) {
    auto field = object.FindMember(key.data());
    if (field == object.MemberEnd()) { throw "Missing user field"; }
    if (!field->value.IsObject()) { throw "User field is not an object"; }
    return { get_uint64(field->value, "id"), get_string_view(field->value, "screen_name") };
  }

  bool run(Document &root, std::vector<tweet<std::string_view>> &result) {
    if (root.HasParseError() || !root.IsObject()) { return false; }
    auto statuses = root.FindMember("statuses");
    if (statuses == root.MemberEnd() || !statuses->value.IsArray()) { return false; }
    for (auto &tweet : statuses->value.GetArray()) {
      if (!tweet.IsObject()) { return false; }
      result.emplace_back(partial_tweets::tweet<std::string_view>{
        get_string_view(tweet, "created_at"),
        get_uint64     (tweet, "id"),
        get_string_view(tweet, "text"),
        get_nullable_uint64     (tweet, "in_reply_to_status_id"),
        get_user       (tweet, "user"),
        get_uint64     (tweet, "retweet_count"),
        get_uint64     (tweet, "favorite_count")
      });
    }

    return true;
  }
};

struct rapidjson : rapidjson_base {
  bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
    return rapidjson_base::run(doc.Parse<kParseValidateEncodingFlag>(json.data()), result);
  }
};
BENCHMARK_TEMPLATE(partial_tweets, rapidjson)->UseManualTime();
#if SIMDJSON_COMPETITION_ONDEMAND_INSITU
struct rapidjson_insitu : rapidjson_base {
  bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
    return rapidjson_base::run(doc.ParseInsitu<kParseValidateEncodingFlag|kParseInsituFlag>(json.data()), result);
  }
};
BENCHMARK_TEMPLATE(partial_tweets, rapidjson_insitu)->UseManualTime();
#endif // SIMDJSON_COMPETITION_ONDEMAND_INSITU
} // namespace partial_tweets

#endif // SIMDJSON_COMPETITION_RAPIDJSON
