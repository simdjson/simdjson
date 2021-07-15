#pragma once

#if SIMDJSON_EXCEPTIONS

#include "amazon_cellphones.h"
#include <algorithm>

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
      amazon_cellphones::brand<StringType> b { doc.at(1), doc.at(5) };
      auto is_in = std::find(result.begin(),result.end(),b);
      if (is_in != result.end()) {
        std::cout << (*is_in).brand_name << '\t' << b.brand_name << std::endl;
        (*is_in).total_rating = (*is_in).total_rating + b.total_rating;
        (*is_in).count++;
      }
      else {
        std::cout << "NEW: " << b.brand_name << std::endl;
        result.push_back(b);
      }
    }
    for (auto b : result) {
      std::cout << b.brand_name << '\t' << b.total_rating << '\t' << b.count << std::endl;
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(amazon_cellphones, simdjson_dom)->UseManualTime();

} // namespace amazon_cellphones

#endif // SIMDJSON_EXCEPTIONS