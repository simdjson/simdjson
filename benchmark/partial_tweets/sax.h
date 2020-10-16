#pragma once


#include "partial_tweets.h"
#include "sax_tweet_reader_visitor.h"

namespace partial_tweets {

using namespace simdjson;
using namespace simdjson::builtin;
using namespace simdjson::builtin::stage2;

class Sax {
public:
  simdjson_really_inline bool Run(const padded_string &json) noexcept;

  simdjson_really_inline const std::vector<tweet> &Result() { return tweets; }
  simdjson_really_inline size_t ItemCount() { return tweets.size(); }

private:
  simdjson_really_inline error_code RunNoExcept(const padded_string &json) noexcept;
  error_code Allocate(size_t new_capacity);
  std::unique_ptr<uint8_t[]> string_buf{};
  size_t capacity{};
  dom_parser_implementation dom_parser{};
  std::vector<tweet> tweets{};
};

// NOTE: this assumes the dom_parser is already allocated
bool Sax::Run(const padded_string &json) noexcept {
  auto error = RunNoExcept(json);
  if (error) { std::cerr << error << std::endl; return false; }
  return true;
}

error_code Sax::RunNoExcept(const padded_string &json) noexcept {
  tweets.clear();

  // Allocate capacity if needed
  if (capacity < json.size()) {
    SIMDJSON_TRY( Allocate(json.size()) );
  }

  // Run stage 1 first.
  SIMDJSON_TRY( dom_parser.stage1((uint8_t *)json.data(), json.size(), false) );

  // Then walk the document, parsing the tweets as we go
  json_iterator iter(dom_parser, 0);
  sax_tweet_reader_visitor visitor(tweets, string_buf.get());
  SIMDJSON_TRY( iter.walk_document<false>(visitor) );
  return SUCCESS;
}

error_code Sax::Allocate(size_t new_capacity) {
  // string_capacity copied from document::allocate
  size_t string_capacity = SIMDJSON_ROUNDUP_N(5 * new_capacity / 3 + SIMDJSON_PADDING, 64);
  string_buf.reset(new (std::nothrow) uint8_t[string_capacity]);
  if (auto error = dom_parser.set_capacity(new_capacity)) { return error; }
  if (capacity == 0) { // set max depth the first time only
    if (auto error = dom_parser.set_max_depth(DEFAULT_MAX_DEPTH)) { return error; }
  }
  capacity = new_capacity;
  return SUCCESS;
}

BENCHMARK_TEMPLATE(PartialTweets, Sax);

} // namespace partial_tweets

