#pragma once

#ifdef SIMDJSON_COMPETITION_BOOSTJSON

#include "find_tweet.h"

namespace find_tweet {

struct boostjson {
  using StringType=std::string;

  bool run(simdjson::padded_string &json, uint64_t find_id, std::string &result) {

    auto root = boost::json::parse(json);
    for (const auto &tweet : root.at("statuses").as_array()) {
      if (tweet.at("id") == find_id) {
        result = tweet.at("text").as_string();
        return true;
      }
    }

    return false;
  }
};

BENCHMARK_TEMPLATE(find_tweet, boostjson)->UseManualTime();

} // namespace find_tweet

#endif // SIMDJSON_COMPETITION_BOOSTJSON
