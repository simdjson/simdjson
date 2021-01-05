#pragma once

#if SIMDJSON_EXCEPTIONS

#include "find_tweet.h"

namespace find_tweet {

using namespace simdjson;

struct simdjson_dom {
  dom::parser parser{};

  bool run(const simdjson::padded_string &json, uint64_t find_id, std::string_view &text) {
    text = "";
    auto doc = parser.parse(json);
    for (auto tweet : doc["statuses"]) {
      if (uint64_t(tweet["id"]) == find_id) {
        text = tweet["text"];
        return true;
      }
    }
    return false;
  }
};

BENCHMARK_TEMPLATE(find_tweet, simdjson_dom);

} // namespace find_tweet

#endif // SIMDJSON_EXCEPTIONS