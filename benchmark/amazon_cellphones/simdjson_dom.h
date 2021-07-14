#pragma once

#if SIMDJSON_EXCEPTIONS

#include "amazon_cellphones.h"

namespace amazon_cellphones {

using namespace simdjson;

struct simdjson_dom {
  dom::parser parser{};

  bool run(simdjson::padded_string &json, std::vector<uint64_t> &result) {
    // Walk the document, parsing as we go
    auto stream = parser.parse_many(json);
    for (auto i = stream.begin(); i != stream.end(); ++i) {
      std::cout << i.source() << std::endl;
    }
    return true;
  }
};

BENCHMARK_TEMPLATE(amazon_cellphones, simdjson_dom)->UseManualTime();

} // namespace amazon_cellphones

#endif // SIMDJSON_EXCEPTIONS