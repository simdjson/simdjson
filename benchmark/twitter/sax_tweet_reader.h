#ifndef TWITTER_SAX_TWEET_READER_H
#define TWITTER_SAX_TWEET_READER_H

#include "simdjson.h"
#include "sax_tweet_reader_visitor.h"
#include "tweet.h"
#include <vector>

SIMDJSON_TARGET_HASWELL

namespace twitter {
namespace {

using namespace simdjson;
using namespace haswell;
using namespace haswell::stage2;

struct sax_tweet_reader {
  std::vector<tweet> tweets;
  std::unique_ptr<uint8_t[]> string_buf;
  size_t capacity;
  dom_parser_implementation dom_parser;

  sax_tweet_reader();
  error_code set_capacity(size_t new_capacity);
  error_code read_tweets(padded_string &json);
}; // struct tweet_reader

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

} // unnamed namespace
} // namespace twitter

SIMDJSON_UNTARGET_REGION

#endif // TWITTER_SAX_TWEET_READER_H