#pragma once

#if SIMDJSON_EXCEPTIONS

#include "find_tweet.h"

namespace find_tweet {

using namespace simdjson;
using namespace simdjson::builtin;

class simdjson_ondemand {
  ondemand::parser parser{};
public:
  bool run(const simdjson::padded_string &json, uint64_t find_id, std::string_view &text) {
    // Walk the document, parsing as we go
    auto doc = parser.iterate(json);
    for (auto tweet : doc.find_field("statuses")) {
      if (uint64_t(tweet.find_field("id")) == find_id) {
        text = tweet.find_field("text");
        return true;
      }
    }
    return false;
  }
};

BENCHMARK_TEMPLATE(find_tweet, simdjson_ondemand);

} // namespace find_tweet

#endif // SIMDJSON_EXCEPTIONS
