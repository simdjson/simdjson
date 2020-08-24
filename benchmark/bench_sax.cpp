#define SIMDJSON_IMPLEMENTATION_FALLBACK 0
#define SIMDJSON_IMPLEMENTATION_WESTMERE 0
#define SIMDJSON_IMPLEMENTATION_ARM64 0

#include <iostream>
#include <sstream>
#include <random>

#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

#include "simdjson.cpp"

#if SIMDJSON_EXCEPTIONS

using namespace benchmark;
using namespace simdjson;
using std::cerr;
using std::endl;

const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";

#if SIMDJSON_IMPLEMENTATION_HASWELL

#include "twitter/tweet.h"
#include <vector>

SIMDJSON_TARGET_HASWELL

namespace ondemand_bench {

using namespace simdjson;
using namespace haswell;

simdjson_really_inline uint64_t nullable_int(ondemand::value && value) {
  if (value.is_null()) { return 0; }
  return std::move(value);
}

simdjson_really_inline twitter::twitter_user read_user(ondemand::object && user) {
  // TODO figure out why we can't use u directly ... the destructor doesn't seem to get invoked when
  // read_user() finishes, for some reason.
  ondemand::object u = std::move(user);
  return { u["id"], u["screen_name"] };
}
simdjson_really_inline void read_tweets(ondemand::parser &parser, padded_string &json, std::vector<twitter::tweet> &tweets) {
  // Walk the document, parsing the tweets as we go
  auto doc = parser.parse(json);
  auto root = doc.get_object();
  ondemand::array statuses = root["statuses"];
  for (ondemand::object tweet : statuses) {
    tweets.emplace_back(twitter::tweet{
      tweet["created_at"],
      tweet["id"],
      tweet["text"],
      nullable_int(tweet["in_reply_to_status_id"]),
      read_user(tweet["user"]),
      tweet["retweet_count"],
      tweet["favorite_count"]
    });
  }
}

static void ondemand_tweets(State &state) {
  // Load twitter.json to a buffer
  padded_string json;
  if (auto error = padded_string::load(TWITTER_JSON).get(json)) { cerr << error << endl; return; }

  // Allocate and warm the vector
  std::vector<twitter::tweet> tweets;
  ondemand::parser parser;
  read_tweets(parser, json, tweets);

  // Read tweets
  size_t byte_count = 0;
  size_t tweet_count = 0;
  for (SIMDJSON_UNUSED auto _ : state) {
    tweets.clear();
    read_tweets(parser, json, tweets);
    byte_count += json.size();
    tweet_count += tweets.size();
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["bytes"] = benchmark::Counter(
	        double(byte_count), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["tweets"] = Counter(double(tweet_count), benchmark::Counter::kIsRate);
}

BENCHMARK(ondemand_tweets)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  });

} // namespace ondemand_bench


SIMDJSON_UNTARGET_REGION

SIMDJSON_TARGET_HASWELL

namespace iter_bench {

using namespace simdjson;
using namespace haswell;

simdjson_really_inline void read_tweets(ondemand::parser &parser, padded_string &json, std::vector<twitter::tweet> &tweets) {
  // Walk the document, parsing the tweets as we go

  // { "statuses": 
  auto doc = parser.parse(json);
  ondemand::json_iterator &iter = doc.iterate();
  if (!iter.start_object()   || !iter.find_field_raw("statuses")) { throw; }
  // { "statuses": [
  if (!iter.start_array()) { throw; }

  do {
    twitter::tweet tweet;

    if (!iter.start_object()   || !iter.find_field_raw("created_at")) { throw; }
    tweet.created_at = iter.get_raw_json_string().value().unescape(parser);

    if (!iter.has_next_field() || !iter.find_field_raw("id")) { throw; }
    tweet.id = iter.get_uint64();

    if (!iter.has_next_field() || !iter.find_field_raw("text")) { throw; }
    tweet.text = iter.get_raw_json_string().value().unescape(parser);

    if (!iter.has_next_field() || !iter.find_field_raw("in_reply_to_status_id")) { throw; }
    if (!iter.is_null()) {
      tweet.in_reply_to_status_id = iter.get_uint64();
    }

    if (!iter.has_next_field() || !iter.find_field_raw("user")) { throw; }
    {
      if (!iter.start_object()   || !iter.find_field_raw("id")) { throw; }
      tweet.user.id = iter.get_uint64();

      if (!iter.has_next_field() || !iter.find_field_raw("screen_name")) { throw; }
      tweet.user.screen_name = iter.get_raw_json_string().value().unescape(parser);

      iter.skip_container(); // Skip the rest of the user object
    }

    if (!iter.has_next_field() || !iter.find_field_raw("retweet_count")) { throw; }
    tweet.retweet_count = iter.get_uint64();

    if (!iter.has_next_field() || !iter.find_field_raw("favorite_count")) { throw; }
    tweet.favorite_count = iter.get_uint64();

    tweets.push_back(tweet);

    iter.skip_container(); // Skip the rest of the tweet object

  } while (iter.has_next_element());
}

static void iter_tweets(State &state) {
  // Load twitter.json to a buffer
  padded_string json;
  if (auto error = padded_string::load(TWITTER_JSON).get(json)) { cerr << error << endl; return; }

  // Allocate and warm the vector
  std::vector<twitter::tweet> tweets;
  ondemand::parser parser;
  read_tweets(parser, json, tweets);

  // Read tweets
  size_t byte_count = 0;
  size_t tweet_count = 0;
  for (SIMDJSON_UNUSED auto _ : state) {
    tweets.clear();
    read_tweets(parser, json, tweets);
    byte_count += json.size();
    tweet_count += tweets.size();
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["bytes"] = benchmark::Counter(
	        double(byte_count), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["tweets"] = Counter(double(tweet_count), benchmark::Counter::kIsRate);
}

BENCHMARK(iter_tweets)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  });

} // namespace iter_bench


