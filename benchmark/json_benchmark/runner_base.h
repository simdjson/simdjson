#pragma once

#include "constants.h"
#include "run_json_benchmark.h"
#include "diff_results.h"

namespace json_benchmark {

//
// Extend this to create a new type of test (e.g. partial_tweets).
//
template<typename I>
struct runner_base {
  /** Run once, before all iterations. */
  simdjson_warn_unused bool setup(benchmark::State &) { return true; }

  /** Run on each iteration. This is what gets benchmarked. */
  simdjson_warn_unused bool run(benchmark::State &state) {
    return implementation.run(state);
  }

  /** Called before each iteration, to clear / set up state. */
  simdjson_warn_unused bool before_run(benchmark::State &state) { return true; }

  /** Called after each iteration, to tear down / massage state. */
  simdjson_warn_unused bool after_run(benchmark::State &) { return true; }

  /** Get the total number of bytes processed in each iteration. Used for metrics like bytes/second. */
  size_t bytes_per_iteration();

  /** Get the total number of documents processed in each iteration. Used for metrics like documents/second. */
  size_t documents_per_iteration();

  /** Get the total number of items processed in each iteration. Used for metrics like items/second. */
  size_t items_per_iteration();

  I implementation{};
};

}