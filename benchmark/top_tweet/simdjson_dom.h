#pragma once

#if SIMDJSON_EXCEPTIONS

#include "top_tweet.h"

namespace top_tweet {

using namespace simdjson;

struct simdjson_dom {
  using StringType=std::string_view;

  dom::parser parser{};

  bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    result.retweet_count = -1;
    dom::element top_tweet{};

    auto doc = parser.parse(json);
    for (auto tweet : doc["statuses"]) {
      int64_t retweet_count = tweet["retweet_count"];
      if (retweet_count <= max_retweet_count && retweet_count >= result.retweet_count) {
        result.retweet_count = retweet_count;
        top_tweet = tweet;
      }
    }

    result.text = top_tweet["text"];
    result.screen_name = top_tweet["user"]["screen_name"];
    return result.retweet_count != -1;
  }
};

BENCHMARK_TEMPLATE(top_tweet, simdjson_dom)->UseManualTime();

} // namespace top_tweet

#endif // SIMDJSON_EXCEPTIONS