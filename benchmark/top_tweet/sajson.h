#pragma once

#ifdef SIMDJSON_COMPETITION_SAJSON

#include "top_tweet.h"

namespace top_tweet {

struct sajson {
  using StringType=std::string_view;

  size_t ast_buffer_size{0};
  size_t *ast_buffer{nullptr};
  ~sajson() { free(ast_buffer); }

  bool run(simdjson::padded_string &json, int32_t max_retweet_count, top_tweet_result<StringType> &result) {
    if (!ast_buffer) {
      ast_buffer_size = json.size();
      ast_buffer = (size_t *)std::malloc(ast_buffer_size * sizeof(size_t));
    }
    auto doc = ::sajson::parse(
      ::sajson::bounded_allocation(ast_buffer, ast_buffer_size),
      ::sajson::mutable_string_view(json.size(), json.data())
    );
    if (!doc.is_valid()) { return false; }

    auto root = doc.get_root();
    if (root.get_type() != ::sajson::TYPE_OBJECT) { return false; }
    auto statuses = root.get_value_of_key({ "statuses", strlen("statuses") });
    if (statuses.get_type() != ::sajson::TYPE_ARRAY) { return false; }

    for (size_t i=0; i<statuses.get_length(); i++) {
      auto tweet = statuses.get_array_element(i);
      if (tweet.get_type() != ::sajson::TYPE_OBJECT) { return false; }

      // We can't keep a copy of "value" around, so AFAICT we can't lazily parse
      auto retweet_count_val = tweet.get_value_of_key({ "retweet_count", strlen("retweet_count") });
      if (retweet_count_val.get_type() != ::sajson::TYPE_INTEGER) { return false; }
      int32_t retweet_count = retweet_count_val.get_integer_value();
      if (retweet_count <= max_retweet_count && retweet_count >= result.retweet_count) {
        result.retweet_count = retweet_count;

        auto text = tweet.get_value_of_key({ "text", strlen("text") });
        if (text.get_type() != ::sajson::TYPE_STRING) { return false; }
        result.text = { text.as_cstring(), text.get_string_length() };

        auto user = tweet.get_value_of_key({ "user", strlen("user") });
        if (user.get_type() != ::sajson::TYPE_OBJECT) { return false; }
        auto screen_name = user.get_value_of_key({ "screen_name", strlen("screen_name") });
        if (screen_name.get_type() != ::sajson::TYPE_STRING) { return false; }
        result.screen_name = { screen_name.as_cstring(), screen_name.get_string_length() };
      }
    }

    return result.retweet_count != -1;
  }
};

BENCHMARK_TEMPLATE(top_tweet, sajson)->UseManualTime();

} // namespace top_tweet

#endif // SIMDJSON_COMPETITION_SAJSON