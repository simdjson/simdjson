#pragma once

#ifdef SIMDJSON_COMPETITION_JSONIFIER

#include "distinct_user_id.h"

namespace distinct_user_id {

namespace jsonifier_api = ::jsonifier::generic;

struct jsonifier_generic {
  ::jsonifier::jsonifier_core<> parser{};

  bool run(simdjson::padded_string &json, std::vector<uint64_t> &result) {
    jsonifier_api::document doc = parser.iterate(json);
    jsonifier_api::array statuses = doc.findField("statuses").getArray();
    if (statuses.error() != jsonifier_api::error_code::success) { return false; }
    for (jsonifier_api::value element : statuses) {
      jsonifier_api::object tweet = element.getObject();
      uint64_t id{};
      if (tweet.findField("user").findField("id").getUint64(id) != jsonifier_api::error_code::success) { return false; }
      result.push_back(id);
      jsonifier_api::value retweet = tweet.findField("retweeted_status");
      if (retweet.error() == jsonifier_api::error_code::success) {
        if (retweet.findField("user").findField("id").getUint64(id) != jsonifier_api::error_code::success) { return false; }
        result.push_back(id);
      } else if (retweet.error() != jsonifier_api::error_code::no_such_field) {
        return false;
      }
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(distinct_user_id, jsonifier_generic)->UseManualTime();

} // namespace distinct_user_id

#endif // SIMDJSON_COMPETITION_JSONIFIER
