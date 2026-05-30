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
    user.for_each<user_sel_t>([&](std::size_t i, ondemand::value v) {
      switch (i) {
        case 0: out.id          = uint64_t(v);         break; // "id"
        case 1: out.screen_name = std::string_view(v); break; // "screen_name"
      }
    });
    return out;
  }

  bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
    auto doc = parser.iterate(json);
    for (ondemand::object tw : doc.find_field("statuses")) {
      tweet<std::string_view> t{};
      tw.for_each<tweet_sel_t>([&](std::size_t i, ondemand::value v) {
        switch (i) {
          case 0: t.created_at             = std::string_view(v);    break; // "created_at"
          case 1: t.id                     = uint64_t(v);            break; // "id"
          case 2: t.result                 = std::string_view(v);    break; // "text"
          case 3: t.in_reply_to_status_id  = nullable_int(v);        break; // "in_reply_to_status_id"
          case 4: t.user                   = read_user(v);           break; // "user"
          case 5: t.retweet_count          = uint64_t(v);            break; // "retweet_count"
          case 6: t.favorite_count         = uint64_t(v);            break; // "favorite_count"
        }
      });
      result.emplace_back(std::move(t));
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(partial_tweets, simdjson_ondemand_key_selector)->UseManualTime();

} // namespace partial_tweets

#endif // SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
