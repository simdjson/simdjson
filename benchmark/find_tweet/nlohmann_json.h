#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "find_tweet.h"

namespace find_tweet {

struct nlohmann_json {
  using StringType=std::string;

  bool run(simdjson::padded_string &json, uint64_t find_id, std::string &result) {
    auto root = nlohmann::json::parse(json.data(), json.data() + json.size());
    for (auto tweet : root["statuses"]) {
      if (tweet["id"] == find_id) {
        result = tweet["text"];
        return true;
      }
    }

    return false;
  }
};

BENCHMARK_TEMPLATE(find_tweet, nlohmann_json)->UseManualTime();

} // namespace find_tweet

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON