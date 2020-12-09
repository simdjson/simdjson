
#pragma once
#include <vector>
#include <cstdint>
#include "event_counter.h"
#include "json_benchmark.h"


//
// Interface
//

namespace find_tweet {
template<typename T> static void FindTweet(benchmark::State &state);
const uint64_t TWEET_ID = 505874901689851900;
} // namespace

//
// Implementation
//

#include "dom.h"


namespace find_tweet {

using namespace simdjson;

template<typename T> static void FindTweet(benchmark::State &state) {
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

} // namespace find_tweet