SIMDJSON_UNTARGET_REGION

#include "twitter/sax_tweet_reader.h"

static void sax_tweets(State &state) {
  // Load twitter.json to a buffer
  padded_string json;
  if (auto error = padded_string::load(TWITTER_JSON).get(json)) { cerr << error << endl; return; }

  size_t bytes = 0;
  size_t tweets = 0;
  {
    // Yes, we leak this. Destructor issues. TODO fix that
    twitter::sax_tweet_reader reader;

    // Warm the vector and allocate capacity
    if (auto error = reader.read_tweets(json)) { throw error; }

    // Read tweets
    for (SIMDJSON_UNUSED auto _ : state) {
      if (auto error = reader.read_tweets(json)) { throw error; }
      bytes += json.size();
      tweets += reader.tweets.size();
    }
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["bytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["tweets"] = Counter(double(tweets), benchmark::Counter::kIsRate);
}
BENCHMARK(sax_tweets)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  });

#endif // SIMDJSON_IMPLEMENTATION_HASWELL

#include "twitter/tweet.h"

simdjson_really_inline uint64_t nullable_int(dom::element element) {
  if (element.is_null()) { return 0; }
  return element;
}
simdjson_really_inline void read_dom_tweets(dom::parser &parser, padded_string &json, std::vector<twitter::tweet> &tweets) {
  for (dom::element tweet : parser.parse(json)["statuses"]) {
    auto user = tweet["user"];
    tweets.emplace_back(twitter::tweet{
      tweet["created_at"],
      tweet["id"],
      tweet["text"],
      nullable_int(tweet["in_reply_to_status_id"]),
      { user["id"], user["screen_name"] },
      tweet["retweet_count"],
      tweet["favorite_count"]
    });
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
  for (SIMDJSON_UNUSED auto _ : state) {
    tweets.clear();
    read_dom_tweets(parser, json, tweets);
    bytes += json.size();
    num_tweets += tweets.size();
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["bytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["tweets"] = Counter(double(num_tweets), benchmark::Counter::kIsRate);
}
BENCHMARK(dom_tweets)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  });

static void parse_tweets(State &state) {
  // Load twitter.json to a buffer
  padded_string json;
  if (auto error = padded_string::load(TWITTER_JSON).get(json)) { cerr << error << endl; return; }

  // Allocate
  dom::parser parser;
  if (auto error = parser.allocate(json.size())) { cerr << error << endl; return; };

  // Read tweets
  size_t bytes = 0;
  for (SIMDJSON_UNUSED auto _ : state) {
    if (parser.parse(json).error()) { throw "Parsing failed"; };
    bytes += json.size();
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["bytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
}
BENCHMARK(parse_tweets)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  });


/********************
 * Large file parsing benchmarks:
 ********************/

static std::string build_json_array(size_t N) {
  std::default_random_engine e;
  std::uniform_real_distribution<> dis(0, 1);
  std::stringstream myss;
  myss << "[" << std::endl;
  if(N > 0) {
    myss << "{ \"x\":" << dis(e) << ",  \"y\":" << dis(e) << ", \"z\":" << dis(e) << "}" << std::endl;
  }
  for(size_t i = 1; i < N; i++) {
    myss << "," << std::endl;
    myss << "{ \"x\":" << dis(e) << ",  \"y\":" << dis(e) << ", \"z\":" << dis(e) << "}";
  }
  myss << std::endl;
  myss << "]" << std::endl;
  std::string answer = myss.str();
  std::cout << "Creating a source file spanning " << (answer.size() + 512) / 1024 << " KB " << std::endl;  
  return answer;
}

static const simdjson::padded_string& get_my_json_str() {
  static simdjson::padded_string s = build_json_array(1000000);
  return s;
}

struct my_point {
  double x;
  double y;
  double z;
};

// ./benchmark/bench_sax --benchmark_filter=largerandom


/*** 
 * We start with the naive DOM-based approach.
 **/
static void dom_largerandom(State &state) {
  // Load twitter.json to a buffer
  const padded_string& json = get_my_json_str();

  // Allocate
  dom::parser parser;
  if (auto error = parser.allocate(json.size())) { cerr << error << endl; return; };

  // Read
  size_t bytes = 0;
  size_t points = 0;
  simdjson::error_code error;
  for (SIMDJSON_UNUSED auto _ : state) {
    std::vector<my_point> container;
    dom::element doc;
    if ((error = parser.parse(json).get(doc))) { 
      std::cerr << "failure: " << error << std::endl;
      throw "Parsing failed"; 
    };
    for (auto point : doc) {
      container.emplace_back(my_point{point["x"], point["y"], point["z"]});
    }
    bytes += json.size();
    points += container.size();
    benchmark::DoNotOptimize(container.data());
  }

  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["bytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["points"] = Counter(double(points), benchmark::Counter::kIsRate);
}

BENCHMARK(dom_largerandom)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  });

