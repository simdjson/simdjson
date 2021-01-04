#pragma once

#ifdef SIMDJSON_COMPETITION_YYJSON

#include "distinct_user_id.h"

namespace distinct_user_id {

class yyjson {
public:
  bool run(const simdjson::padded_string &json, std::vector<uint64_t> &ids) {
    // Walk the document, parsing the tweets as we go
    yyjson_doc *doc = yyjson_read(json.data(), json.size(), 0);
    if (!doc) { return false; }
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *statuses = yyjson_obj_get(root, "statuses");

    size_t tweet_idx, tweets_max;
    yyjson_val *tweet;
    yyjson_arr_foreach(statuses, tweet_idx, tweets_max, tweet) {
      auto user = yyjson_obj_get(tweet, "user");
      auto id = yyjson_obj_get(user, "id");
      ids.push_back(yyjson_get_sint(id));

      // Not all tweets have a "retweeted_status", but when they do
      // we want to go and find the user within.
      auto retweet = yyjson_obj_get(tweet, "retweeted_status");
      if (retweet) {
        user = yyjson_obj_get(retweet, "user");
        id = yyjson_obj_get(user, "id");
        ids.push_back(yyjson_get_sint(id));
      }
    }

    return true;
  }

};

BENCHMARK_TEMPLATE(distinct_user_id, yyjson);

} // namespace distinct_user_id

#endif // SIMDJSON_COMPETITION_YYJSON
