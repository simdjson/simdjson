#pragma once

#ifdef SIMDJSON_COMPETITION_YYJSON

#include "distinct_user_id.h"

namespace distinct_user_id {

struct yyjson_base {
  bool run(yyjson_doc *doc, std::vector<uint64_t> &result) {
    if (!doc) { return false; }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) { return false; }
    yyjson_val *statuses = yyjson_obj_get(root, "statuses");
    if (!yyjson_is_arr(statuses)) { return false; }

    // Walk the document, parsing the tweets as we go
    size_t tweet_idx, tweets_max;
    yyjson_val *tweet;
    yyjson_arr_foreach(statuses, tweet_idx, tweets_max, tweet) {
      auto user = yyjson_obj_get(tweet, "user");
      if (!yyjson_is_obj(user)) { return false; }
      auto id = yyjson_obj_get(user, "id");
      if (!yyjson_is_uint(id)) { return false; }
      result.push_back(yyjson_get_uint(id));

      // Not all tweets have a "retweeted_status", but when they do
      // we want to go and find the user within.
      auto retweet = yyjson_obj_get(tweet, "retweeted_status");
      if (retweet) {
        if (!yyjson_is_obj(retweet)) { return false; }
        user = yyjson_obj_get(retweet, "user");
        if (!yyjson_is_obj(user)) { return false; }
        id = yyjson_obj_get(user, "id");
        if (!yyjson_is_uint(id)) { return false; }
        result.push_back(yyjson_get_sint(id));
      }
    }

    return true;
  }

};

struct yyjson : yyjson_base {
  bool run(simdjson::padded_string &json, std::vector<uint64_t> &result) {
    return yyjson_base::run(yyjson_read(json.data(), json.size(), 0), result);
  }
};
BENCHMARK_TEMPLATE(distinct_user_id, yyjson)->UseManualTime();

#if SIMDJSON_COMPETITION_ONDEMAND_INSITU
struct yyjson_insitu : yyjson_base {
  bool run(simdjson::padded_string &json, std::vector<uint64_t> &result) {
    return yyjson_base::run(yyjson_read_opts(json.data(), json.size(), YYJSON_READ_INSITU, 0, 0), result);
  }
};
BENCHMARK_TEMPLATE(distinct_user_id, yyjson_insitu)->UseManualTime();
#endif // SIMDJSON_COMPETITION_ONDEMAND_INSITU
} // namespace distinct_user_id

#endif // SIMDJSON_COMPETITION_YYJSON
