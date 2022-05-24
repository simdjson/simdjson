#pragma once

#if SIMDJSON_EXCEPTIONS

#include "amazon_cellphones.h"

namespace amazon_cellphones {

using namespace simdjson;

template<bool threaded>
struct simdjson_dom {
  using StringType = std::string;

  dom::parser parser{};

  bool run(simdjson::padded_string &json, std::map<StringType, brand> &result) {
#ifdef SIMDJSON_THREADS_ENABLED
    parser.threaded = threaded;
#endif
    auto stream = parser.parse_many(json);
    auto i = stream.begin();
    ++i;  // Skip first line
    for (;i != stream.end(); ++i) {
      auto doc = *i;
      StringType copy(std::string_view(doc.at(1)));
      auto x = result.find(copy);
      if (x == result.end()) {  // If key not found, add new key
        result.emplace(copy, amazon_cellphones::brand{
          double(doc.at(5)) * uint64_t(doc.at(7)),
          uint64_t(doc.at(7))
        });
      } else {  // Otherwise, update key data
        x->second.cumulative_rating += double(doc.at(5)) * uint64_t(doc.at(7));
        x->second.reviews_count += uint64_t(doc.at(7));
      }
    }

    return true;
  }

};

BENCHMARK_TEMPLATE(amazon_cellphones, simdjson_dom<UNTHREADED>)->UseManualTime();
#ifdef SIMDJSON_THREADS_ENABLED
BENCHMARK_TEMPLATE(amazon_cellphones, simdjson_dom<THREADED>)->UseManualTime();
#endif

} // namespace amazon_cellphones

#endif // SIMDJSON_EXCEPTIONS