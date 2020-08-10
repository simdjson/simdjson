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
public:
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
  // Since we only care about one thing at each level, we just use depth as the marker for what
  // object/array we're nested inside.
  enum class containers {
    document = 0,   //
    top_object = 1, // {
    statuses = 2,   // { "statuses": [
    tweet = 3,      // { "statuses": [ {
    user = 4        // { "statuses": [ { "user": {
  };
  /**
   * The largest depth we care about.
   * There can be things at lower depths.
   */
  static constexpr uint32_t MAX_SUPPORTED_DEPTH = uint32_t(containers::user);
  static constexpr const char *STATE_NAMES[] = {
    "document",
    "top object",
    "statuses",
    "tweet",
    "user"
  };
  enum class field_type {
    any,
    unsigned_integer,
    string,
    nullable_unsigned_integer,
    object,
    array
  };
  struct field {
    const char * key{};
    size_t len{0};
    size_t offset;
    containers container{containers::document};
    field_type type{field_type::any};
  };

  containers container{containers::document};
  std::vector<tweet> &tweets;
  uint8_t *current_string_buf_loc;
  const uint8_t *current_key{};

  really_inline bool in_container(json_iterator &iter);
  really_inline bool in_container_child(json_iterator &iter);
  really_inline void start_container(json_iterator &iter);
  really_inline void end_container(json_iterator &iter);
  really_inline error_code parse_nullable_unsigned(json_iterator &iter, const uint8_t *value, const field &f);
  really_inline error_code parse_unsigned(json_iterator &iter, const uint8_t *value, const field &f);
  really_inline error_code parse_string(json_iterator &iter, const uint8_t *value, const field &f);

  struct field_lookup {
    field entries[256]{};

    field_lookup();
    really_inline field get(const uint8_t * key, containers container);
  private:
    really_inline uint8_t hash(const char * key, uint32_t depth);
    really_inline void add(const char * key, size_t len, containers container, field_type type, size_t offset);
    really_inline void neg(const char * const key, uint32_t depth);
  };
  static field_lookup fields;
}; // sax_tweet_reader_visitor

sax_tweet_reader::sax_tweet_reader() : tweets{}, string_buf{}, capacity{0}, dom_parser() {
}

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

#define KEY_IS(MATCH) (!strncmp((const char *)current_key, "\"" MATCH "\"", strlen("\"" MATCH "\"")))

sax_tweet_reader_visitor::sax_tweet_reader_visitor(std::vector<tweet> &_tweets, uint8_t *string_buf)
  : tweets{_tweets},
    current_string_buf_loc{string_buf} {
}

