#pragma once

#if SIMDJSON_EXCEPTIONS

#include "top_tweet.h"

namespace top_tweet {

using namespace simdjson;

struct simdjson_ondemand {
  using StringType=std::string_view;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    result.retweet_count = -1;
    // We save these DOM values for later so we don't have to parse them
    // into string_views until we're sure which ones we want to parse
    // NOTE: simdjson does not presently support reuse of objects or arrays--just scalars. This is
    // why we have to grab the text and screen_name fields instead of just saving the tweet object.
    ondemand::value screen_name, text;

    auto doc = parser.iterate(json);
    for (auto tweet : doc["statuses"]) {
      // Since text, user.screen_name, and retweet_count generally appear in order, it's nearly free
      // for us to retrieve them here (and will cost a bit more if we do it in the if
      // statement).
      auto tweet_text = tweet["text"];
      auto tweet_screen_name = tweet["user"]["screen_name"];
      int64_t retweet_count = tweet["retweet_count"];
      if (retweet_count <= max_retweet_count && retweet_count >= result.retweet_count) {
        result.retweet_count = retweet_count;
        // TODO std::move should not be necessary
        text = std::move(tweet_text);
        screen_name = std::move(tweet_screen_name);
      }
    }

    // Now that we know which was the most retweeted, parse the values in it
    result.screen_name = screen_name;
    result.text = text;
    return result.retweet_count != -1;
  }
};

BENCHMARK_TEMPLATE(top_tweet, simdjson_ondemand)->UseManualTime();

struct simdjson_ondemand_forward_only {
  using StringType=std::string_view;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    result.retweet_count = -1;

    auto doc = parser.iterate(json);
    for (auto tweet : doc["statuses"]) {
      // Since text, user.screen_name, and retweet_count generally appear in order, it's nearly free
      // for us to retrieve them here (and will cost a bit more if we do it in the if
      // statement).
      auto tweet_text = tweet["text"];
      auto tweet_screen_name = tweet["user"]["screen_name"];
      int64_t retweet_count = tweet["retweet_count"];
      if (retweet_count <= max_retweet_count && retweet_count >= result.retweet_count) {
        result.retweet_count = retweet_count;
        result.text = tweet_text;
        result.screen_name = tweet_screen_name;
      }
    }

    return result.retweet_count != -1;
  }
};

BENCHMARK_TEMPLATE(top_tweet, simdjson_ondemand_forward_only)->UseManualTime();

} // namespace top_tweet

#endif // SIMDJSON_EXCEPTIONS
