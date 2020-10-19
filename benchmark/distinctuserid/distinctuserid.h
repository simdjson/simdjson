
#pragma once
#include <vector>
#include <cstdint>
#include "event_counter.h"
#include "json_benchmark.h"


bool equals(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }

void remove_duplicates(std::vector<int64_t> &v) {
  std::sort(v.begin(), v.end());
  auto last = std::unique(v.begin(), v.end());
  v.erase(last, v.end());
}

//
// Interface
//

namespace distinct_user_id {
template<typename T> static void DistinctUserID(benchmark::State &state);
} // namespace 

//
// Implementation
//

#include "dom.h"


namespace distinct_user_id {

using namespace simdjson;

template<typename T> static void DistinctUserID(benchmark::State &state) {
  //
  // Load the JSON file
  //
  constexpr const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";
  error_code error;
  padded_string json;
  if ((error = padded_string::load(TWITTER_JSON).get(json))) {
    std::cerr << error << std::endl;
    state.SkipWithError("error loading");
    return;
  }

  JsonBenchmark<T, Dom>(state, json);
}

} // namespace distinct_user_id
