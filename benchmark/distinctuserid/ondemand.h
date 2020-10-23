#pragma once

#if SIMDJSON_EXCEPTIONS

#include "distinctuserid.h"

namespace distinct_user_id {

using namespace simdjson;
using namespace simdjson::builtin;


class OnDemand {
public:
  OnDemand() { 
    if(!displayed_implementation) {
      std::cout << "On Demand implementation: " << builtin_implementation()->name() << std::endl; 
      displayed_implementation = true;
    }
  } 
  simdjson_really_inline bool Run(const padded_string &json);
  simdjson_really_inline const std::vector<int64_t> &Result() { return ids; }
  simdjson_really_inline size_t ItemCount() { return ids.size(); }

private:
  ondemand::parser parser{};
  std::vector<int64_t> ids{};

  static inline bool displayed_implementation = false;
};

simdjson_really_inline bool OnDemand::Run(const padded_string &json) {
  ids.clear();
  // Walk the document, parsing as we go
  auto doc = parser.iterate(json);
  for (ondemand::object tweet : doc["statuses"]) {
      // We believe that all statuses have a matching
      // user, and we are willing to throw when they do not:
      //
      // You might think that you do not need the braces, but
      // you do, otherwise you will get the wrong answer. That is
      // because you can only have one active object or array 
      // at a time. 
      {
        ondemand::object user = tweet["user"];
        int64_t id = user["id"]; 
        ids.push_back(id);
      }
      // Not all tweets have a "retweeted_status", but when they do 
      // we want to go and find the user within.
      auto retweet = tweet["retweeted_status"];
      if(!retweet.error()) {
          ondemand::object retweet_content = retweet;
          ondemand::object reuser = retweet_content["user"];
          int64_t rid = reuser["id"]; 
          ids.push_back(rid);
      }
  }
  remove_duplicates(ids);
  return true;
}

BENCHMARK_TEMPLATE(DistinctUserID, OnDemand);

} // namespace distinct_user_id

#endif // SIMDJSON_EXCEPTIONS
