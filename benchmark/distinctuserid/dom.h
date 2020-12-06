#pragma once

#if SIMDJSON_EXCEPTIONS

#include "distinctuserid.h"

namespace distinct_user_id {

using namespace simdjson;

class Dom {
public:
  simdjson_really_inline bool Run(const padded_string &json);
  simdjson_really_inline const std::vector<int64_t> &Result() { return ids; }
  simdjson_really_inline size_t ItemCount() { return ids.size(); }

private:
  dom::parser parser{};
  std::vector<int64_t> ids{};
};

simdjson_really_inline bool Dom::Run(const padded_string &json) {
  ids.clear();
  // Walk the document, parsing as we go
  auto doc = parser.parse(json);
  for (dom::object tweet : doc["statuses"]) {
      // We believe that all statuses have a matching
      // user, and we are willing to throw when they do not.
      ids.push_back(tweet["user"]["id"]);
      // Not all tweets have a "retweeted_status", but when they do
      // we want to go and find the user within.
      auto retweet = tweet["retweeted_status"];
      if(retweet.error() != NO_SUCH_FIELD) {
          ids.push_back(retweet["user"]["id"]);
      }
  }
  remove_duplicates(ids);
  return true;
}

BENCHMARK_TEMPLATE(DistinctUserID, Dom);

} // namespace distinct_user_id

#endif // SIMDJSON_EXCEPTIONS