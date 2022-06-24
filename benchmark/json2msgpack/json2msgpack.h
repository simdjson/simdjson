#pragma once

#include "json_benchmark/file_runner.h"

namespace json2msgpack {

using namespace json_benchmark;

inline bool diff_str(const std::string_view &result,
                     const std::string_view &reference) {
  if (result != reference) {
    std::stringstream str;
    str << "result incorrect: " << result << " ... reference: " << reference;
    printf("result 1 : ");
    for (size_t i = 0; i < result.size(); i++) {
      printf("%x ", uint32_t(result[i]));
    }
    printf("\n");
    printf("result 2 : ");
    for (size_t i = 0; i < reference.size(); i++) {
      printf("%x ", uint32_t(reference[i]));
    }
    printf("\n");
    return false;
  }
  return true;
}

template <typename I> struct runner : public file_runner<I> {
  std::string_view result;
  std::unique_ptr<char[]> buffer;

  bool setup(benchmark::State &state) {
    bool isok = this->load_json(state, TWITTER_JSON);
    if (isok) {
      // Let us allocate a sizeable buffer.
      buffer = std::unique_ptr<char[]>(new char[this->json.size() * 4]);
    }
    return isok;
  }

  bool before_run(benchmark::State &state) {
    if (!file_runner<I>::before_run(state)) {
      return false;
    }
    // Clear the buffer.
    ::memset(buffer.get(), 0, this->json.size() * 4);
    return true;
  }

  bool run(benchmark::State &) {
    return this->implementation.run(this->json, buffer.get(), result);
  }

  template <typename R>
  bool diff(benchmark::State &state, runner<R> &reference) {
    diff_str(result, reference.result);
    return diff_results(state, result, reference.result, diff_flags::NONE);
  }
};

struct simdjson_ondemand;

template <typename I>
simdjson_really_inline static void json2msgpack(benchmark::State &state) {
  run_json_benchmark<runner<I>, runner<simdjson_ondemand>>(state);
}

} // namespace json2msgpack