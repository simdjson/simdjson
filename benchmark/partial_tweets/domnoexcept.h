#pragma once

#include "partial_tweets.h"

namespace partial_tweets {

using namespace simdjson;

class DomNoExcept {
public:
  simdjson_really_inline bool Run(const simdjson::padded_string &json) noexcept;

  simdjson_really_inline const std::vector<tweet> &Result() { return tweets; }
  simdjson_really_inline size_t ItemCount() { return tweets.size(); }

private:
  dom::parser parser{};
  std::vector<tweet> tweets{};

  simdjson_really_inline simdjson_result<uint64_t> nullable_int(simdjson_result<dom::element> result) noexcept {
    dom::element element;
    SIMDJSON_TRY( result.get(element) );
    if (element.is_null()) { return 0; }
    return element.get_uint64();
  }

  simdjson_really_inline error_code RunNoExcept(const simdjson::padded_string &json) noexcept;
};

simdjson_really_inline bool DomNoExcept::Run(const simdjson::padded_string &json) noexcept {
  auto error = RunNoExcept(json);
  if (error) { std::cerr << error << std::endl; return false; }
  return true;
}

simdjson_really_inline error_code DomNoExcept::RunNoExcept(const simdjson::padded_string &json) noexcept {
  tweets.clear();

  dom::array tweet_array;
  SIMDJSON_TRY( parser.parse(json)["statuses"].get_array().get(tweet_array) );

  for (auto tweet_element : tweet_array) {
    dom::object tweet;
    SIMDJSON_TRY( tweet_element.get_object().get(tweet) );

    dom::object user;
    SIMDJSON_TRY( tweet["user"].get_object().get(user) );

    partial_tweets::tweet t;
    SIMDJSON_TRY( tweet["created_at"]    .get_string().get(t.created_at) );
    SIMDJSON_TRY( tweet["id"]            .get_uint64().get(t.id) );
    SIMDJSON_TRY( tweet["text"]          .get_string().get(t.text) );
    SIMDJSON_TRY( nullable_int(tweet["in_reply_to_status_id"]).get(t.in_reply_to_status_id) );
    SIMDJSON_TRY( user["id"]             .get_uint64().get(t.user.id) );
    SIMDJSON_TRY( user["screen_name"]    .get_string().get(t.user.screen_name) );
    SIMDJSON_TRY( tweet["retweet_count"] .get_uint64().get(t.retweet_count) );
    SIMDJSON_TRY( tweet["favorite_count"].get_uint64().get(t.favorite_count) );

    tweets.push_back(t);
  }
  return SUCCESS;
}

} // namespace partial_tweets
