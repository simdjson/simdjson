#pragma once

#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS

#include "partial_tweets.h"

namespace partial_tweets {

using namespace simdjson;

struct simdjson_ondemand_key_selector {
  using StringType = std::string_view;

  ondemand::parser parser{};

  // Compile-time selectors — all PHF tables are static constexpr, so every
  // call to match_raw fully inlines.
  using tweet_sel_t = ondemand::key_selector<
      "created_at", "id", "text", "in_reply_to_status_id",
      "user", "retweet_count", "favorite_count">;
  using user_sel_t = ondemand::key_selector<"id", "screen_name">;

  simdjson_inline uint64_t nullable_int(ondemand::value value) {
    if (value.is_null()) { return 0; }
    return value;
  }

  simdjson_inline twitter_user<std::string_view> read_user(ondemand::object user) {
    twitter_user<std::string_view> out{};
    // Bind the two scalar fields straight to the struct members. Input is known
    // valid here, so we discard the result; a real caller would check it.
    (void) user.for_each<user_sel_t>(out.id, out.screen_name);
    return out;
  }

  bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
    auto doc = parser.iterate(json);
    for (ondemand::object tw : doc.find_field("statuses")) {
      tweet<std::string_view> t{};
      // Scalar/string fields bind directly to members; the two fields that need
      // custom logic (a nullable int and a nested object) use a lambda. Input is
      // known valid here, so we discard the result; a real caller would check it.
      (void) tw.for_each<tweet_sel_t>(
        t.created_at,
        t.id,
        t.result,
        [&](ondemand::value v){ t.in_reply_to_status_id  = nullable_int(v); },
        [&](ondemand::value v){ t.user                   = read_user(v); },
        t.retweet_count,
        t.favorite_count
      );
      result.emplace_back(std::move(t));
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(partial_tweets, simdjson_ondemand_key_selector)->UseManualTime();

} // namespace partial_tweets

#endif // SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
