#pragma once

#ifdef SIMDJSON_COMPETITION_SAJSON

#include "partial_tweets.h"

namespace partial_tweets {

struct sajson {
  using StringType=std::string_view;

  size_t ast_buffer_size{0};
  size_t *ast_buffer{nullptr};
  ~sajson() { free(ast_buffer); }

  simdjson_inline std::string_view get_string_view(const ::sajson::value &obj, std::string_view key) {
    auto val = obj.get_value_of_key({key.data(), key.length()});
    if (val.get_type() != ::sajson::TYPE_STRING) { throw "field is not a string"; }
    return { val.as_cstring(), val.get_string_length() };
  }
  simdjson_inline uint64_t get_uint52(const ::sajson::value &obj, std::string_view key) {
    auto val = obj.get_value_of_key({key.data(), key.length()});
    switch (val.get_type()) {
      case ::sajson::TYPE_INTEGER: {
        int64_t result;
        if (!val.get_int53_value(&result) || result < 0) { throw "field is not uint52"; }
        return uint64_t(result);
      }
      default:
        throw "field not integer";
    }
  }
  simdjson_inline uint64_t get_str_uint64(const ::sajson::value &obj, std::string_view key) {
    // Since sajson only supports 53-bit numbers, and IDs in twitter.json can be > 53 bits, we read the corresponding id_str and parse that.
    auto val = obj.get_value_of_key({key.data(), key.length()});
    if (val.get_type() != ::sajson::TYPE_STRING) { throw "field not a string"; }
    auto str = val.as_cstring();
    char *endptr;
    uint64_t result = strtoull(str, &endptr, 10);
    if (endptr != &str[val.get_string_length()]) { throw "field is a string, but not an integer string"; }
    return result;
  }
  simdjson_inline uint64_t get_nullable_str_uint64(const ::sajson::value &obj, std::string_view key) {
    auto val = obj.get_value_of_key({key.data(), key.length()});
    if (val.get_type() == ::sajson::TYPE_NULL) { return 0; }
    if (val.get_type() != ::sajson::TYPE_STRING) { throw "field not a string"; }
    auto str = val.as_cstring();
    char *endptr;
    uint64_t result = strtoull(str, &endptr, 10);
    if (endptr != &str[val.get_string_length()]) { throw "field is a string, but not an integer string"; }
    return result;
  }
  simdjson_inline partial_tweets::twitter_user<std::string_view> get_user(const ::sajson::value &obj, std::string_view key) {
    auto user = obj.get_value_of_key({key.data(), key.length()});
    if (user.get_type() != ::sajson::TYPE_OBJECT) { throw "user is not an object"; }
    return { get_str_uint64(user, "id_str"), get_string_view(user, "screen_name") };
  }

  bool run(simdjson::padded_string &json, std::vector<tweet<std::string_view>> &result) {
    if (!ast_buffer) {
      ast_buffer_size = json.size();
      ast_buffer = (size_t *)std::malloc(ast_buffer_size * sizeof(size_t));
    }
    auto doc = ::sajson::parse(
      ::sajson::bounded_allocation(ast_buffer, ast_buffer_size),
      ::sajson::mutable_string_view(json.size(), json.data())
    );
    if (!doc.is_valid()) { return false; }

    auto root = doc.get_root();
    if (root.get_type() != ::sajson::TYPE_OBJECT) { return false; }
    auto statuses = root.get_value_of_key({"statuses", strlen("statuses")});
    if (statuses.get_type() != ::sajson::TYPE_ARRAY) { return false; }

    for (size_t i=0; i<statuses.get_length(); i++) {
      auto tweet = statuses.get_array_element(i);
      if (tweet.get_type() != ::sajson::TYPE_OBJECT) { return false; }
      result.emplace_back(partial_tweets::tweet<std::string_view>{
        get_string_view(tweet, "created_at"),
        get_str_uint64 (tweet, "id_str"),
        get_string_view(tweet, "text"),
        get_nullable_str_uint64(tweet, "in_reply_to_status_id_str"),
        get_user       (tweet, "user"),
        get_uint52     (tweet, "retweet_count"),
        get_uint52     (tweet, "favorite_count")
      });
    }

    return true;
  }
};

BENCHMARK_TEMPLATE(partial_tweets, sajson)->UseManualTime();

} // namespace partial_tweets

#endif // SIMDJSON_COMPETITION_SAJSON

