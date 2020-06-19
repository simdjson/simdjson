#include <benchmark/benchmark.h>
#include "simdjson.h"
#include <sstream>

using namespace simdjson;
using namespace benchmark;
using namespace std;

const padded_string EMPTY_ARRAY("[]", 2);

const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";
const char *NUMBERS_JSON = SIMDJSON_BENCHMARK_DATA_DIR "numbers.json";



static void numbers_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr;
  simdjson::error_code error;
  if (!parser.load(NUMBERS_JSON).get(arr, error)) {
    cerr << "could not read " << NUMBERS_JSON << " as an array: " << error << endl;
    return;
  }
  for (UNUSED auto _ : state) {
    std::vector<double> container;
    for (auto e : arr) {
      double x;
      if (!e.get(x,error)) { cerr << "found a node that is not an number: " << error << endl; break;}
      container.push_back(x);
    }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }  
}
BENCHMARK(numbers_scan);

static void numbers_size_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr;
  simdjson::error_code error;
  if(!parser.load(NUMBERS_JSON).get(arr, error)) {
    cerr << "could not read " << NUMBERS_JSON << " as an array: " << error << endl;
    return;
  }
  for (UNUSED auto _ : state) {
    std::vector<double> container;
    container.resize(arr.size());
    size_t pos = 0;
    for (auto e : arr) {
      double x;
      if (!e.get(x,error)) { cerr << "found a node that is not an number: " << error << endl; break;}
      container[pos++] = x;
    }
    if(pos != container.size()) { cerr << "bad count" << endl; }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }  
}
BENCHMARK(numbers_size_scan);


static void numbers_type_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr;
  simdjson::error_code error;
  if(!parser.load(NUMBERS_JSON).get(arr, error)) {
    cerr << "could not read " << NUMBERS_JSON << " as an array" << endl;
    return;
  }
  for (UNUSED auto _ : state) {
    std::vector<double> container;
    for (auto e : arr) {
      dom::element_type actual_type = e.type();
      if(actual_type != dom::element_type::DOUBLE) {
        cerr << "found a node that is not an number?" << endl; break;
      }
      double x;
      e.get(x,error);
      container.push_back(x);
    }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(numbers_type_scan);

static void numbers_type_size_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr;
  simdjson::error_code error;
  if (!parser.load(NUMBERS_JSON).get(arr, error)) {
    cerr << "could not read " << NUMBERS_JSON << " as an array: " << error << endl;
    return;
  }
  for (UNUSED auto _ : state) {
    std::vector<double> container;
    container.resize(arr.size());
    size_t pos = 0;
    for (auto e : arr) {
      dom::element_type actual_type = e.type();
      if(actual_type != dom::element_type::DOUBLE) {
        cerr << "found a node that is not an number?" << endl; break;
      }
      double x;
      e.get(x,error);
      container[pos++] = x;
    }
    if(pos != container.size()) { cerr << "bad count" << endl; }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(numbers_type_size_scan);

static void numbers_load_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr;
  simdjson::error_code error;
  for (UNUSED auto _ : state) {
    // this may hit the disk, but probably just once
    if (!parser.load(NUMBERS_JSON).get(arr, error)) {
      cerr << "could not read " << NUMBERS_JSON << " as an array: " << error << endl;
      break;
    }
    std::vector<double> container;
    for (auto e : arr) {
      double x;
      if (!e.get(x,error)) { cerr << "found a node that is not an number: " << error << endl; break;}
      container.push_back(x);
    }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }  
}
BENCHMARK(numbers_load_scan);

static void numbers_load_size_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr;
  simdjson::error_code error;
  for (UNUSED auto _ : state) {
    // this may hit the disk, but probably just once
    if(!parser.load(NUMBERS_JSON).get(arr, error)) {
      cerr << "could not read " << NUMBERS_JSON << " as an array" << endl;
      break;
    }
    std::vector<double> container;
    container.resize(arr.size());
    size_t pos = 0;
    for (auto e : arr) {
      double x;
      if(!e.get(x,error)) { cerr << "found a node that is not an number?" << endl; break;}
      container[pos++] = x;
    }
    if(pos != container.size()) { cerr << "bad count" << endl; }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }  
}
BENCHMARK(numbers_load_size_scan);


#if SIMDJSON_EXCEPTIONS


static void numbers_exceptions_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr = parser.load(NUMBERS_JSON);
  for (UNUSED auto _ : state) {
    std::vector<double> container;
    for (double x : arr) {
      container.push_back(x);
    }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }  
}
BENCHMARK(numbers_exceptions_scan);

static void numbers_exceptions_size_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr = parser.load(NUMBERS_JSON);
  for (UNUSED auto _ : state) {
    std::vector<double> container;
    container.resize(arr.size());
    size_t pos = 0;
    for (auto e : arr) {
      container[pos++] = double(e);
    }
    if(pos != container.size()) { cerr << "bad count" << endl; }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }  
}
BENCHMARK(numbers_exceptions_size_scan);



