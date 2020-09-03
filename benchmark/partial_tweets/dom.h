#pragma once

#if SIMDJSON_EXCEPTIONS

#include "partial_tweets.h"

namespace partial_tweets {

using namespace simdjson;

class Dom {
public:
  simdjson_really_inline bool Run(const padded_string &json);

  simdjson_really_inline const std::vector<tweet> &Result() { return tweets; }
  simdjson_really_inline size_t ItemCount() { return tweets.size(); }

private:
  dom::parser parser{};
  std::vector<tweet> tweets{};

  simdjson_really_inline uint64_t nullable_int(dom::element element) {
    if (element.is_null()) { return 0; }
    return element;
  }
};

simdjson_really_inline bool Dom::Run(const padded_string &json) {
  tweets.clear();

  for (dom::element tweet : parser.parse(json)["statuses"]) {
    auto user = tweet["user"];
    tweets.emplace_back(partial_tweets::tweet{
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

BENCHMARK_TEMPLATE(PartialTweets, Dom);

} // namespace partial_tweets

#endif // SIMDJSON_EXCEPTIONS