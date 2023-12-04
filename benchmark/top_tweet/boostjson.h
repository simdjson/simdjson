#pragma once

#if SIMDJSON_COMPETITION_BOOSTJSON

#include "top_tweet.h"

namespace top_tweet {

using namespace simdjson;

struct boostjson {
  using StringType=std::string;

  bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    result.retweet_count = -1;
    boost::json::value top_tweet{};

    auto root = boost::json::parse(json);
    for (const auto &tweet : root.at("statuses").as_array()) {
      int64_t retweet_count = tweet.at("retweet_count").as_int64();
      if (retweet_count <= max_retweet_count && retweet_count >= result.retweet_count) {
        result.retweet_count = retweet_count;
        top_tweet = tweet;
      }
    }

    result.text = top_tweet.at("text").as_string();
    result.screen_name = top_tweet.at("user").at("screen_name").as_string();
    return result.retweet_count != -1;
  }
};

BENCHMARK_TEMPLATE(top_tweet, boostjson)->UseManualTime();

} // namespace top_tweet

#endif // SIMDJSON_COMPETITION_BOOSTJSON