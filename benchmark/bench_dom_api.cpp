#include <benchmark/benchmark.h>
#include "simdjson.h"
#include <sstream>

using namespace simdjson;
using namespace benchmark;
using namespace std;

#ifndef JSON_TEST_PATH
#define JSON_TEST_PATH "jsonexamples/twitter.json"
#endif

const padded_string EMPTY_ARRAY("[]", 2);

#if SIMDJSON_EXCEPTIONS

static void twitter_count(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::element doc = parser.load(JSON_TEST_PATH);
  for (auto _ : state) {
    uint64_t result_count = doc["search_metadata"]["count"];
    if (result_count != 100) { return; }
  }
}
BENCHMARK(twitter_count);

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
static void iterator_twitter_count(State& state) {
  // Prints the number of results in twitter.json
  padded_string json = padded_string::load(JSON_TEST_PATH);
  ParsedJson pj = build_parsed_json(json);
  for (auto _ : state) {
    ParsedJson::Iterator iter(pj);
    // uint64_t result_count = doc["search_metadata"]["count"];
    if (!iter.move_to_key("search_metadata")) { return; }
    if (!iter.move_to_key("count")) { return; }
    if (!iter.is_integer()) { return; }
    int64_t result_count = iter.get_integer();

    if (result_count != 100) { return; }
  }
}
BENCHMARK(iterator_twitter_count);
SIMDJSON_POP_DISABLE_WARNINGS

static void twitter_default_profile(State& state) {
  // Count unique users with a default profile.
  dom::parser parser;
  dom::element doc = parser.load(JSON_TEST_PATH);
  for (auto _ : state) {
    set<string_view> default_users;
    for (dom::object tweet : doc["statuses"].get<dom::array>()) {
      dom::object user = tweet["user"];
      if (user["default_profile"]) {
        default_users.insert(user["screen_name"]);
      }
    }
    if (default_users.size() != 86) { return; }
  }
}
BENCHMARK(twitter_default_profile);

static void twitter_image_sizes(State& state) {
  // Count unique image sizes
  dom::parser parser;
  dom::element doc = parser.load(JSON_TEST_PATH);
  for (auto _ : state) {
    set<tuple<uint64_t, uint64_t>> image_sizes;
    for (dom::object tweet : doc["statuses"].get<dom::array>()) {
      auto [media, not_found] = tweet["entities"]["media"];
      if (!not_found) {
        for (dom::object image : media.get<dom::array>()) {
          for (auto [key, size] : image["sizes"].get<dom::object>()) {
            image_sizes.insert({ size["w"], size["h"] });
          }
        }
      }
    }
    if (image_sizes.size() != 15) { return; };
  }
}
BENCHMARK(twitter_image_sizes);

#endif // SIMDJSON_EXCEPTIONS

static void error_code_twitter_count(State& state) noexcept {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::element doc = parser.load(JSON_TEST_PATH);
  for (auto _ : state) {
    auto [value, error] = doc["search_metadata"]["count"].get<uint64_t>();
    if (error) { return; }
    if (value != 100) { return; }
  }
}
BENCHMARK(error_code_twitter_count);

static void error_code_twitter_default_profile(State& state) noexcept {
  // Count unique users with a default profile.
  dom::parser parser;
  dom::element doc = parser.load(JSON_TEST_PATH);
  for (auto _ : state) {
    set<string_view> default_users;

    auto [tweets, error] = doc["statuses"].get<dom::array>();
    if (error) { return; }
    for (dom::element tweet : tweets) {
      auto [user, error2] = tweet["user"].get<dom::object>();
      if (error2) { return; }
      auto [default_profile, error3] = user["default_profile"].get<bool>();
      if (error3) { return; }
      if (default_profile) {
        auto [screen_name, error4] = user["screen_name"].get<std::string_view>();
        if (error4) { return; }
        default_users.insert(screen_name);
      }
    }

    if (default_users.size() != 86) { return; }
  }
}
BENCHMARK(error_code_twitter_default_profile);

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
static void iterator_twitter_default_profile(State& state) {
  // Count unique users with a default profile.
  padded_string json = padded_string::load(JSON_TEST_PATH);
  ParsedJson pj = build_parsed_json(json);
  for (auto _ : state) {
    set<string_view> default_users;
    ParsedJson::Iterator iter(pj);

    // for (dom::object tweet : doc["statuses"].get<dom::array>()) {
    if (!(iter.move_to_key("statuses") && iter.is_array())) { return; }
    if (iter.down()) { // first status
      do {

        // dom::object user = tweet["user"];
        if (!(iter.move_to_key("user") && iter.is_object())) { return; }

        // if (user["default_profile"]) {
        if (iter.move_to_key("default_profile")) {
          if (iter.is_true()) {
            if (!iter.up()) { return; } // back to user

            // default_users.insert(user["screen_name"]);
            if (!(iter.move_to_key("screen_name") && iter.is_string())) { return; }
            default_users.insert(string_view(iter.get_string(), iter.get_string_length()));
          }
          if (!iter.up()) { return; } // back to user
        }

        if (!iter.up()) { return; } // back to status

      } while (iter.next()); // next status
    }

    if (default_users.size() != 86) { return; }
  }
}
SIMDJSON_POP_DISABLE_WARNINGS
BENCHMARK(iterator_twitter_default_profile);

