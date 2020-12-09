#pragma once

#if SIMDJSON_EXCEPTIONS

#include "find_tweet.h"

namespace find_tweet {

using namespace simdjson;

class Dom {
public:
  simdjson_really_inline bool Run(const padded_string &json);
  simdjson_really_inline std::string_view Result() { return text; }
  simdjson_really_inline size_t ItemCount() { return 1; }

private:
  dom::parser parser{};
  std::string_view text{};
};

simdjson_really_inline bool Dom::Run(const padded_string &json) {
  text = "";
  auto doc = parser.parse(json);
  for (dom::object tweet : doc["statuses"]) {
      if (uint64_t(tweet["id"]) == TWEET_ID) {
        text = tweet["text"];
        return true;
      }
  }
  return false;
}

BENCHMARK_TEMPLATE(FindTweet, Dom);

} // namespace find_tweet

#endif // SIMDJSON_EXCEPTIONS