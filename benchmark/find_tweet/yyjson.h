#pragma once

#ifdef SIMDJSON_COMPETITION_YYJSON

#include "find_tweet.h"

namespace find_tweet {

struct yyjson {
  bool run(const simdjson::padded_string &json, uint64_t find_id, std::string_view &text) {
    // Walk the document, parsing the tweets as we go
    yyjson_doc *doc = yyjson_read(json.data(), json.size(), 0);
    if (!doc) { return false; }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) { return false; }
    yyjson_val *statuses = yyjson_obj_get(root, "statuses");
    if (!yyjson_is_arr(statuses)) { return "Statuses is not an array!"; }

    size_t tweet_idx, tweets_max;
    yyjson_val *tweet;
    yyjson_arr_foreach(statuses, tweet_idx, tweets_max, tweet) {
      if (!yyjson_is_obj(tweet)) { return false; }
      auto id = yyjson_obj_get(tweet, "id");
      if (!yyjson_is_uint(id)) { return false; }
      if (yyjson_get_uint(id) == find_id) {
        auto _text = yyjson_obj_get(tweet, "text");
        if (yyjson_is_str(id)) { return false; }
        text = yyjson_get_str(_text);
        return true;
      }
    }
    return false;
  }
};

BENCHMARK_TEMPLATE(find_tweet, yyjson);

} // namespace find_tweet

#endif // SIMDJSON_COMPETITION_YYJSON
