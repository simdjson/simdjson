#pragma once

#ifdef SIMDJSON_COMPETITION_SAJSON

#include "distinct_user_id.h"

namespace distinct_user_id {

struct sajson {
  size_t ast_buffer_size{0};
  size_t *ast_buffer{nullptr};
  ~sajson() { free(ast_buffer); }

  simdjson_inline std::string_view get_string_view(const ::sajson::value &obj, std::string_view key) {
    auto val = obj.get_value_of_key({key.data(), key.length()});
    if (val.get_type() != ::sajson::TYPE_STRING) { throw "field is not a string"; }
    return { val.as_cstring(), val.get_string_length() };
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

  bool run(simdjson::padded_string &json, std::vector<uint64_t> &result) {
    using namespace sajson;
    if (!ast_buffer) {
      ast_buffer_size = json.size();
      ast_buffer = (size_t *)std::malloc(ast_buffer_size * sizeof(size_t));
    }
    auto doc = parse(
      bounded_allocation(ast_buffer, ast_buffer_size),
      mutable_string_view(json.size(), json.data())
    );
    if (!doc.is_valid()) { return false; }

    auto root = doc.get_root();
    if (root.get_type() != TYPE_OBJECT) { return false; }
    auto statuses = root.get_value_of_key({"statuses", strlen("statuses")});
    if (statuses.get_type() != TYPE_ARRAY) { return false; }

    for (size_t i=0; i<statuses.get_length(); i++) {
      auto tweet = statuses.get_array_element(i);

      // get tweet.user.id
      if (tweet.get_type() != TYPE_OBJECT) { return false; }
      auto user = tweet.get_value_of_key({"user", strlen("user")});
      if (user.get_type() != TYPE_OBJECT) { return false; }
      result.push_back(get_str_uint64(user, "id_str"));

      // get tweet.retweeted_status.user.id
      auto retweet = tweet.get_value_of_key({"retweeted_status", strlen("retweeted_status")});
      switch (retweet.get_type()) {
        case TYPE_OBJECT: {
          auto retweet_user = retweet.get_value_of_key({"user", strlen("user")});
          if (retweet_user.get_type() != TYPE_OBJECT) { return false; }
          result.push_back(get_str_uint64(retweet_user, "id_str"));
          break;
        }
        // TODO distinguish null and missing. null is bad. missing is fine.
        case TYPE_NULL:
          break;
        default:
          return false;
      }
    }

    return true;
  }
};

BENCHMARK_TEMPLATE(distinct_user_id, sajson)->UseManualTime();

} // namespace distinct_user_id

#endif // SIMDJSON_COMPETITION_SAJSON

