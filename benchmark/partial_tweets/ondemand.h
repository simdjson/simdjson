#pragma once

#ifdef SIMDJSON_IMPLEMENTATION
#if SIMDJSON_EXCEPTIONS

#include "partial_tweets.h"

namespace partial_tweets {

using namespace simdjson;
using namespace SIMDJSON_IMPLEMENTATION;
using namespace SIMDJSON_IMPLEMENTATION::stage2;

class OnDemand {
public:
  simdjson_really_inline bool Run(const padded_string &json);
  simdjson_really_inline bool SetUp() { return true; }
  simdjson_really_inline bool TearDown() { return true; }
  simdjson_really_inline const std::vector<tweet> &Records() { return tweets; }

private:
  ondemand::parser parser;
  std::vector<tweet> tweets;

  simdjson_really_inline uint64_t nullable_int(ondemand::value && value) {
    if (value.is_null()) { return 0; }
    return std::move(value);
  }

  simdjson_really_inline twitter_user read_user(ondemand::object && user) {
    // Move user into a local object so it gets destroyed (and moves the iterator)
    ondemand::object u = std::move(user);
    return { u["id"], u["screen_name"] };
  }
};

simdjson_really_inline bool OnDemand::Run(const padded_string &json) {
  tweets.clear();

  // Walk the document, parsing the tweets as we go
  auto doc = parser.iterate(json);
  auto root = doc.get_object();
  ondemand::array statuses = root["statuses"];
  for (ondemand::object tweet : statuses) {
    tweets.emplace_back(partial_tweets::tweet{
      tweet["created_at"],
      tweet["id"],
      tweet["text"],
      nullable_int(tweet["in_reply_to_status_id"]),
      read_user(tweet["user"]),
      tweet["retweet_count"],
      tweet["favorite_count"]
    });
  }
  return true;
}

BENCHMARK_TEMPLATE(PartialTweets, OnDemand);

} // namespace partial_tweets

#endif // SIMDJSON_EXCEPTIONS
#endif // SIMDJSON_IMPLEMENTATION