#if SIMDJSON_IMPLEMENTATION_HASWELL

SIMDJSON_TARGET_HASWELL

/*** 
 * On Demand approach.
 **/
static void ondemand_largerandom(State &state) {
  using namespace haswell;
  // Load twitter.json to a buffer
  const padded_string& json = get_my_json_str();

  // Allocate
  ondemand::parser parser;
  error_code error;
  if ((error = parser.allocate(json.size()))) { throw error; };

  // Read
  size_t bytes = 0;
  size_t points = 0;
  for (SIMDJSON_UNUSED auto _ : state) {
    std::vector<my_point> container;
    auto doc = parser.parse(json);
    ondemand::array array = doc.get_array();
    for (ondemand::object point_object : array) {
      auto point = point_object.begin();
      container.emplace_back(my_point{(*point).value(), (*++point).value(), (*++point).value()});
    }
    bytes += json.size();
    points += container.size();
    benchmark::DoNotOptimize(container.data());
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["bytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["points"] = Counter(double(points), benchmark::Counter::kIsRate);
}

SIMDJSON_UNTARGET_REGION

BENCHMARK(ondemand_largerandom)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  });

SIMDJSON_TARGET_HASWELL

static simdjson_really_inline double first_double(haswell::ondemand::json_iterator &iter) {
  if (iter.start_object().error() || iter.field_key().error() || iter.field_value()) { throw "Invalid field"; }
  return iter.get_double();
}

static simdjson_really_inline double next_double(haswell::ondemand::json_iterator &iter) {
  if (!iter.has_next_field() || iter.field_key().error() || iter.field_value()) { throw "Invalid field"; }
  return iter.get_double();
}

/*** 
 * On Demand Iterator approach.
 **/
static void iter_largerandom(State &state) {
  using namespace haswell;
  // Load twitter.json to a buffer
  const padded_string& json = get_my_json_str();

  // Allocate
  ondemand::parser parser;
  error_code error;
  if ((error = parser.allocate(json.size()))) { throw error; };

  // Read
  size_t bytes = 0;
  size_t points = 0;
  for (SIMDJSON_UNUSED auto _ : state) {
    std::vector<my_point> container;
    auto doc = parser.parse(json);
    ondemand::json_iterator &iter = doc.iterate();
    if (iter.start_array()) {
      do {
        container.emplace_back(my_point{first_double(iter), next_double(iter), next_double(iter)});
        if (iter.has_next_field()) { throw "Too many fields"; }
      } while (iter.has_next_element());
    }
    bytes += json.size();
    points += container.size();
    benchmark::DoNotOptimize(container.data());
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["bytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["points"] = Counter(double(points), benchmark::Counter::kIsRate);
}

SIMDJSON_UNTARGET_REGION

BENCHMARK(iter_largerandom)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  });

/*** 
 * Next we are going to code the SAX approach.
 **/

SIMDJSON_TARGET_HASWELL

