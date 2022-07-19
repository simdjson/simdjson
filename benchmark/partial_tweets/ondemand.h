#pragma once

#if SIMDJSON_EXCEPTIONS

#include "partial_tweets.h"

namespace partial_tweets {

using namespace simdjson;


class OnDemand {
public:
  OnDemand() {
    if(!displayed_implementation) {
      std::cout << "On Demand implementation: " << builtin_implementation()->name() << std::endl;
      displayed_implementation = true;
    }
  }
  simdjson_inline bool Run(const padded_string &json);
  simdjson_inline const std::vector<tweet> &Result() { return tweets; }
  simdjson_inline size_t ItemCount() { return tweets.size(); }

private:
  ondemand::parser parser{};
  std::vector<tweet> tweets{};

  simdjson_inline uint64_t nullable_int(ondemand::value value) {
    if (value.is_null()) { return 0; }
    return value;
  }

  simdjson_inline twitter_user read_user(ondemand::object user) {
    return { user.find_field("id"), user.find_field("screen_name") };
  }

  static inline bool displayed_implementation = false;
};

simdjson_inline bool OnDemand::Run(const padded_string &json) {
  tweets.clear();

  // Walk the document, parsing the tweets as we go
  auto doc = parser.iterate(json);
  for (ondemand::object tweet : doc.find_field("statuses")) {
    tweets.emplace_back(partial_tweets::tweet{
      tweet.find_field("created_at"),
      tweet.find_field("id"),
      tweet.find_field("text"),
      nullable_int(tweet.find_field("in_reply_to_status_id")),
      read_user(tweet.find_field("user")),
      tweet.find_field("retweet_count"),
      tweet.find_field("favorite_count")
    });
  }
  return true;
}

BENCHMARK_TEMPLATE(PartialTweets, OnDemand);

} // namespace partial_tweets

#endif // SIMDJSON_EXCEPTIONS
