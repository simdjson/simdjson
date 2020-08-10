#define SIMDJSON_IMPLEMENTATION_FALLBACK 0
#define SIMDJSON_IMPLEMENTATION_WESTMERE 0
#define SIMDJSON_IMPLEMENTATION_AMD64 0

#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

#include "simdjson.cpp"
#include "twitter/sax_tweet_reader.h"

using namespace benchmark;
using namespace simdjson;
using std::cerr;
using std::endl;

const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";
const int REPETITIONS = 10;

static void sax_tweets(State &state) {
  // Load twitter.json to a buffer
  padded_string json;
  if (auto error = padded_string::load(TWITTER_JSON).get(json)) { cerr << error << endl; return; }

  // Allocate
  twitter::sax_tweet_reader reader;
  if (auto error = reader.set_capacity(json.size())) { cerr << error << endl; return; }

  // Warm the vector
  if (auto error = reader.read_tweets(json)) { throw error; }

  // Read tweets
  size_t bytes = 0;
  size_t tweets = 0;
  for (UNUSED auto _ : state) {
    if (auto error = reader.read_tweets(json)) { throw error; }
    bytes += json.size();
    tweets += reader.tweets.size();
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["Gigabytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["tweets"] = Counter(double(tweets), benchmark::Counter::kIsRate);
}

really_inline uint64_t nullable_int(dom::element element) {
  if (element.is_null()) { return 0; }
  return element;
}
really_inline void read_dom_tweets(dom::parser &parser, padded_string &json, std::vector<twitter::tweet> &tweets) {
  for (dom::element tweet : parser.parse(json)["statuses"]) {
    auto user = tweet["user"];
    tweets.push_back(
      {
        tweet["id"],
        tweet["text"],
        tweet["created_at"],
        nullable_int(tweet["in_reply_to_status_id"]),
        tweet["retweet_count"],
        tweet["favorite_count"],
        { user["id"], user["screen_name"] }
      }
    );
  }
}

static void dom_tweets(State &state) {
  // Load twitter.json to a buffer
  padded_string json;
  if (auto error = padded_string::load(TWITTER_JSON).get(json)) { cerr << error << endl; return; }

  // Allocate
  dom::parser parser;
  if (auto error = parser.allocate(json.size())) { cerr << error << endl; return; };

  // Warm the vector
  std::vector<twitter::tweet> tweets;
  read_dom_tweets(parser, json, tweets);

  // Read tweets
  size_t bytes = 0;
  size_t num_tweets = 0;
  for (UNUSED auto _ : state) {
    tweets.clear();
    read_dom_tweets(parser, json, tweets);
    bytes += json.size();
    num_tweets += tweets.size();
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["Gigabytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["tweets"] = Counter(double(num_tweets), benchmark::Counter::kIsRate);
}

static void dom_parse(State &state) {
  // Load twitter.json to a buffer
  padded_string json;
  if (auto error = padded_string::load(TWITTER_JSON).get(json)) { cerr << error << endl; return; }

  // Allocate
  dom::parser parser;
  if (auto error = parser.allocate(json.size())) { cerr << error << endl; return; };

  // Read tweets
  size_t bytes = 0;
  for (UNUSED auto _ : state) {
    if (parser.parse(json).error()) { throw "Parsing failed"; };
    bytes += json.size();
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["Gigabytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
}

BENCHMARK(sax_tweets)->Repetitions(REPETITIONS)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);
BENCHMARK(dom_tweets)->Repetitions(REPETITIONS)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);
BENCHMARK(dom_parse)->Repetitions(REPETITIONS)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);

BENCHMARK_MAIN();
