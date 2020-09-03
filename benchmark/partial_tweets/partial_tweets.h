#pragma once

//
// Interface
//

namespace partial_tweets {
template<typename T> static void PartialTweets(benchmark::State &state);
} // namespace partial_tweets

//
// Implementation
//

#include "tweet.h"
#include <vector>
#include "event_counter.h"
#include "domnoexcept.h"
#include "json_benchmark.h"

namespace partial_tweets {

using namespace simdjson;

template<typename T> static void PartialTweets(benchmark::State &state) {
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

  JsonBenchmark<T, DomNoExcept>(state, json);
}

} // namespace partial_tweets
