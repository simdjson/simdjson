#pragma once

#ifdef SIMDJSON_COMPETITION_SAJSON

#include "find_tweet.h"

namespace find_tweet {

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

  bool run(simdjson::padded_string &json, uint64_t find_id, std::string_view &result) {
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
    if (root.get_type() != ::sajson::TYPE_OBJECT) { printf("a\n"); return false; }
    auto statuses = root.get_value_of_key({"statuses", strlen("statuses")});
    if (statuses.get_type() != ::sajson::TYPE_ARRAY) { return false; }

    for (size_t i=0; i<statuses.get_length(); i++) {
      auto tweet = statuses.get_array_element(i);
      if (tweet.get_type() != ::sajson::TYPE_OBJECT) { printf("b\n"); return false; }
      // TODO if there is a way to get the raw string, it might be faster to iota find_id and then
      // compare it to each id_str, instead of parsing each int and comparing to find_id.
      if (get_str_uint64(tweet, "id_str") == find_id) {
        result = get_string_view(tweet, "text");
        return true;
      }
    }

    return false;
  }
};

BENCHMARK_TEMPLATE(find_tweet, sajson)->UseManualTime();

} // namespace find_tweet

#endif // SIMDJSON_COMPETITION_SAJSON

