#pragma once

#ifdef SIMDJSON_COMPETITION_YYJSON

#include "top_tweet.h"

namespace top_tweet {

struct yyjson_base {
  using StringType=std::string_view;

  bool run(yyjson_doc *doc, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    result.retweet_count = -1;

    yyjson_val *top_tweet{};

    if (!doc) { return false; }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) { return false; }
    yyjson_val *statuses = yyjson_obj_get(root, "statuses");
    if (!yyjson_is_arr(statuses)) { return false; }

    // Walk the document, parsing the tweets as we go
    size_t tweet_idx, tweets_max;
    yyjson_val *tweet;
    yyjson_arr_foreach(statuses, tweet_idx, tweets_max, tweet) {
      if (!yyjson_is_obj(tweet)) { return false; }

      auto retweet_count_val = yyjson_obj_get(tweet, "retweet_count");
      if (!yyjson_is_uint(retweet_count_val)) { return false; }
      int64_t retweet_count = yyjson_get_uint(retweet_count_val);
      if (retweet_count <= max_retweet_count && retweet_count >= result.retweet_count) {
        result.retweet_count = retweet_count;
        top_tweet = tweet;
      }
    }

    auto text = yyjson_obj_get(top_tweet, "text");
    if (!yyjson_is_str(text)) { return false; }
    result.text = { yyjson_get_str(text), yyjson_get_len(text) };

    auto user = yyjson_obj_get(top_tweet, "user");
    if (!yyjson_is_obj(user)) { return false; }
    auto screen_name = yyjson_obj_get(user, "screen_name");
    if (!yyjson_is_str(screen_name)) { return false; }
    result.screen_name = { yyjson_get_str(screen_name), yyjson_get_len(screen_name) };

    return result.retweet_count != -1;
  }
};

struct yyjson : yyjson_base {
  bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    return yyjson_base::run(yyjson_read(json.data(), json.size(), 0), max_retweet_count, result);
  }
};
BENCHMARK_TEMPLATE(top_tweet, yyjson)->UseManualTime();
#if SIMDJSON_COMPETITION_ONDEMAND_INSITU
struct yyjson_insitu : yyjson_base {
  bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    return yyjson_base::run(yyjson_read_opts(json.data(), json.size(), YYJSON_READ_INSITU, 0, 0), max_retweet_count, result);
  }
};
BENCHMARK_TEMPLATE(top_tweet, yyjson_insitu)->UseManualTime();
#endif // SIMDJSON_COMPETITION_ONDEMAND_INSITU
} // namespace top_tweet

#endif // SIMDJSON_COMPETITION_YYJSON
