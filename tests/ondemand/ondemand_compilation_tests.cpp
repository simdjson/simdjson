#include <set>
#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

#if SIMDJSON_EXCEPTIONS

// bogus functions for compilation tests
void process1(int64_t ) {}
void process2(int64_t ) {}
void process3(int64_t ) {}

// Do not run this, it is only meant to compile
void compilation_test_1() {
  const padded_string bogus = ""_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(bogus);
  for (ondemand::object my_object : doc["mykey"]) {
      for (auto field : my_object) {
        if (field.key() == "key_value1") { process1(field.value()); }
        else if (field.key() == "key_value2") { process2(field.value()); }
        else if (field.key() == "key_value3") { process3(field.value()); }
      }
    }
}

// Do not run this, it is only meant to compile
 void compilation_test_2() {
  const padded_string bogus = ""_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(bogus);
  std::set<std::string_view> default_users;
  ondemand::array tweets = doc["statuses"].get_array();
  for (auto tweet_value : tweets) {
    auto tweet = tweet_value.get_object();
    ondemand::object user = tweet["user"].get_object();
    std::string_view screen_name = user["screen_name"].get_string();
    bool default_profile = user["default_profile"].get_bool();
    if (default_profile) { default_users.insert(screen_name); }
  }
}


// Do not run this, it is only meant to compile
void compilation_test_3() {
  const padded_string bogus = ""_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(bogus);
  ondemand::array tweets;
  if(! doc["statuses"].get(tweets)) { return; }
  for (auto tweet_value : tweets) {
    auto tweet = tweet_value.get_object();
    for (auto field : tweet) {
      std::string_view key = field.unescaped_key().value();
      std::cout << "key = " << key << std::endl;
      std::string_view val = std::string_view(field.value());
      std::cout << "value (assuming it is a string) = " << val << std::endl;
    }
  }
}

// Do not run this, it is only meant to compile
void compilation_test_4() {
  const padded_string bogus = ""_padded;
  ondemand::parser parser;
  int64_t x1 = (int64_t)parser.iterate(bogus);
  double x2 = (double)parser.iterate(bogus);
  std::string_view x = (std::string_view)parser.iterate(bogus);
  uint64_t x3 = (uint64_t)parser.iterate(bogus);
  bool x4 = (bool)parser.iterate(bogus);
  (void) x1;
  (void) x2;
  (void) x3;
  (void) x4;
  (void) x;
}
#endif // SIMDJSON_EXCEPTIONS

int main(void) {
  return 0;
}
