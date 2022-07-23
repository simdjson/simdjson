#pragma once

#ifdef SIMDJSON_COMPETITION_YYJSON

#include "find_tweet.h"

namespace find_tweet {

struct yyjson_base {
  using StringType=std::string_view;

  bool run(yyjson_doc *doc, uint64_t find_id, std::string_view &result) {
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
      auto id = yyjson_obj_get(tweet, "id");
      if (!yyjson_is_uint(id)) { return false; }
      if (yyjson_get_uint(id) == find_id) {
        auto text = yyjson_obj_get(tweet, "text");
        if (yyjson_is_str(id)) { return false; }
        result = { yyjson_get_str(text), yyjson_get_len(text) };
        return true;
      }
    }
    return false;
  }
};

struct yyjson : yyjson_base {
  bool run(simdjson::padded_string &json, uint64_t find_id, std::string_view &result) {
    return yyjson_base::run(yyjson_read(json.data(), json.size(), 0), find_id, result);
  }
};
BENCHMARK_TEMPLATE(find_tweet, yyjson)->UseManualTime();
#if SIMDJSON_COMPETITION_ONDEMAND_INSITU
struct yyjson_insitu : yyjson_base {
  bool run(simdjson::padded_string &json, uint64_t find_id, std::string_view &result) {
    return yyjson_base::run(yyjson_read_opts(json.data(), json.size(), YYJSON_READ_INSITU, 0, 0), find_id, result);
  }
};
BENCHMARK_TEMPLATE(find_tweet, yyjson_insitu)->UseManualTime();
#endif // SIMDJSON_COMPETITION_ONDEMAND_INSITU
} // namespace find_tweet

#endif // SIMDJSON_COMPETITION_YYJSON
