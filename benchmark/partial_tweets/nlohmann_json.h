#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "partial_tweets.h"

namespace partial_tweets {

struct nlohmann_json {
  using StringType=std::string;

  simdjson_inline uint64_t nullable_int(nlohmann::json value) {
    if (value.is_null()) { return 0; }
    return value;
  }

  bool run(simdjson::padded_string &json, std::vector<tweet<std::string>> &result) {
    auto root = nlohmann::json::parse(json.data(), json.data() + json.size());
    for (auto tweet : root["statuses"]) {
      auto user = tweet["user"];
      result.emplace_back(partial_tweets::tweet<std::string>{
        tweet["created_at"],
        tweet["id"],
        tweet["text"],
        nullable_int(tweet["in_reply_to_status_id"]),
        { user["id"], user["screen_name"] },
        tweet["retweet_count"],
        tweet["favorite_count"]
      });
    }

    return true;
  }
};

BENCHMARK_TEMPLATE(partial_tweets, nlohmann_json)->UseManualTime();

} // namespace partial_tweets

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON
