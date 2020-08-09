#define SIMDJSON_IMPLEMENTATION_FALLBACK 0
#define SIMDJSON_IMPLEMENTATION_WESTMERE 0
#define SIMDJSON_IMPLEMENTATION_AMD64 0

#include "simdjson.h"
#include "simdjson.cpp"
using namespace simdjson;

using namespace haswell;
using namespace haswell::stage2;

SIMDJSON_TARGET_HASWELL

namespace twitter {

#define KEY_IS(KEY, MATCH) (!strncmp((const char *)KEY, "\"" MATCH "\"", strlen("\"" MATCH "\"")))

struct twitter_user {
  uint64_t id{};
  std::string_view screen_name{};
};
struct tweet {
  uint64_t id{};
  std::string_view text{};
  std::string_view created_at{};
  uint64_t in_reply_to_status_id{};
  uint64_t retweet_count{};
  uint64_t favorite_count{};
  twitter_user user{};
};
struct sax_tweet_reader {
  std::vector<tweet> tweets;
  std::unique_ptr<uint8_t[]> string_buf;
  size_t capacity;
  dom_parser_implementation dom_parser;

  sax_tweet_reader();
  error_code set_capacity(size_t new_capacity);
  error_code read_tweets(padded_string &json);
}; // struct tweet_reader

} // namespace twitter

namespace twitter {

struct sax_tweet_reader_visitor {
  bool in_statuses{false};
  bool in_user{false};
  std::vector<tweet> &tweets;
  uint8_t *current_string_buf_loc;
  const uint8_t *current_key{};

  sax_tweet_reader_visitor(std::vector<tweet> &_tweets, uint8_t *string_buf);