static void error_code_twitter_image_sizes(State& state) noexcept {
  // Count unique image sizes
  dom::parser parser;
  dom::element doc = parser.load(JSON_TEST_PATH);
  for (auto _ : state) {
    set<tuple<uint64_t, uint64_t>> image_sizes;
    auto [statuses, error] = doc["statuses"].get<dom::array>();
    if (error) { return; }
    for (dom::element tweet : statuses) {
      auto [images, not_found] = tweet["entities"]["media"].get<dom::array>();
      if (!not_found) {
        for (dom::element image : images) {
          auto [sizes, error2] = image["sizes"].get<dom::object>();
          if (error2) { return; }
          for (auto [key, size] : sizes) {
            auto [width, error3] = size["w"].get<uint64_t>();
            auto [height, error4] = size["h"].get<uint64_t>();
            if (error3 || error4) { return; }
            image_sizes.insert({ width, height });
          }
        }
      }
    }
    if (image_sizes.size() != 15) { return; };
  }
}
BENCHMARK(error_code_twitter_image_sizes);

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
static void iterator_twitter_image_sizes(State& state) {
  // Count unique image sizes
  padded_string json = padded_string::load(JSON_TEST_PATH);
  ParsedJson pj = build_parsed_json(json);
  for (auto _ : state) {
    set<tuple<uint64_t, uint64_t>> image_sizes;
    ParsedJson::Iterator iter(pj);

    // for (dom::object tweet : doc["statuses"].get<dom::array>()) {
    if (!(iter.move_to_key("statuses") && iter.is_array())) { return; }
    if (iter.down()) { // first status
      do {

        // auto [media, not_found] = tweet["entities"]["media"];
        // if (!not_found) {
        if (iter.move_to_key("entities")) {
          if (!iter.is_object()) { return; }
          if (iter.move_to_key("media")) {
            if (!iter.is_array()) { return; }

            //   for (dom::object image : media.get<dom::array>()) {
            if (iter.down()) { // first media
              do {

                // for (auto [key, size] : image["sizes"].get<dom::object>()) {
                if (!(iter.move_to_key("sizes") && iter.is_object())) { return; }
                if (iter.down()) { // first size
                  do {
                    iter.move_to_value();

                    // image_sizes.insert({ size["w"], size["h"] });
                    if (!(iter.move_to_key("w")) && !iter.is_integer()) { return; }
                    uint64_t width = iter.get_integer();
                    if (!iter.up()) { return; } // back to size
                    if (!(iter.move_to_key("h")) && !iter.is_integer()) { return; }
                    uint64_t height = iter.get_integer();
                    if (!iter.up()) { return; } // back to size
                    image_sizes.insert({ width, height });

                  } while (iter.next()); // next size
                  if (!iter.up()) { return; } // back to sizes
                }
                if (!iter.up()) { return; } // back to image
              } while (iter.next()); // next image
              if (!iter.up()) { return; } // back to media
            }
            if (!iter.up()) { return; } // back to entities
          }
          if (!iter.up()) { return; } // back to status
        } 
      } while (iter.next()); // next status
    }

    if (image_sizes.size() != 15) { return; };
  }
}
BENCHMARK(iterator_twitter_image_sizes);

static void print_json(State& state) noexcept {
  // Prints the number of results in twitter.json
  padded_string json = get_corpus(JSON_TEST_PATH);
  dom::parser parser;
  if (int error = json_parse(json, parser); error != SUCCESS) { cerr << error_message(error) << endl; return; }
  for (auto _ : state) {
    std::stringstream s;
    if (!parser.print_json(s)) { cerr << "print_json failed" << endl; return; }
  }
}
BENCHMARK(print_json);
SIMDJSON_POP_DISABLE_WARNINGS

BENCHMARK_MAIN();