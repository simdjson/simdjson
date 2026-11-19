#pragma once

#ifdef SIMDJSON_COMPETITION_JSONIFIER

#include "top_tweet.h"

namespace top_tweet {

namespace jsonifier_api = ::jsonifier::generic;

struct jsonifier_generic {
  using StringType=std::string_view;

  ::jsonifier::jsonifier_core<> parser{};

  bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    result.retweet_count = -1;
    jsonifier_api::value screen_name, text;

    jsonifier_api::document doc = parser.iterate(json);
    jsonifier_api::array statuses = doc.findField("statuses").getArray();
    if (statuses.error() != jsonifier_api::error_code::success) { return false; }
    for (jsonifier_api::value element : statuses) {
      jsonifier_api::object tweet = element.getObject();
      jsonifier_api::value tweet_text = tweet.findField("text");
      jsonifier_api::value tweet_screen_name = tweet.findField("user").findField("screen_name");
      int64_t retweet_count{};
      if (tweet.findField("retweet_count").getInt64(retweet_count) != jsonifier_api::error_code::success) { return false; }
      if (retweet_count <= max_retweet_count && retweet_count >= result.retweet_count) {
        result.retweet_count = retweet_count;
        text = tweet_text;
        screen_name = tweet_screen_name;
      }
    }

    if (result.retweet_count == -1) { return false; }
    if (screen_name.getString(result.screen_name) != jsonifier_api::error_code::success) { return false; }
    return text.getString(result.text) == jsonifier_api::error_code::success;
  }
};

BENCHMARK_TEMPLATE(top_tweet, jsonifier_generic)->UseManualTime();

} // namespace top_tweet

#endif // SIMDJSON_COMPETITION_JSONIFIER
