#pragma once

#ifdef SIMDJSON_COMPETITION_JSONIFIER

#include "amazon_cellphones.h"

namespace amazon_cellphones {

namespace jsonifier_api = ::jsonifier::generic;

struct jsonifier_generic {
  using StringType = std::string;

  ::jsonifier::jsonifier_core<> parser{};

  bool run(simdjson::padded_string &json, std::map<StringType, brand> &result) {
    auto stream = parser.iterateMany(json);
    auto i = stream.begin();
    ++i;
    for (; i != stream.end(); ++i) {
      auto doc = *i;
      auto fields = doc.getArray();
      if (fields.error() != jsonifier_api::error_code::success) { return false; }
      size_t index{0};
      std::string_view name;
      double rating{};
      uint64_t reviews{};
      for (auto value : fields) {
        switch (index) {
        case 1:
          if (value.getString(name) != jsonifier_api::error_code::success) { return false; }
          break;
        case 5:
          if (value.getDouble(rating) != jsonifier_api::error_code::success) { return false; }
          break;
        case 7:
          if (value.getUint64(reviews) != jsonifier_api::error_code::success) { return false; }
          break;
        default:
          break;
        }
        index++;
      }

      StringType copy(name);
      auto x = result.find(copy);
      if (x == result.end()) {
        result.emplace(copy, amazon_cellphones::brand{
          rating * reviews,
          reviews
        });
      } else {
        x->second.cumulative_rating += rating * reviews;
        x->second.reviews_count += reviews;
      }
    }

    return stream.error() == jsonifier_api::error_code::success;
  }
};

BENCHMARK_TEMPLATE(amazon_cellphones, jsonifier_generic)->UseManualTime();

} // namespace amazon_cellphones

#endif // SIMDJSON_COMPETITION_JSONIFIER
