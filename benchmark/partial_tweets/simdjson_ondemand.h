#pragma once

#if SIMDJSON_EXCEPTIONS

#include "partial_tweets.h"

namespace partial_tweets {

using namespace simdjson;

struct simdjson_ondemand {
  using StringType = std::string_view;

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


#if SIMDJSON_SUPPORTS_EXTRACT

using namespace simdjson::ondemand;

struct simdjson_ondemand_extract {
  using StringType = std::string_view;
  ondemand::parser parser{};
  bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
    // Walk the document, parsing the tweets as we go
    auto doc = parser.iterate(json);
    for (ondemand::object tweet_object : doc.find_field("statuses")) {
      tweet<std::string_view> t;
      auto error = tweet_object.extract(
        to{"created_at", t.created_at},
        to{"id", t.id},
        to{"text", t.result},
        to{"in_reply_to_status_id", [&t](auto val) {
          if(val.is_null()) {
            t.in_reply_to_status_id = 0;
          } else {
            t.in_reply_to_status_id = val;
          }
         }},
        to{"user", sub{
          to{"id", t.user.id},
          to{"screen_name", t.user.screen_name},
        }},
        to{"retweet_count", t.retweet_count},
        to{"favorite_count", t.favorite_count});
        if(error) { throw simdjson_error(error); }
        result.push_back(t);
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(partial_tweets, simdjson_ondemand_extract)->UseManualTime();


#endif

} // namespace partial_tweets

#endif // SIMDJSON_EXCEPTIONS
