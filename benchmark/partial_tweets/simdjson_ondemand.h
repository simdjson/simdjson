#pragma once

#if SIMDJSON_EXCEPTIONS

#include "partial_tweets.h"

namespace partial_tweets {

using namespace simdjson;

struct simdjson_ondemand {
  using StringType=std::string_view;

  ondemand::parser parser{};

  simdjson_inline uint64_t nullable_int(ondemand::value value) {
    if (value.is_null()) { return 0; }
    return value;
  }

  simdjson_inline twitter_user<std::string_view> read_user(ondemand::object user) {
    return { user.find_field("id"), user.find_field("screen_name") };
  }

  bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
    // Walk the document, parsing the tweets as we go
    auto doc = parser.iterate(json);
    for (ondemand::object tweet : doc.find_field("statuses")) {
      result.emplace_back(partial_tweets::tweet<std::string_view>{
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
};

BENCHMARK_TEMPLATE(partial_tweets, simdjson_ondemand)->UseManualTime();

// Same workload, but parsed with iterate_unpadded (bounds-safe value parsers for
// input without SIMDJSON_PADDING). Exercises strings, integers and key matching
// on the unpadded code paths. Run on the same buffer so the measured difference is
// purely the cost of those paths.
struct simdjson_ondemand_unpadded {
  using StringType=std::string_view;

  ondemand::parser parser{};

  simdjson_inline uint64_t nullable_int(ondemand::value value) {
    if (value.is_null()) { return 0; }
    return value;
  }

  simdjson_inline twitter_user<std::string_view> read_user(ondemand::object user) {
    return { user.find_field("id"), user.find_field("screen_name") };
  }

  bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
    auto doc = parser.iterate_unpadded(json.data(), json.size());
    for (ondemand::object tweet : doc.find_field("statuses")) {
      result.emplace_back(partial_tweets::tweet<std::string_view>{
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
};

BENCHMARK_TEMPLATE(partial_tweets, simdjson_ondemand_unpadded)->UseManualTime();

} // namespace partial_tweets

#endif // SIMDJSON_EXCEPTIONS