namespace largerandom {
namespace {

using namespace simdjson;
using namespace haswell;
using namespace haswell::stage2;
struct sax_point_reader_visitor {
public:
  sax_point_reader_visitor(std::vector<my_point> &_points) : points(_points) {
  }

  simdjson_really_inline error_code visit_document_start(json_iterator &) { return SUCCESS; }
  simdjson_really_inline error_code visit_object_start(json_iterator &) {
    idx = 0;
    return SUCCESS;
  }
  simdjson_really_inline error_code visit_key(json_iterator &, const uint8_t *) {
    return SUCCESS; 
  }
  simdjson_really_inline error_code visit_primitive(json_iterator &, const uint8_t *value) {
    return numberparsing::parse_double(value).get(buffer[idx++]);
  }
  simdjson_really_inline error_code visit_array_start(json_iterator &)  { return SUCCESS; }
  simdjson_really_inline error_code visit_array_end(json_iterator &) { return SUCCESS; }
  simdjson_really_inline error_code visit_object_end(json_iterator &)  {
    points.emplace_back(my_point{buffer[0], buffer[1], buffer[2]});
    return SUCCESS;
  }
  simdjson_really_inline error_code visit_document_end(json_iterator &)  { return SUCCESS; }
  simdjson_really_inline error_code visit_empty_array(json_iterator &)  { return SUCCESS; }
  simdjson_really_inline error_code visit_empty_object(json_iterator &)  { return SUCCESS; }
  simdjson_really_inline error_code visit_root_primitive(json_iterator &, const uint8_t *)  { return SUCCESS; }
  simdjson_really_inline error_code increment_count(json_iterator &) { return SUCCESS; }
  std::vector<my_point> &points;
  size_t idx{0};
  double buffer[3];
};

struct sax_point_reader {
  std::vector<my_point> points;
  std::unique_ptr<uint8_t[]> string_buf;
  size_t capacity;
  dom_parser_implementation dom_parser;

  sax_point_reader();
  error_code set_capacity(size_t new_capacity);
  error_code read_points(const padded_string &json);
}; // struct sax_point_reader

sax_point_reader::sax_point_reader() : points{}, string_buf{}, capacity{0}, dom_parser() {
}

error_code sax_point_reader::set_capacity(size_t new_capacity) {
  // string_capacity copied from document::allocate
  size_t string_capacity = SIMDJSON_ROUNDUP_N(5 * new_capacity / 3 + 32, 64);
  string_buf.reset(new (std::nothrow) uint8_t[string_capacity]);
  if (auto error = dom_parser.set_capacity(new_capacity)) { return error; }
  if (capacity == 0) { // set max depth the first time only
    if (auto error = dom_parser.set_max_depth(DEFAULT_MAX_DEPTH)) { return error; }
  }
  capacity = new_capacity;
  return SUCCESS;
}

error_code sax_point_reader::read_points(const padded_string &json) {
  // Allocate capacity if needed
  points.clear();
  if (capacity < json.size()) {
    if (auto error = set_capacity(capacity)) { return error; }
  }

  // Run stage 1 first.
  if (auto error = dom_parser.stage1((uint8_t *)json.data(), json.size(), false)) { return error; }

  // Then walk the document, parsing the tweets as we go
  json_iterator iter(dom_parser, 0);
  sax_point_reader_visitor visitor(points);
  if (auto error = iter.walk_document<false>(visitor)) { return error; }
  return SUCCESS;
}

} // unnamed namespace
} // namespace largerandom

SIMDJSON_UNTARGET_REGION





// ./benchmark/bench_sax --benchmark_filter=largerandom
static void sax_largerandom(State &state) {
  // Load twitter.json to a buffer
  const padded_string& json = get_my_json_str();

  // Allocate
  largerandom::sax_point_reader reader;
  if (auto error = reader.set_capacity(json.size())) { throw error; }
  // warming
  for(size_t i = 0; i < 10; i++) {
    if (auto error = reader.read_points(json)) { throw error; }
  }

  // Read
  size_t bytes = 0;
  size_t points = 0;
  for (SIMDJSON_UNUSED auto _ : state) {
    if (auto error = reader.read_points(json)) { throw error; }
    bytes += json.size();
    points += reader.points.size();
    benchmark::DoNotOptimize(reader.points.data());
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["bytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
  state.counters["points"] = Counter(double(points), benchmark::Counter::kIsRate);
}
BENCHMARK(sax_largerandom)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  });

#endif // SIMDJSON_IMPLEMENTATION_HASWELL

#endif // SIMDJSON_EXCEPTIONS

BENCHMARK_MAIN();
