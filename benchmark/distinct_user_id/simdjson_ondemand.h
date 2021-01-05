#pragma once

#if SIMDJSON_EXCEPTIONS

#include "distinct_user_id.h"

namespace distinct_user_id {

using namespace simdjson;
using namespace simdjson::builtin;

struct simdjson_ondemand {
  ondemand::parser parser{};

  bool run(const simdjson::padded_string &json, std::vector<uint64_t> &ids) {
    // Walk the document, parsing as we go
    auto doc = parser.iterate(json);
    for (ondemand::object tweet : doc.find_field("statuses")) {
      // We believe that all statuses have a matching
      // user, and we are willing to throw when they do not.
      ids.push_back(tweet.find_field("user").find_field("id"));
      // Not all tweets have a "retweeted_status", but when they do
      // we want to go and find the user within.
      auto retweet = tweet.find_field("retweeted_status");
      if (!retweet.error()) {
        ids.push_back(retweet.find_field("user").find_field("id"));
      }
    }

    return true;
  }
};

BENCHMARK_TEMPLATE(distinct_user_id, simdjson_ondemand)->UseManualTime();

} // namespace distinct_user_id

#endif // SIMDJSON_EXCEPTIONS
