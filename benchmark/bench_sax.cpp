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
  uint64_t *expect_int{};
  std::string_view *expect_string{};

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
  iter.dom_parser.is_array[iter.depth] = true;
  if (expect_int || expect_string) { iter.log_error("expected int/string"); return TAPE_ERROR; }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::start_object(json_iterator &iter) {
  // iter.log_start_value("object");
  iter.dom_parser.is_array[iter.depth] = false;

  // if we expected an int or string and got an array or object, it's an error
  if (expect_int || expect_string) { iter.log_error("expected int/string"); return TAPE_ERROR; }

  // { "statuses": [ {
  if (in_statuses && iter.depth == 3) {
    iter.log_start_value("tweet");
    tweets.push_back({});
  }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::key(json_iterator &iter, const uint8_t *key) {
  // iter.log_value("key");
  if (in_statuses) {
    switch (iter.depth) {
      case 3: // in tweet: { "statuses": [ { <key>
        // NOTE: the way we're comparing key (fairly naturally) means the caller doesn't have to check " for us at all
        if (KEY_IS(key, "user")) { iter.log_start_value("user"); in_user = true; }

        else if (KEY_IS(key, "id")) { iter.log_value("id"); expect_int = &tweets.back().id; }
        else if (KEY_IS(key, "in_reply_to_status_id")) { iter.log_value("in_reply_to_status_id"); expect_int = &tweets.back().in_reply_to_status_id; }
        else if (KEY_IS(key, "retweet_count")) { iter.log_value("retweet_count"); expect_int = &tweets.back().retweet_count; }
        else if (KEY_IS(key, "favorite_count")) { iter.log_value("favorite_count"); expect_int = &tweets.back().favorite_count; }

        else if (KEY_IS(key, "text")) { iter.log_value("text"); expect_string = &tweets.back().text; }
        else if (KEY_IS(key, "created_at")) { iter.log_value("created_at"); expect_string = &tweets.back().created_at; }
        break;
      case 4:
        if (in_user) { // in user: { "statuses": [ { "user": { <key>
          if (KEY_IS(key, "id")) { iter.log_value("id"); expect_int = &tweets.back().user.id; }
          else if (KEY_IS(key, "screen_name")) { iter.log_value("screen_name"); expect_string = &tweets.back().user.screen_name; }
        }
        break;
      default: break;
    }
  } else {
    if (iter.depth == 1 && KEY_IS(key, "statuses")) {
      iter.log_start_value("statuses");
      in_statuses = true;
    }
  }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::primitive(json_iterator &iter, const uint8_t *value) {
  // iter.log_value("primitive");
  if (expect_int) {
    iter.log_value("int");
    if (auto error = numberparsing::parse_unsigned(value).get(*expect_int)) {
      // If number parsing failed, check if it's null before returning the error
      if (!atomparsing::is_valid_null_atom(value)) { iter.log_error("expected number or null"); return error; }
    }
    expect_int = nullptr;
  } else if (expect_string) {
    iter.log_value("string");
    // Must be a string!
    if (value[0] != '"') { iter.log_error("expected string"); return STRING_ERROR; }
    auto end = stringparsing::parse_string(value, current_string_buf_loc);
    if (!end) { iter.log_error("error parsing string"); return STRING_ERROR; }
    *expect_string = std::string_view((const char *)current_string_buf_loc, end-current_string_buf_loc);
    current_string_buf_loc = end;
    expect_string = nullptr;
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

really_inline error_code sax_tweet_reader_visitor::empty_array(json_iterator &iter) {
  // if we expected an int or string and got an array or object, it's an error
  // iter.log_value("empty array");
  if (expect_int || expect_string) { iter.log_error("expected int/string"); return TAPE_ERROR; }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::empty_object(json_iterator &iter) {
  // if we expected an int or string and got an array or object, it's an error
  // iter.log_value("empty object");
  if (expect_int || expect_string) { iter.log_error("expected int/string"); return TAPE_ERROR; }
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

static void sax_tweets(State& state) {
  // Load twitter.json to a buffer
  padded_string json;
  if (auto error = padded_string::load(TWITTER_JSON).get(json)) { cerr << error << endl; return; }

  // Allocate
  twitter::sax_tweet_reader reader;
  if (auto error = reader.set_capacity(json.size())) { cerr << error << endl; return; }

  // Make the tweet_reader
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
BENCHMARK(sax_tweets)->Repetitions(10)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);

BENCHMARK_MAIN();
