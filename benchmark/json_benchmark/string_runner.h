#pragma once

#include "runner_base.h"
#include "simdjson.h"

namespace json_benchmark {

template<typename I>
struct string_runner : public runner_base<I> {
  const simdjson::padded_string &original_json;
  simdjson::padded_string json;
  string_runner(const simdjson::padded_string &_json) : original_json{_json}, json(original_json.data(), original_json.size()) {}

  simdjson_warn_unused bool before_run(benchmark::State &state) {
    if (!runner_base<I>::after_run(state)) { return false; };
    // Copy the original json in case we did *in situ*
    std::memcpy(json.data(), original_json.data(), original_json.size());
    return true;
  }

  /** Get the total number of bytes processed in each iteration. Used for metrics like bytes/second. */
  size_t bytes_per_iteration() {
    return json.size();
  }

  /** Get the total number of documents processed in each iteration. Used for metrics like documents/second. */
  size_t documents_per_iteration() {
    return 1;
  }

  /** Get the total number of items processed in each iteration. Used for metrics like items/second. */
  size_t items_per_iteration() {
    return 1;
  }
};

} // namespace json_benchmark