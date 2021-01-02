#pragma once

#include "partial_tweets.h"

namespace partial_tweets {

class Yyjson {
public:
  simdjson_really_inline const std::vector<tweet> &Result() { return tweets; }
  simdjson_really_inline size_t ItemCount() { return tweets.size(); }

private:
  std::vector<tweet> tweets{};

  simdjson_really_inline std::string_view get_string_view(yyjson_val *obj, std::string_view key) {
    auto val = yyjson_obj_getn(obj, key.data(), key.length());
    return { yyjson_get_str(val), yyjson_get_len(val) };
  }
  simdjson_really_inline uint64_t get_uint64(yyjson_val *obj, std::string_view key) {
    auto val = yyjson_obj_getn(obj, key.data(), key.length());
    return yyjson_get_uint(val);
  }
  simdjson_really_inline partial_tweets::twitter_user get_user(yyjson_val *obj, std::string_view key) {
    auto user = yyjson_obj_getn(obj, key.data(), key.length());
    return { get_uint64(user, "id"), get_string_view(user, "screen_name") };
  }

public:
  simdjson_really_inline bool Run(const padded_string &json) {
    tweets.clear();

    // Walk the document, parsing the tweets as we go
    yyjson_doc *doc = yyjson_read(json.data(), json.size(), 0);
    if (!doc) { return false; }
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *statuses = yyjson_obj_get(root, "statuses");

    size_t tweet_idx, tweets_max;
    yyjson_val *tweet;
    yyjson_arr_foreach(statuses, tweet_idx, tweets_max, tweet) {
      // TODO these can't actually handle errors
      tweets.emplace_back(partial_tweets::tweet{
        get_string_view(tweet, "created_at"),
        get_uint64     (tweet, "id"),
        get_string_view(tweet, "text"),
        get_uint64     (tweet, "in_reply_to_status_id"),
        get_user       (tweet, "user"),
        get_uint64     (tweet, "retweet_count"),
        get_uint64     (tweet, "favorite_count")
      });
    }
    return true;
  }

};

BENCHMARK_TEMPLATE(PartialTweets, Yyjson);

} // namespace partial_tweets
