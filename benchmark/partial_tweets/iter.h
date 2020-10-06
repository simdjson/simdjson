#pragma once

#if SIMDJSON_EXCEPTIONS

#include "partial_tweets.h"

namespace partial_tweets {

using namespace simdjson;
using namespace simdjson::builtin;

class Iter {
public:
  simdjson_really_inline bool Run(const padded_string &json);

  simdjson_really_inline const std::vector<tweet> &Result() { return tweets; }
  simdjson_really_inline size_t ItemCount() { return tweets.size(); }

private:
  ondemand::parser parser{};
  std::vector<tweet> tweets{};

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

simdjson_really_inline bool Iter::Run(const padded_string &json) {
  tweets.clear();

  // Walk the document, parsing the tweets as we go

  // { "statuses": 
  auto iter = parser.iterate_raw(json).value();
  if (!iter.start_object()   || !iter.find_field_raw("statuses")) { return false; }
  // { "statuses": [
  if (!iter.start_array()) { return false; }

  do {
    tweet tweet;

    if (!iter.start_object()   || !iter.find_field_raw("created_at")) { return false; }
    tweet.created_at = iter.consume_string();

    if (!iter.has_next_field() || !iter.find_field_raw("id")) { return false; }
    tweet.id = iter.consume_uint64();

    if (!iter.has_next_field() || !iter.find_field_raw("text")) { return false; }
    tweet.text = iter.consume_string();

    if (!iter.has_next_field() || !iter.find_field_raw("in_reply_to_status_id")) { return false; }
    if (!iter.is_null()) {
      tweet.in_reply_to_status_id = iter.consume_uint64();
    }

    if (!iter.has_next_field() || !iter.find_field_raw("user")) { return false; }
    {
      if (!iter.start_object()   || !iter.find_field_raw("id")) { return false; }
      tweet.user.id = iter.consume_uint64();

      if (!iter.has_next_field() || !iter.find_field_raw("screen_name")) { return false; }
      tweet.user.screen_name = iter.consume_string();

      if (iter.skip_container()) { return false; } // Skip the rest of the user object
    }

    if (!iter.has_next_field() || !iter.find_field_raw("retweet_count")) { return false; }
    tweet.retweet_count = iter.consume_uint64();

    if (!iter.has_next_field() || !iter.find_field_raw("favorite_count")) { return false; }
    tweet.favorite_count = iter.consume_uint64();

    tweets.push_back(tweet);

    if (iter.skip_container()) { return false; } // Skip the rest of the tweet object

  } while (iter.has_next_element());

  return true;
}

BENCHMARK_TEMPLATE(PartialTweets, Iter);

} // namespace partial_tweets

#endif // SIMDJSON_EXCEPTIONS
