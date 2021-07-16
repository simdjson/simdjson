#pragma once

#if SIMDJSON_EXCEPTIONS

#include "amazon_cellphones.h"

namespace amazon_cellphones {

using namespace simdjson;

struct simdjson_dom {
  using StringType = std::string_view;

  dom::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<brand<StringType>> &result) {
    auto stream = parser.parse_many(json);
    auto i = stream.begin();
    ++i;  // Skip first line
    for (;i != stream.end(); ++i) {
      auto doc = *i;
      auto x = result.begin();
      while(true) {
        if (x == result.end()) {
          result.emplace_back(amazon_cellphones::brand<StringType>{
            doc.at(1),
            doc.at(5).get_double() * doc.at(7).get_uint64(),
            doc.at(7),
          });
          break;
        }
        else if ((*x).brand_name == doc.at(1)) {
          (*x).cumulative_rating += doc.at(5).get_double() * doc.at(7).get_uint64();
          (*x).count += doc.at(7).get_uint64();
          break;
        }
        ++x;
      }
    }
    std::cout << result << std::endl;
    return true;
  }
};

BENCHMARK_TEMPLATE(amazon_cellphones, simdjson_dom)->UseManualTime();

} // namespace amazon_cellphones

#endif // SIMDJSON_EXCEPTIONS