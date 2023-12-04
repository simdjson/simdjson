#pragma once

#ifdef SIMDJSON_COMPETITION_BOOSTJSON

#include "distinct_user_id.h"

namespace distinct_user_id {

struct boostjson {
  bool run(simdjson::padded_string &json, std::vector<uint64_t> &result) {

    auto root = boost::json::parse(json);
    for (const auto &tweet : root.at("statuses").as_array()) {
      result.push_back(tweet.at("user").at("id").to_number<uint64_t>());

      if (tweet.as_object().if_contains("retweeted_status")) {
        result.push_back(tweet.at("retweeted_status").at("user").at("id").to_number<uint64_t>());
      }
    }

    return true;
  }
};

BENCHMARK_TEMPLATE(distinct_user_id, boostjson)->UseManualTime();

} // namespace distinct_user_id

#endif // SIMDJSON_COMPETITION_BOOSTJSON
