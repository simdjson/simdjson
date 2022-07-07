
#pragma once

#include "json_benchmark/file_runner.h"

namespace top_tweet {

using namespace json_benchmark;

template<typename StringType>
struct top_tweet_result {
  int64_t retweet_count{};
  StringType screen_name{};
  StringType text{};
  template<typename OtherStringType>
  simdjson_inline bool operator==(const top_tweet_result<OtherStringType> &other) const {
    return retweet_count == other.retweet_count &&
           screen_name == other.screen_name &&
           text == other.text;
  }
  template<typename OtherStringType>
  simdjson_inline bool operator!=(const top_tweet_result<OtherStringType> &other) const { return !(*this == other); }
};

template<typename StringType>
simdjson_unused static std::ostream &operator<<(std::ostream &o, const top_tweet_result<StringType> &t) {
  o << "retweet_count: " << t.retweet_count << std::endl;
  o << "screen_name: " << t.screen_name << std::endl;
  o << "text: " << t.text << std::endl;
  return o;
}

template<typename I>
struct runner : public file_runner<I> {
  top_tweet_result<typename I::StringType> result{};

  bool setup(benchmark::State &state) {
    return this->load_json(state, TWITTER_JSON);
  }

  bool before_run(benchmark::State &state) {
    if (!file_runner<I>::before_run(state)) { return false; }
    result.retweet_count = -1;
    return true;
  }

  bool run(benchmark::State &) {
    return this->implementation.run(this->json, 60, result);
  }

  template<typename R>
  bool diff(benchmark::State &state, runner<R> &reference) {
    return diff_results(state, result, reference.result, diff_flags::NONE);
  }

  size_t items_per_iteration() {
    return 1;
  }
};

struct simdjson_dom;

template<typename I> simdjson_inline static void top_tweet(benchmark::State &state) {
  json_benchmark::run_json_benchmark<runner<I>, runner<simdjson_dom>>(state);
}

} // namespace top_tweet
