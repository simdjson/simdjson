#pragma once

#if SIMDJSON_EXCEPTIONS

#include "amazon_cellphones.h"

namespace amazon_cellphones {

using namespace simdjson;

struct simdjson_dom {
  using StringType = std::string_view;

  dom::parser parser{};

  bool run(simdjson::padded_string &json, std::map<StringType, brand<StringType>> &result) {
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
    auto stream = parser.parse_many(json);
    auto i = stream.begin();
    ++i;  // Skip first line
    for (;i != stream.end(); ++i) {
      auto doc = *i;
      std::cout << "CURRENT: " << doc.at(1) << '\t' << doc.at(5) << std::endl;
      std::cout << "BEFORE: " << result << std::endl;
      auto is_in = result.find(doc.at(1));
      if (is_in != result.end()) {
        is_in->second.count++;
        is_in->second.total_rating = is_in->second.total_rating + doc.at(5).get_double();
      }
      else {
        result.emplace(doc.at(1), amazon_cellphones::brand<StringType>{
          doc.at(1).get_string(),
          doc.at(5).get_double()
        });
        std::cout << "NEW: "<< doc.at(1) << '\t' << doc.at(5) << std::endl;
      }
      std::cout << "AFTER: " << result << std::endl;
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(amazon_cellphones, simdjson_dom)->UseManualTime();

} // namespace amazon_cellphones

#endif // SIMDJSON_EXCEPTIONS