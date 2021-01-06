#pragma once

#ifdef SIMDJSON_COMPETITION_NLOHMANN_JSON

#include "distinct_user_id.h"

namespace distinct_user_id {

struct nlohmann_json {
  bool run(simdjson::padded_string &json, std::vector<uint64_t> &result) {
    auto root = nlohmann::json::parse(json.data(), json.data() + json.size());
    for (auto tweet : root["statuses"]) {
      result.push_back(tweet["user"]["id"]);
      if (tweet.contains("retweeted_status")) {
        result.push_back(tweet["retweeted_status"]["user"]["id"]);
      }
    }

    return true;
  }
};

BENCHMARK_TEMPLATE(distinct_user_id, nlohmann_json)->UseManualTime();

} // namespace distinct_user_id

#endif // SIMDJSON_COMPETITION_NLOHMANN_JSON