really_inline error_code sax_tweet_reader_visitor::start_document(json_iterator &iter) {
  start_container(iter);
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::start_array(json_iterator &iter) {
  iter.dom_parser.is_array[iter.depth] = true;

  // If we're not in a container we care about, don't bother with the rest
  if (!in_container_child(iter)) { return SUCCESS; }

  // Handle fields first
  if (current_key) {
    switch (fields.get(current_key, container).type) {
      case field_type::array: // { "statuses": [
        start_container(iter);
        return SUCCESS;
      case field_type::any:
        return SUCCESS;
      case field_type::object:
      case field_type::unsigned_integer:
      case field_type::nullable_unsigned_integer:
      case field_type::string:
        iter.log_error("unexpected array field");
        return INCORRECT_TYPE;
    }
  }

  // We're not in a field, so it must be a child of an array. We support any of those.
  iter.log_error("unexpected array");
  return INCORRECT_TYPE;
}
really_inline error_code sax_tweet_reader_visitor::start_object(json_iterator &iter) {
  // iter.log_start_value("object");
  iter.dom_parser.is_array[iter.depth] = false;

  // If we're not in a container we care about, don't bother with the rest
  if (!in_container_child(iter)) { return SUCCESS; }

  // Handle known fields
  if (current_key) {
    auto f = fields.get(current_key, container);
    switch (f.type) {
      case field_type::object: // { "statuses": [ { "user": {
        start_container(iter);
        return SUCCESS;
      case field_type::any:
        return SUCCESS;
      case field_type::array:
      case field_type::unsigned_integer:
      case field_type::nullable_unsigned_integer:
      case field_type::string:
        iter.log_error("unexpected object field");
        return INCORRECT_TYPE;
    }
  }

  // It's not a field, so it's a child of an array or document
  switch (container) {
    case containers::document: // top_object: {
    case containers::statuses: // tweet:      { "statuses": [ {
      start_container(iter);
      return SUCCESS;
    case containers::top_object:
    case containers::tweet:
    case containers::user:
      iter.log_error("unexpected object");
      return INCORRECT_TYPE;
  }
  SIMDJSON_UNREACHABLE();
}
really_inline error_code sax_tweet_reader_visitor::key(json_iterator &, const uint8_t *key) {
  current_key = key;
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::primitive(json_iterator &iter, const uint8_t *value) {
  // Don't bother unless we're in a container we care about
  if (!in_container(iter)) { return SUCCESS; }

  // Handle fields first
  if (current_key) {
    auto f = fields.get(current_key, container);
    switch (f.type) {
      case field_type::unsigned_integer:
        return parse_unsigned(iter, value, f);
      case field_type::nullable_unsigned_integer:
        return parse_nullable_unsigned(iter, value, f);
      case field_type::string:
        return parse_string(iter, value, f);
      case field_type::any:
        return SUCCESS;
      case field_type::array:
      case field_type::object:
        iter.log_error("unexpected primitive");
        return INCORRECT_TYPE;
    }
  }

  // If it's not a field, it's a child of an array.
  // The only array we support is statuses, which must contain objects.
  iter.log_error("unexpected primitive");
  return INCORRECT_TYPE;
}
really_inline error_code sax_tweet_reader_visitor::end_array(json_iterator &iter) {
  if (in_container(iter)) { end_container(iter); }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::end_object(json_iterator &iter) {
  if (in_container(iter)) { end_container(iter); }
  return SUCCESS;
}

really_inline error_code sax_tweet_reader_visitor::end_document(json_iterator &iter) {
  iter.log_end_value("document");
  return SUCCESS;
}

really_inline error_code sax_tweet_reader_visitor::empty_array(json_iterator &) {
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::empty_object(json_iterator &) {
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::root_primitive(json_iterator &iter, const uint8_t *) {
  iter.log_error("unexpected root primitive");
  return INCORRECT_TYPE;
}

really_inline void sax_tweet_reader_visitor::increment_count(json_iterator &) { }
really_inline bool sax_tweet_reader_visitor::in_array(json_iterator &iter) noexcept {
  return iter.dom_parser.is_array[iter.depth];
}

really_inline bool sax_tweet_reader_visitor::in_container(json_iterator &iter) {
  return iter.depth == uint32_t(container);
}
really_inline bool sax_tweet_reader_visitor::in_container_child(json_iterator &iter) {
  return iter.depth == uint32_t(container) + 1;
}
really_inline void sax_tweet_reader_visitor::start_container(json_iterator &iter) {
  SIMDJSON_ASSUME(iter.depth <= MAX_SUPPORTED_DEPTH); // Asserts in debug mode
  container = containers(iter.depth);
  if (logger::LOG_ENABLED) { iter.log_start_value(STATE_NAMES[iter.depth]); }
}
really_inline void sax_tweet_reader_visitor::end_container(json_iterator &iter) {
  if (logger::LOG_ENABLED) { iter.log_end_value(STATE_NAMES[int(container)]); }
  container = containers(int(container) - 1);
}
really_inline error_code sax_tweet_reader_visitor::parse_nullable_unsigned(json_iterator &iter, const uint8_t *value, const field &f) {
  iter.log_value(f.key);
  auto i = reinterpret_cast<uint64_t *>(reinterpret_cast<char *>(&tweets.back() + f.offset));
  if (auto error = numberparsing::parse_unsigned(value).get(*i)) {
    // If number parsing failed, check if it's null before returning the error
    if (!atomparsing::is_valid_null_atom(value)) { iter.log_error("expected number or null"); return error; }
    i = 0;
  }
  return SUCCESS;
}
really_inline error_code sax_tweet_reader_visitor::parse_unsigned(json_iterator &iter, const uint8_t *value, const field &f) {
  iter.log_value(f.key);
  auto i = reinterpret_cast<uint64_t *>(reinterpret_cast<char *>(&tweets.back() + f.offset));
  return numberparsing::parse_unsigned(value).get(*i);
}
really_inline error_code sax_tweet_reader_visitor::parse_string(json_iterator &iter, const uint8_t *value, const field &f) {
  iter.log_value(f.key);
  auto s = reinterpret_cast<std::string_view *>(reinterpret_cast<char *>(&tweets.back() + f.offset));
  return stringparsing::parse_string_to_buffer(value, current_string_buf_loc, *s);
}

sax_tweet_reader_visitor::field_lookup sax_tweet_reader_visitor::fields{};

really_inline uint8_t sax_tweet_reader_visitor::field_lookup::hash(const char * key, uint32_t depth) {
  // These shift numbers were chosen specifically because this yields only 2 collisions between
  // keys in twitter.json, leaves 0 as a distinct value, and has 0 collisions between keys we
  // actually care about.
  return uint8_t((key[0] << 0) ^ (key[1] << 3) ^ (key[2] << 3) ^ (key[3] << 1) ^ depth);
}
really_inline sax_tweet_reader_visitor::field sax_tweet_reader_visitor::field_lookup::get(const uint8_t * key, containers c) {
  auto index = hash((const char *)key, uint32_t(c));
  auto entry = entries[index];
  // TODO if any key is > SIMDJSON_PADDING, this will access inaccessible memory!
  if (c != entry.container || memcmp(key, entry.key, entry.len)) { return entries[0]; }
  return entry;
}
really_inline void sax_tweet_reader_visitor::field_lookup::add(const char * key, size_t len, containers c, field_type type, size_t offset) {
  auto index = hash(key, uint32_t(c));
  if (index == 0) {
    fprintf(stderr, "%s (depth %d) hashes to zero, which is used as 'missing value'\n", key, int(c));
    assert(false);
  }
  if (entries[index].key) {
    fprintf(stderr, "%s (depth %d) collides with %s (depth %d) !\n", key, int(c), entries[index].key, int(entries[index].container));
    assert(false);
  }
  entries[index] = { key, len, offset, c, type };
}
really_inline void sax_tweet_reader_visitor::field_lookup::neg(const char * const key, uint32_t depth) {
  auto index = hash(key, depth);
  if (entries[index].key) {
    fprintf(stderr, "%s (depth %d) conflicts with %s (depth %d) !\n", key, depth, entries[index].key, int(entries[index].container));
    assert(false);
  }
}

sax_tweet_reader_visitor::field_lookup::field_lookup() {
  add("\"statuses\"", strlen("\"statuses\""), containers::top_object, field_type::array, 0); // { "statuses": [...]
  #define TWEET_FIELD(KEY, TYPE) add("\"" #KEY "\"", strlen("\"" #KEY "\""), containers::tweet, TYPE, offsetof(tweet, KEY));
  TWEET_FIELD(id, field_type::unsigned_integer);
  TWEET_FIELD(in_reply_to_status_id, field_type::nullable_unsigned_integer);
  TWEET_FIELD(retweet_count, field_type::unsigned_integer);
  TWEET_FIELD(favorite_count, field_type::unsigned_integer);
  TWEET_FIELD(text, field_type::string);
  TWEET_FIELD(created_at, field_type::string);
  TWEET_FIELD(user, field_type::object)
  #undef TWEET_FIELD
  #define USER_FIELD(KEY, TYPE) add("\"" #KEY "\"", strlen("\"" #KEY "\""), containers::user, TYPE, offsetof(tweet, user)+offsetof(twitter_user, KEY));
  USER_FIELD(id, field_type::unsigned_integer);
  USER_FIELD(screen_name, field_type::string);
  #undef USER_FIELD

  // Check for collisions with other (unused) hash keys in typical twitter JSON
  #define NEG(key, depth) neg("\"" #key "\"", depth);
  NEG(display_url, 9);
  NEG(expanded_url, 9);
  neg("\"h\":", 9);
  NEG(indices, 9);
  NEG(resize, 9);
  NEG(url, 9);
  neg("\"w\":", 9);
  NEG(display_url, 8);
  NEG(expanded_url, 8);
  neg("\"h\":", 8);
  NEG(indices, 8);
  NEG(large, 8);
  NEG(medium, 8);
  NEG(resize, 8);
  NEG(small, 8);
  NEG(thumb, 8);
  NEG(url, 8);
  neg("\"w\":", 8);
  NEG(display_url, 7);
  NEG(expanded_url, 7);
  NEG(id_str, 7);
  NEG(id, 7);
  NEG(indices, 7);
  NEG(large, 7);
  NEG(media_url_https, 7);
  NEG(media_url, 7);
  NEG(medium, 7);
  NEG(name, 7);
  NEG(sizes, 7);
  NEG(small, 7);
  NEG(source_status_id_str, 7);
  NEG(source_status_id, 7);
  NEG(thumb, 7);
  NEG(type, 7);
  NEG(url, 7);
  NEG(urls, 7);
  NEG(description, 6);
  NEG(display_url, 6);
  NEG(expanded_url, 6);
  NEG(id_str, 6);
  NEG(id, 6);
  NEG(indices, 6);
  NEG(media_url_https, 6);
  NEG(media_url, 6);
  NEG(name, 6);
  NEG(sizes, 6);
  NEG(source_status_id_str, 6);
  NEG(source_status_id, 6);
  NEG(type, 6);
  NEG(url, 6);
  NEG(urls, 6);
  NEG(contributors_enabled, 5);
  NEG(default_profile_image, 5);
  NEG(default_profile, 5);
  NEG(description, 5);
  NEG(entities, 5);
  NEG(favourites_count, 5);
  NEG(follow_request_sent, 5);
  NEG(followers_count, 5);
  NEG(following, 5);
  NEG(friends_count, 5);
  NEG(geo_enabled, 5);
  NEG(hashtags, 5);
  NEG(id_str, 5);
  NEG(id, 5);
  NEG(is_translation_enabled, 5);
  NEG(is_translator, 5);
  NEG(iso_language_code, 5);
  NEG(lang, 5);
  NEG(listed_count, 5);
  NEG(location, 5);
  NEG(media, 5);
  NEG(name, 5);
  NEG(notifications, 5);
  NEG(profile_background_color, 5);
  NEG(profile_background_image_url_https, 5);
  NEG(profile_background_image_url, 5);
  NEG(profile_background_tile, 5);
  NEG(profile_banner_url, 5);
  NEG(profile_image_url_https, 5);
  NEG(profile_image_url, 5);
  NEG(profile_link_color, 5);
  NEG(profile_sidebar_border_color, 5);
  NEG(profile_sidebar_fill_color, 5);
  NEG(profile_text_color, 5);
  NEG(profile_use_background_image, 5);
  NEG(protected, 5);
  NEG(result_type, 5);
  NEG(statuses_count, 5);
  NEG(symbols, 5);
  NEG(time_zone, 5);
  NEG(url, 5);
  NEG(urls, 5);
  NEG(user_mentions, 5);
  NEG(utc_offset, 5);
  NEG(verified, 5);
  NEG(contributors_enabled, 4);
  NEG(contributors, 4);
  NEG(coordinates, 4);
  NEG(default_profile_image, 4);
  NEG(default_profile, 4);
  NEG(description, 4);
  NEG(entities, 4);
  NEG(favorited, 4);
  NEG(favourites_count, 4);
  NEG(follow_request_sent, 4);
  NEG(followers_count, 4);
  NEG(following, 4);
  NEG(friends_count, 4);
  NEG(geo_enabled, 4);
  NEG(geo, 4);
  NEG(hashtags, 4);
  NEG(id_str, 4);
  NEG(in_reply_to_screen_name, 4);
  NEG(in_reply_to_status_id_str, 4);
  NEG(in_reply_to_user_id_str, 4);
  NEG(in_reply_to_user_id, 4);
  NEG(is_translation_enabled, 4);
  NEG(is_translator, 4);
  NEG(iso_language_code, 4);
  NEG(lang, 4);
  NEG(listed_count, 4);
  NEG(location, 4);
  NEG(media, 4);
  NEG(metadata, 4);
  NEG(name, 4);
  NEG(notifications, 4);
  NEG(place, 4);
  NEG(possibly_sensitive, 4);
  NEG(profile_background_color, 4);
  NEG(profile_background_image_url_https, 4);
  NEG(profile_background_image_url, 4);
  NEG(profile_background_tile, 4);
  NEG(profile_banner_url, 4);
  NEG(profile_image_url_https, 4);
  NEG(profile_image_url, 4);
  NEG(profile_link_color, 4);
  NEG(profile_sidebar_border_color, 4);
  NEG(profile_sidebar_fill_color, 4);
  NEG(profile_text_color, 4);
  NEG(profile_use_background_image, 4);
  NEG(protected, 4);
  NEG(result_type, 4);
  NEG(retweeted, 4);
  NEG(source, 4);
  NEG(statuses_count, 4);
  NEG(symbols, 4);
  NEG(time_zone, 4);
  NEG(truncated, 4);
  NEG(url, 4);
  NEG(urls, 4);
  NEG(user_mentions, 4);
  NEG(utc_offset, 4);
  NEG(verified, 4);
  NEG(contributors, 3);
  NEG(coordinates, 3);
  NEG(entities, 3);
  NEG(favorited, 3);
  NEG(geo, 3);
  NEG(id_str, 3);
  NEG(in_reply_to_screen_name, 3);
  NEG(in_reply_to_status_id_str, 3);
  NEG(in_reply_to_user_id_str, 3);
  NEG(in_reply_to_user_id, 3);
  NEG(lang, 3);
  NEG(metadata, 3);
  NEG(place, 3);
  NEG(possibly_sensitive, 3);
  NEG(retweeted_status, 3);
  NEG(retweeted, 3);
  NEG(source, 3);
  NEG(truncated, 3);
  NEG(completed_in, 2);
  NEG(count, 2);
  NEG(max_id_str, 2);
  NEG(max_id, 2);
  NEG(next_results, 2);
  NEG(query, 2);
  NEG(refresh_url, 2);
  NEG(since_id_str, 2);
  NEG(since_id, 2);
  NEG(search_metadata, 1);
  #undef NEG
}

// sax_tweet_reader_visitor::field_lookup::find_min() {
//   int min_count = 100000;
//   for (int a=0;a<4;a++) {
//     for (int b=0;b<4;b++) {
//       for (int c=0;c<4;c++) {
//         twitter::sax_tweet_reader_visitor::field_lookup fields(a,b,c);
//         if (fields.collision_count) { continue; }
//         if (fields.zero_emission) { continue; }
//         if (fields.conflict_count < min_count) { printf("min=%d,%d,%d (%d)", a, b, c, fields.conflict_count); }
//       }
//     }
//   }
// }

#undef KEY_IS

} // namespace twitter

SIMDJSON_UNTARGET_REGION


SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include <benchmark/benchmark.h>
SIMDJSON_POP_DISABLE_WARNINGS

using namespace benchmark;
using std::cerr;
using std::endl;

const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";
const int REPETITIONS = 10;

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
BENCHMARK(dom_parse)->Repetitions(REPETITIONS)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);

BENCHMARK_MAIN();
