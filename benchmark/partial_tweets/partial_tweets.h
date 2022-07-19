
#pragma once

#include "json_benchmark/file_runner.h"
#include "tweet.h"
#include <vector>

namespace partial_tweets {

using namespace json_benchmark;

template<typename I>
struct runner : public file_runner<I> {
  std::vector<tweet<typename I::StringType>> result{};

  bool setup(benchmark::State &state) {
    return this->load_json(state, TWITTER_JSON);
  }

  bool before_run(benchmark::State &state) {
    if (!file_runner<I>::before_run(state)) { return false; }
    result.clear();
    return true;
  }

  bool run(benchmark::State &) {
    return this->implementation.run(this->json, result);
  }

  template<typename R>
  bool diff(benchmark::State &state, runner<R> &reference) {
    return diff_results(state, result, reference.result, diff_flags::NONE);
  }

  size_t items_per_iteration() {
    return result.size();
  }
};

struct simdjson_dom;

template<typename I> simdjson_inline static void partial_tweets(benchmark::State &state) {
  run_json_benchmark<runner<I>, runner<simdjson_dom>>(state);
}

} // namespace partial_tweets