  really_inline error_code start_document(json_iterator &iter);
  really_inline error_code start_object(json_iterator &iter);
  really_inline error_code key(json_iterator &iter, const uint8_t *key);
  really_inline error_code primitive(json_iterator &iter, const uint8_t *value);
  really_inline error_code start_array(json_iterator &iter);
  really_inline error_code end_array(json_iterator &iter);
  really_inline error_code end_object(json_iterator &iter);
  really_inline error_code end_document(json_iterator &iter);
  really_inline error_code empty_array(json_iterator &iter);
  really_inline error_code empty_object(json_iterator &iter);
  really_inline error_code root_primitive(json_iterator &iter, const uint8_t *value);
  really_inline void increment_count(json_iterator &iter);
  really_inline bool in_array(json_iterator &iter) noexcept;

private:
  really_inline error_code parse_nullable_unsigned(json_iterator &iter, uint64_t &i, const uint8_t *value, const char *name);
  really_inline error_code parse_unsigned(json_iterator &iter, uint64_t &i, const uint8_t *value, const char *name);
  really_inline error_code parse_string(json_iterator &iter, std::string_view &s, const uint8_t *value, const char *name);
}; // sax_tweet_reader_visitor

sax_tweet_reader::sax_tweet_reader() : tweets{}, string_buf{}, capacity{0}, dom_parser() {}

error_code sax_tweet_reader::set_capacity(size_t new_capacity) {
  // string_capacity copied from document::allocate
  size_t string_capacity = ROUNDUP_N(5 * new_capacity / 3 + 32, 64);
  string_buf.reset(new (std::nothrow) uint8_t[string_capacity]);
  if (auto error = dom_parser.set_capacity(new_capacity)) { return error; }
  if (capacity == 0) { // set max depth the first time only
    if (auto error = dom_parser.set_max_depth(DEFAULT_MAX_DEPTH)) { return error; }
  }
  capacity = new_capacity;
  return SUCCESS;
}

// NOTE: this assumes the dom_parser is already allocated
error_code sax_tweet_reader::read_tweets(padded_string &json) {
  // Allocate capacity if needed
  tweets.clear();
  if (capacity < json.size()) {
    if (auto error = set_capacity(capacity)) { return error; }
  }

  // Run stage 1 first.
  if (auto error = dom_parser.stage1((uint8_t *)json.data(), json.size(), false)) { return error; }

  // Then walk the document, parsing the tweets as we go
  json_iterator iter(dom_parser, 0);
  sax_tweet_reader_visitor visitor(tweets, string_buf.get());
  if (auto error = iter.walk_document<false>(visitor)) { return error; }
  return SUCCESS;
}

sax_tweet_reader_visitor::sax_tweet_reader_visitor(std::vector<tweet> &_tweets, uint8_t *string_buf)
  : tweets{_tweets},
    current_string_buf_loc{string_buf} {
}

really_inline error_code sax_tweet_reader_visitor::start_document(json_iterator &iter) {
  iter.log_start_value("document");
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::start_array(json_iterator &iter) {
  // iter.log_start_value("array");
  // if we expected an int or string and got an array or object, it's an error
  if (iter.depth == 2 && KEY_IS(current_key, "statuses")) {
    iter.log_start_value("statuses");
    in_statuses = true;
  }
  iter.dom_parser.is_array[iter.depth] = true;
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::start_object(json_iterator &iter) {
  // iter.log_start_value("object");
  iter.dom_parser.is_array[iter.depth] = false;

  // { "statuses": [ {
  if (in_statuses) {
    switch (iter.depth) {
      case 3:
        iter.log_start_value("tweet");
        tweets.push_back({});
        break;
      case 4:
        // NOTE: the way we're comparing key (fairly naturally) means the caller doesn't have to check " for us at all
        if (KEY_IS(current_key, "user")) { iter.log_start_value("user"); in_user = true; }
    }
  }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::key(json_iterator &, const uint8_t *key) {
  current_key = key;
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::parse_nullable_unsigned(json_iterator &iter, uint64_t &i, const uint8_t *value, const char *name) {
  iter.log_value(name);
  if (auto error = numberparsing::parse_unsigned(value).get(i)) {
    // If number parsing failed, check if it's null before returning the error
    if (!atomparsing::is_valid_null_atom(value)) { iter.log_error("expected number or null"); return error; }
    i = 0;
  }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::parse_unsigned(json_iterator &iter, uint64_t &i, const uint8_t *value, const char *name) {
  iter.log_value(name);
  return numberparsing::parse_unsigned(value).get(i);
}
really_inline error_code sax_tweet_reader_visitor::parse_string(json_iterator &iter, std::string_view &s, const uint8_t *value, const char *name) {
  iter.log_value(name);
  // Must be a string!
  if (value[0] != '"') { iter.log_error("expected string"); return STRING_ERROR; }
  auto end = stringparsing::parse_string(value, current_string_buf_loc);
  if (!end) { iter.log_error("error parsing string"); return STRING_ERROR; }
  s = std::string_view((const char *)current_string_buf_loc, end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::primitive(json_iterator &iter, const uint8_t *value) {
  // iter.log_value("primitive");
  // iter.log_value("key");
  if (in_statuses) {
    switch (iter.depth) {
      case 3: // in tweet: { "statuses": [ { <key>
        if (KEY_IS(current_key, "id")) { return parse_unsigned(iter, tweets.back().id, value, "id"); }
        else if (KEY_IS(current_key, "in_reply_to_status_id")) { return parse_nullable_unsigned(iter, tweets.back().in_reply_to_status_id, value, "in_reply_to_status_id"); }
        else if (KEY_IS(current_key, "retweet_count")) { return parse_unsigned(iter, tweets.back().retweet_count, value, "retweet_count"); }
        else if (KEY_IS(current_key, "favorite_count")) { return parse_unsigned(iter, tweets.back().favorite_count, value, "favorite_count"); }
        else if (KEY_IS(current_key, "text")) { return parse_string(iter, tweets.back().text, value, "text"); }
        else if (KEY_IS(current_key, "created_at")) { return parse_string(iter, tweets.back().created_at, value, "created_at"); }
        break;
      case 4:
        if (in_user) { // in user: { "statuses": [ { "user": { <key>
          if (KEY_IS(current_key, "id")) { return parse_unsigned(iter, tweets.back().user.id, value, "id"); }
          else if (KEY_IS(current_key, "screen_name")) { { return parse_string(iter, tweets.back().user.screen_name, value, "screen_name"); } }
        }
        break;
      default: break;
    }
  }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::end_array(json_iterator &iter) {
  // iter.log_end_value("array");
  // When we hit the end of { "statuses": [ ... ], we're done with statuses.
  if (in_statuses && iter.depth == 2) { iter.log_end_value("statuses"); in_statuses = false; }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::end_object(json_iterator &iter) {
  // iter.log_end_value("object");
  // When we hit the end of { "statuses": [ { "user": { ... }, we're done with the user
  if (in_user && iter.depth == 4) { iter.log_end_value("user"); in_user = false; }
  if (in_statuses && iter.depth == 3) { iter.log_end_value("tweet"); }
  return SUCCESS;
}

really_inline error_code sax_tweet_reader_visitor::end_document(json_iterator &iter) {
  iter.log_end_value("document");
  return SUCCESS;
}

really_inline error_code sax_tweet_reader_visitor::empty_array(json_iterator &) {
  // iter.log_value("empty array");
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::empty_object(json_iterator &) {
  // iter.log_value("empty object");
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::root_primitive(json_iterator &iter, const uint8_t *) {
  // iter.log_value("root primitive");
  iter.log_error("unexpected root primitive");
  return TAPE_ERROR;
}

really_inline void sax_tweet_reader_visitor::increment_count(json_iterator &) { }
really_inline bool sax_tweet_reader_visitor::in_array(json_iterator &iter) noexcept {
  return iter.dom_parser.is_array[iter.depth];
}

} // namespace twitter

SIMDJSON_UNTARGET_REGION


SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

using namespace benchmark;
using namespace std;

const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";
const int REPETITIONS = 20;

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
        nullable_int(tweet["retweet_count"]),
        nullable_int(tweet["favorite_count"]),
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
BENCHMARK(dom_tweets)->Repetitions(REPETITIONS)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);

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
BENCHMARK(sax_tweets)->Repetitions(REPETITIONS)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);

BENCHMARK_MAIN();
