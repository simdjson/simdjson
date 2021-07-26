#pragma once

#if SIMDJSON_EXCEPTIONS

#include "large_amazon_cellphones.h"

namespace large_amazon_cellphones {

using namespace simdjson;

struct simdjson_ondemand {
  using StringType = std::string;

  ondemand::parser parser{};

  bool run(simdjson::padded_string &json, std::map<StringType, brand> &result) {
    ondemand::document_stream stream = parser.iterate_many(json);
    ondemand::document_stream::iterator i = stream.begin();
    ++i;  // Skip first line
    for (;i != stream.end(); ++i) {
      auto doc = *i;
      size_t index{0};
      StringType copy;
      double rating;
      uint64_t reviews;
      for ( auto value : doc ) {
        switch (index)
        {
        case 1:
          copy = StringType(std::string_view(value));
          break;
        case 5:
          rating = double(value);
          break;
        case 7:
          reviews = uint64_t(value);
          break;
        default:
          break;
        }
        index++;
      }

      auto x = result.find(copy);
      if (x == result.end()) {  // If key not found, add new key
        result.emplace(copy, large_amazon_cellphones::brand{
          rating * reviews,
          reviews
        });
      } else {  // Otherwise, update key data
        x->second.cumulative_rating += rating * reviews;
        x->second.reviews_count += reviews;
      }
    }

    return true;
  }

};

BENCHMARK_TEMPLATE(large_amazon_cellphones, simdjson_ondemand)->UseManualTime();

} // namespace amazon_cellphones

#endif // SIMDJSON_EXCEPTIONS