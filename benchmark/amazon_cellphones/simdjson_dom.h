#pragma once

#if SIMDJSON_EXCEPTIONS

#include "amazon_cellphones.h"

namespace amazon_cellphones {

using namespace simdjson;

struct simdjson_ondemand {
  using StringType = std::string;

  dom::parser parser{};

  bool run(simdjson::padded_string &json, std::map<StringType, brand> &result) {
    auto stream = parser.parse_many(json);
    auto i = stream.begin();
    ++i;  // Skip first line
    for (;i != stream.end(); ++i) {
      auto doc = *i;
      StringType copy(std::string_view(doc.at(1).get_string()));
      auto x = result.find(copy);
      if (x == result.end()) {  // If key not found, add new key
        result.emplace(copy, amazon_cellphones::brand{
          doc.at(5).get_double() * doc.at(7).get_uint64(),
          doc.at(7).get_uint64()
        });
      } else {  // Otherwise, update key data
        x->second.cumulative_rating += doc.at(5).get_double() * doc.at(7).get_uint64();
        x->second.count += doc.at(7).get_uint64();
      }
    }

    return true;
  }

};

BENCHMARK_TEMPLATE(amazon_cellphones, simdjson_dom)->UseManualTime();

} // namespace amazon_cellphones

#endif // SIMDJSON_EXCEPTIONS