#pragma once

#ifdef SIMDJSON_COMPETITION_BOOSTJSON

#include "partial_tweets.h"

namespace partial_tweets {

struct boostjson {
  using StringType=std::string;

  bool run(simdjson::padded_string &json, std::vector<tweet<StringType>> &result) {

    auto root = boost::json::parse(json);
    for (const auto &tweet : root.at("statuses").as_array()) {
      const auto &user = tweet.at("user");

      auto in_reply_to_status_id = tweet.as_object().if_contains("in_reply_to_status_id")
        ? tweet.at("in_reply_to_status_id") : boost::json::value();

      result.emplace_back(partial_tweets::tweet<StringType>{
        tweet.at("created_at").as_string().c_str(),
        tweet.at("id").to_number<uint64_t>(),
        tweet.at("text").as_string().c_str(),
        in_reply_to_status_id.is_null() ? 0 : in_reply_to_status_id.to_number<uint64_t>(),
        {
          user.at("id").to_number<uint64_t>(),
          user.at("screen_name").as_string().c_str()
        },
        tweet.at("retweet_count").to_number<uint64_t>(),
        tweet.at("favorite_count").to_number<uint64_t>()
      });
    }

    return true;
  }
};

BENCHMARK_TEMPLATE(partial_tweets, boostjson)->UseManualTime();

} // namespace partial_tweets

#endif // SIMDJSON_COMPETITION_BOOSTJSON
