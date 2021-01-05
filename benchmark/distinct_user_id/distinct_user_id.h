
#pragma once

#include "json_benchmark/file_runner.h"
#include <vector>

namespace distinct_user_id {

template<typename I>
struct runner : public json_benchmark::file_runner<I> {
  std::vector<uint64_t> ids{};

  bool setup(benchmark::State &state) {
    return this->load_json(state, json_benchmark::TWITTER_JSON);
  }

  bool before_run(benchmark::State &state) {
    if (!json_benchmark::file_runner<I>::before_run(state)) { return false; }
    ids.clear();
    return true;
  }

  bool run(benchmark::State &) {
    return this->implementation.run(this->json, ids);
  }

  bool after_run(benchmark::State &state) {
    if (!json_benchmark::file_runner<I>::after_run(state)) { return false; }
    std::sort(ids.begin(), ids.end());
    auto last = std::unique(ids.begin(), ids.end());
    ids.erase(last, ids.end());
    return true;
  }

  template<typename R>
  bool diff(benchmark::State &state, runner<R> &reference) {
    return diff_results(state, ids, reference.ids);
  }

  size_t items_per_iteration() {
    return ids.size();
  }
};

struct simdjson_dom;

template<typename I> simdjson_really_inline static void distinct_user_id(benchmark::State &state) {
  json_benchmark::run_json_benchmark<runner<I>, runner<simdjson_dom>>(state);
}

} // namespace distinct_user_id
