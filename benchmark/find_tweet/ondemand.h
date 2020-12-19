#pragma once

#if SIMDJSON_EXCEPTIONS

#include "find_tweet.h"

namespace find_tweet {

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
  simdjson_really_inline std::string_view Result() { return text; }
  simdjson_really_inline size_t ItemCount() { return 1; }

private:
  ondemand::parser parser{};
  std::string_view text{};

  static inline bool displayed_implementation = false;
};

simdjson_really_inline bool OnDemand::Run(const padded_string &json) {
  text = "";
  // Walk the document, parsing as we go
  auto doc = parser.iterate(json);
  for (ondemand::object tweet : doc.find_field("statuses")) {
      if (uint64_t(tweet.find_field("id")) == TWEET_ID) {
        text = tweet.find_field("text");
        return true;
      }
  }
  return false;
}

BENCHMARK_TEMPLATE(FindTweet, OnDemand);

} // namespace find_tweet

#endif // SIMDJSON_EXCEPTIONS
