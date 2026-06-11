#pragma once

#if SIMDJSON_EXCEPTIONS

#include "find_tweet.h"

namespace find_tweet {

using namespace simdjson;

struct simdjson_ondemand {
  using StringType=std::string_view;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, uint64_t find_id, std::string_view &result) {
    // Walk the document, parsing as we go
    auto doc = parser.iterate(json);
    for (auto tweet : doc.find_field("statuses")) {
      if (uint64_t(tweet.find_field("id")) == find_id) {
        result = tweet.find_field("text");
        return true;
      }
    }
    return false;
  }
};

BENCHMARK_TEMPLATE(find_tweet, simdjson_ondemand)->UseManualTime();

// Same workload, but parsed with iterate_unpadded (bounds-safe value parsers for
// input without SIMDJSON_PADDING). Run on the same buffer so the measured
// difference is purely the cost of the unpadded code paths.
struct simdjson_ondemand_unpadded {
  using StringType=std::string_view;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, uint64_t find_id, std::string_view &result) {
    auto doc = parser.iterate_unpadded(json.data(), json.size());
    for (auto tweet : doc.find_field("statuses")) {
      if (uint64_t(tweet.find_field("id")) == find_id) {
        result = tweet.find_field("text");
        return true;
      }
    }
    return false;
  }
};

BENCHMARK_TEMPLATE(find_tweet, simdjson_ondemand_unpadded)->UseManualTime();

} // namespace find_tweet

#endif // SIMDJSON_EXCEPTIONS