static void numbers_type_exceptions_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr = parser.load(NUMBERS_JSON);
  for (UNUSED auto _ : state) {
    std::vector<double> container;
    for (auto e : arr) {
      dom::element_type actual_type = e.type();
      if(actual_type != dom::element_type::DOUBLE) {
        cerr << "found a node that is not an number?" << endl; break;
      }
      container.push_back(double(e));
    }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(numbers_type_exceptions_scan);

static void numbers_type_exceptions_size_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::array arr = parser.load(NUMBERS_JSON);
  for (UNUSED auto _ : state) {
    std::vector<double> container;
    container.resize(arr.size());
    size_t pos = 0;
    for (auto e : arr) {
      dom::element_type actual_type = e.type();
      if(actual_type != dom::element_type::DOUBLE) {
        cerr << "found a node that is not an number?" << endl; break;
      }
      container[pos++] = double(e);
    }
    if(pos != container.size()) { cerr << "bad count" << endl; }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(numbers_type_exceptions_size_scan);

static void numbers_exceptions_load_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  for (UNUSED auto _ : state) {
    // this may hit the disk, but probably just once
    dom::array arr = parser.load(NUMBERS_JSON);
    std::vector<double> container;
    for (double x : arr) {
      container.push_back(x);
    }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }  
}
BENCHMARK(numbers_exceptions_load_scan);

static void numbers_exceptions_load_size_scan(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  for (UNUSED auto _ : state) {
    // this may hit the disk, but probably just once
    dom::array arr = parser.load(NUMBERS_JSON);
    std::vector<double> container;
    container.resize(arr.size());
    size_t pos = 0;
    for (double x : arr) {
      container[pos++] = x;
    }
    if(pos != container.size()) { cerr << "bad count" << endl; }
    benchmark::DoNotOptimize(container.data());
    benchmark::ClobberMemory();
  }  
}
BENCHMARK(numbers_exceptions_load_size_scan);


static void twitter_count(State& state) {
  // Prints the number of results in twitter.json
  dom::parser parser;
  dom::element doc = parser.load(TWITTER_JSON);
  for (UNUSED auto _ : state) {
    uint64_t result_count = doc["search_metadata"]["count"];
    if (result_count != 100) { return; }
  }
}
BENCHMARK(twitter_count);

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
static void iterator_twitter_count(State& state) {
  // Prints the number of results in twitter.json
  padded_string json = padded_string::load(TWITTER_JSON);
  ParsedJson pj = build_parsed_json(json);
  for (UNUSED auto _ : state) {
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
  dom::element doc = parser.load(TWITTER_JSON);
  for (UNUSED auto _ : state) {
    set<string_view> default_users;
    for (dom::object tweet : doc["statuses"]) {
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
  dom::element doc = parser.load(TWITTER_JSON);
  simdjson::error_code error;
  for (UNUSED auto _ : state) {
    set<tuple<uint64_t, uint64_t>> image_sizes;
    for (dom::object tweet : doc["statuses"]) {
      dom::array media;
      if (tweet["entities"]["media"].get(media, error)) {
        for (dom::object image : media) {
          for (auto size : image["sizes"].get<dom::object>()) {
            image_sizes.insert({ size.value["w"], size.value["h"] });
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
  simdjson::error_code error;
  dom::element doc;
  if (!parser.load(TWITTER_JSON).get(doc, error)) { return; }
  for (UNUSED auto _ : state) {
    uint64_t value;
    if (!doc["search_metadata"]["count"].get(value, error)) { return; }
    if (value != 100) { return; }
  }
}
BENCHMARK(error_code_twitter_count);

static void error_code_twitter_default_profile(State& state) noexcept {
  // Count unique users with a default profile.
  dom::parser parser;
  simdjson::error_code error;
  dom::element doc;
  if (!parser.load(TWITTER_JSON).get(doc, error)) { std::cerr << error << std::endl; return; }
  for (UNUSED auto _ : state) {
    set<string_view> default_users;

    dom::array tweets;
    if (!doc["statuses"].get(tweets, error)) { return; }
    for (dom::element tweet : tweets) {
      dom::object user;
      if (!tweet["user"].get(user, error)) { return; }
      bool default_profile;
      if (!user["default_profile"].get(default_profile, error)) { return; }
      if (default_profile) {
        std::string_view screen_name;
        if (!user["screen_name"].get(screen_name, error)) { return; }
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
  simdjson::padded_string json;
  simdjson::error_code error;
  if (!padded_string::load(TWITTER_JSON).tie(json, error)) { std::cerr << error << std::endl; return; }
  ParsedJson pj = build_parsed_json(json);
  for (UNUSED auto _ : state) {
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
  simdjson::error_code error;
  dom::element doc;
  if (!parser.load(TWITTER_JSON).get(doc, error)) { std::cerr << error << std::endl; return; }
  for (UNUSED auto _ : state) {
    set<tuple<uint64_t, uint64_t>> image_sizes;
    dom::array statuses;
    if (!doc["statuses"].get(statuses, error)) { return; }
    for (dom::element tweet : statuses) {
      dom::array images;
      if (tweet["entities"]["media"].get(images, error)) {
        for (dom::element image : images) {
          dom::object sizes;
          if (!image["sizes"].get(sizes, error)) { return; }
          for (auto size : sizes) {
            uint64_t width, height;
            if (!size.value["w"].get(width, error) || !size.value["h"].get(height, error)) { return; }
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
  simdjson::error_code error;
  padded_string json;
  if (!padded_string::load(TWITTER_JSON).tie(json, error)) { std::cerr << error << std::endl; return; }
  ParsedJson pj = build_parsed_json(json);
  for (UNUSED auto _ : state) {
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
  simdjson::error_code error;
  padded_string json;
  if (!padded_string::load(TWITTER_JSON).tie(json, error)) { std::cerr << error << std::endl; return; }
  dom::parser parser;
  int code = json_parse(json, parser);
  if (code) { cerr << error_message(code) << endl; return; }
  for (UNUSED auto _ : state) {
    std::stringstream s;
    if (!parser.print_json(s)) { cerr << "print_json failed" << endl; return; }
  }
}
BENCHMARK(print_json);
SIMDJSON_POP_DISABLE_WARNINGS

BENCHMARK_MAIN();