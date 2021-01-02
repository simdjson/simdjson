#pragma once

#include "find_tweet.h"

namespace find_tweet {

class Yyjson {
public:
  simdjson_really_inline std::string_view Result() { return text; }
  simdjson_really_inline size_t ItemCount() { return 1; }

private:
  std::string_view text{};

public:
  simdjson_really_inline bool Run(const padded_string &json) {
    text = "";

    // Walk the document, parsing the tweets as we go
    yyjson_doc *doc = yyjson_read(json.data(), json.size(), 0);
    if (!doc) { return false; }
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *statuses = yyjson_obj_get(root, "statuses");

    size_t tweet_idx, tweets_max;
    yyjson_val *tweet;
    yyjson_arr_foreach(statuses, tweet_idx, tweets_max, tweet) {
      auto id = yyjson_obj_get(tweet, "id");
      if (yyjson_get_uint(id) == TWEET_ID) {
        auto _text = yyjson_obj_get(tweet, "text");
        text = yyjson_get_str(_text);
        return true;
      }
    }
    return false;
  }
};

BENCHMARK_TEMPLATE(FindTweet, Yyjson);

} // namespace find_tweet
