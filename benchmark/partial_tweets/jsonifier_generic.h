#pragma once

#ifdef SIMDJSON_COMPETITION_JSONIFIER

#include "partial_tweets.h"

namespace partial_tweets {

namespace jsonifier_api = ::jsonifier::generic;

struct jsonifier_generic {
  using StringType=std::string_view;

  ::jsonifier::jsonifier_core<> parser{};

  simdjson_inline bool nullable_int(jsonifier_api::value value, uint64_t &out) {
    if (value.isNull()) { out = 0; return true; }
    return value.getUint64(out) == jsonifier_api::error_code::success;
  }

  bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
    jsonifier_api::document doc = parser.iterate(json);
    jsonifier_api::array statuses = doc.findField("statuses").getArray();
    if (statuses.error() != jsonifier_api::error_code::success) { return false; }
    for (jsonifier_api::value element : statuses) {
      jsonifier_api::object tweet = element.getObject();
      std::string_view created_at, text, screen_name;
      uint64_t id{}, in_reply_to_status_id{}, user_id{}, retweet_count{}, favorite_count{};
      if (tweet.findField("created_at").getString(created_at) != jsonifier_api::error_code::success) { return false; }
      if (tweet.findField("id").getUint64(id) != jsonifier_api::error_code::success) { return false; }
      if (tweet.findField("text").getString(text) != jsonifier_api::error_code::success) { return false; }
      if (!nullable_int(tweet.findField("in_reply_to_status_id"), in_reply_to_status_id)) { return false; }
      jsonifier_api::object user = tweet.findField("user").getObject();
      if (user.findField("id").getUint64(user_id) != jsonifier_api::error_code::success) { return false; }
      if (user.findField("screen_name").getString(screen_name) != jsonifier_api::error_code::success) { return false; }
      if (tweet.findField("retweet_count").getUint64(retweet_count) != jsonifier_api::error_code::success) { return false; }
      if (tweet.findField("favorite_count").getUint64(favorite_count) != jsonifier_api::error_code::success) { return false; }
      result.emplace_back(partial_tweets::tweet<std::string_view>{
        created_at,
        id,
        text,
        in_reply_to_status_id,
        { user_id, screen_name },
        retweet_count,
        favorite_count
      });
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(partial_tweets, jsonifier_generic)->UseManualTime();

} // namespace partial_tweets

#endif // SIMDJSON_COMPETITION_JSONIFIER
