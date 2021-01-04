#pragma once

#include "runner_base.h"
#include "simdjson.h"

namespace json_benchmark {

template<typename I>
struct const_json_runner : public runner_base<I> {
protected:
  const simdjson::padded_string &json;
  const_json_runner(const simdjson::padded_string &_json) : json{_json} {}

public:
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