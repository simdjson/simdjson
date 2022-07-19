#pragma once

#include "json_benchmark/file_runner.h"

namespace json2msgpack {

using namespace json_benchmark;

template <typename I> struct runner : public file_runner<I> {
  std::string_view result;
  std::unique_ptr<char[]> buffer;

  bool setup(benchmark::State &state) {
    bool isok = this->load_json(state, TWITTER_JSON);
    if (isok) {
      // Let us allocate a sizeable buffer.
      buffer = std::unique_ptr<char[]>(new char[this->json.size() * 4 + 1024]);
    }
    return isok;
  }

  bool before_run(benchmark::State &state) {
    if (!file_runner<I>::before_run(state)) {
      return false;
    }
    // Clear the buffer.
    ::memset(buffer.get(), 0, this->json.size() * 4 + 1024);
    return true;
  }

  bool run(benchmark::State &) {
    return this->implementation.run(this->json, buffer.get(), result);
  }

  template <typename R>
  bool diff(benchmark::State &state, runner<R> &reference) {
    return diff_results(state, result.size(), reference.result.size(), diff_flags::NONE);
  }
};

struct simdjson_ondemand;

template <typename I>
simdjson_inline static void json2msgpack(benchmark::State &state) {
  run_json_benchmark<runner<I>, runner<simdjson_ondemand>>(state);
}

} // namespace json2msgpack
