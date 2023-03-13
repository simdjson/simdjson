#include <set>
#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace twitter_tests {
  using namespace std;

  bool twitter_count() {
    TEST_START();
    padded_string json;
    ASSERT_SUCCESS( padded_string::load(TWITTER_JSON).get(json) );
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      uint64_t count;
      ASSERT_SUCCESS( doc_result["search_metadata"]["count"].get(count) );
      ASSERT_EQUAL( count, 100 );
      return true;
    }));
    TEST_SUCCEED();
  }
#if SIMDJSON_EXCEPTIONS
  bool twitter_example() {
    TEST_START();
    padded_string json;
    ASSERT_SUCCESS( padded_string::load(TWITTER_JSON).get(json) );
    ondemand::parser parser;
    auto doc = parser.iterate(json);
    for (ondemand::object tweet : doc["statuses"]) {
      uint64_t         id            = tweet["id"];
      std::string_view text          = tweet["text"];
      std::string_view screen_name   = tweet["user"]["screen_name"];
      uint64_t         retweets      = tweet["retweet_count"];
      uint64_t         favorites     = tweet["favorite_count"];
      (void) id;
      (void) text;
      (void) retweets;
      (void) favorites;
      (void) screen_name;
    }
    TEST_SUCCEED();
  }
#endif // SIMDJSON_EXCEPTIONS

  bool twitter_default_profile() {
    TEST_START();
    padded_string json;
    ASSERT_SUCCESS( padded_string::load(TWITTER_JSON).get(json) );
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      // Print users with a default profile.
      set<string_view> default_users;
      for (auto tweet : doc_result["statuses"]) {
        auto user = tweet["user"].get_object();

        // We have to get the screen name before default_profile because it appears first
        std::string_view screen_name;
        ASSERT_SUCCESS( user["screen_name"].get(screen_name) );

        bool default_profile{};
        ASSERT_SUCCESS( user["default_profile"].get(default_profile) );
        if (default_profile) {
          default_users.insert(screen_name);
        }
      }
      ASSERT_EQUAL( default_users.size(), 86 );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool twitter_image_sizes() {
    TEST_START();
    padded_string json;
    ASSERT_SUCCESS( padded_string::load(TWITTER_JSON).get(json) );
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      // Print image names and sizes
      set<pair<uint64_t, uint64_t>> image_sizes;
      for (auto tweet : doc_result["statuses"]) {
        auto media = tweet["entities"]["media"];
        if (!media.error()) {
          for (auto image : media) {
            uint64_t id_val;
            std::string_view id_string;
            ASSERT_SUCCESS( image["id"].get(id_val) );
            ASSERT_SUCCESS( image["id_str"].get(id_string) );
            std::cout << "id = " << id_val << std::endl;
            std::cout << "id_string = " << id_string << std::endl;

            for (auto size : image["sizes"].get_object()) {
              std::string_view size_key;
              ASSERT_SUCCESS( size.unescaped_key().get(size_key) );
              std::cout << "Type of image size = " << size_key << std::endl;

              uint64_t width, height;
              ASSERT_SUCCESS( size.value()["w"].get(width) );
              ASSERT_SUCCESS( size.value()["h"].get(height) );
              image_sizes.insert(make_pair(width, height));
            }
          }
        }
      }
      ASSERT_EQUAL( image_sizes.size(), 15 );
      return true;
    }));
    TEST_SUCCEED();
  }

#if SIMDJSON_EXCEPTIONS

  bool twitter_count_exception() {
    TEST_START();
    padded_string json;
    ASSERT_SUCCESS( padded_string::load(TWITTER_JSON).get(json) );
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      uint64_t count = doc_result["search_metadata"]["count"];
      ASSERT_EQUAL( count, 100 );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool twitter_default_profile_exception() {
    TEST_START();
    padded_string json = padded_string::load(TWITTER_JSON);
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      // Print users with a default profile.
      set<string_view> default_users;
      for (auto tweet : doc_result["statuses"]) {
        ondemand::object user = tweet["user"];

        // We have to get the screen name before default_profile because it appears first
        std::string_view screen_name = user["screen_name"];
        if (user["default_profile"]) {
          default_users.insert(screen_name);
        }
      }
      ASSERT_EQUAL( default_users.size(), 86 );
      return true;
    }));
    TEST_SUCCEED();
  }

  /*
   * Fun fact: id and id_str can differ:
   * 505866668485386240 and 505866668485386241.
   * Presumably, it is because doubles are used
   * at some point in the process and the number
   * 505866668485386241 cannot be represented as a double.
   * (not our fault)
   */
  bool twitter_image_sizes_exception() {
    TEST_START();
    padded_string json = padded_string::load(TWITTER_JSON);
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      // Print image names and sizes
      set<pair<uint64_t, uint64_t>> image_sizes;
      for (auto tweet : doc_result["statuses"]) {
        auto media = tweet["entities"]["media"];
        if (!media.error()) {
          for (auto image : media) {
            std::cout << "id = " << uint64_t(image["id"]) << std::endl;
            std::cout << "id_string = " << std::string_view(image["id_str"]) << std::endl;
            for (auto size : image["sizes"].get_object()) {
              std::cout << "Type of image size = " << std::string_view(size.unescaped_key()) << std::endl;
              // NOTE: the uint64_t is required so that each value is actually parsed before the pair is created
              image_sizes.insert(make_pair<uint64_t,uint64_t>(size.value()["w"], size.value()["h"]));
            }
          }
        }
      }
      ASSERT_EQUAL( image_sizes.size(), 15 );
      return true;
    }));
    TEST_SUCCEED();
  }

#endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return
           twitter_count() &&
           twitter_default_profile() &&
           twitter_image_sizes() &&
#if SIMDJSON_EXCEPTIONS
           twitter_count_exception() &&
           twitter_example() &&
           twitter_default_profile_exception() &&
           twitter_image_sizes_exception() &&
#endif // SIMDJSON_EXCEPTIONS
           true;
  }

} // namespace twitter_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, twitter_tests::run);
}
