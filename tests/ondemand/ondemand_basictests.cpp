#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <set>
#include <sstream>
#include <utility>
#include <ciso646>
#include <unistd.h>

#include "simdjson.h"
#include "test_ondemand.h"


// const size_t AMAZON_CELLPHONES_NDJSON_DOC_COUNT = 793;
#define SIMDJSON_SHOW_DEFINE(x) printf("%s=%s\n", #x, STRINGIFY(x))

using namespace simdjson;
using namespace simdjson::builtin;

#if SIMDJSON_EXCEPTIONS

// bogus functions for compilation tests
void process1(int ) {}
void process2(int ) {}
void process3(int ) {}

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
#endif

#define ONDEMAND_SUBTEST(NAME, JSON, TEST) \
{ \
  std::cout << "- Subtest " << (NAME) << " - JSON: " << (JSON) << " ..." << std::endl; \
  if (!test_ondemand_doc(JSON##_padded, [&](auto doc) { \
    return (TEST); \
  })) { \
    return false; \
  } \
}


namespace key_string_tests {
#if SIMDJSON_EXCEPTIONS
  bool parser_key_value() {
    TEST_START();
    ondemand::parser parser;
    const padded_string json = R"({ "1": "1", "2": "2", "3": "3", "abc": "abc", "\u0075": "\u0075" })"_padded;
    auto doc = parser.iterate(json);
    for(auto field : doc.get_object())  {
      std::string_view keyv = field.unescaped_key();
      std::string_view valuev = field.value();
      if(keyv != valuev) { return false; }
    }
    return true;
  }
#endif
  bool run() {
    return 
#if SIMDJSON_EXCEPTIONS
      parser_key_value() &&
#endif
      true;
  }

}

namespace active_tests {
#if SIMDJSON_EXCEPTIONS
  bool parser_child() {
    TEST_START();
    ondemand::parser parser;
    const padded_string json = R"({ "parent": {"child1": {"name": "John"} , "child2": {"name": "Daniel"}} })"_padded;
    auto doc = parser.iterate(json);
    ondemand::object parent = doc["parent"];
    {
      ondemand::object c1 = parent["child1"];
      if(std::string_view(c1["name"]) != "John") { return false; } 
    }
    {
      ondemand::object c2 = parent["child2"];
      if(std::string_view(c2["name"]) != "Daniel") { return false; }
    }
    return true;
  }
  bool parser_doc_correct() {
    TEST_START();
    ondemand::parser parser;
    const padded_string json = R"({ "key1": 1, "key2":2, "key3": 3 })"_padded;
    auto doc = parser.iterate(json);
    ondemand::object root_object = doc.get_object();
    int64_t k1 = root_object["key1"];
    int64_t k2 = root_object["key2"];
    int64_t k3 = root_object["key3"];
    return (k1 == 1) && (k2 == 2) && (k3 == 3);
  }

  bool parser_doc_limits() {
    TEST_START();
    ondemand::parser parser;
    const padded_string json = R"({ "key1": 1, "key2":2, "key3": 3 })"_padded;
    auto doc = parser.iterate(json);
    int64_t k1 = doc["key1"];
    try {
      int64_t k2 = doc["key2"];
      (void) k2;
    } catch (simdjson::simdjson_error &) {
      return true; // we expect to fail.
    }
    (void) k1;
    return false;
  }
#endif
  bool run() {
    return 
#if SIMDJSON_EXCEPTIONS
      parser_child() &&
      parser_doc_correct() &&
      parser_doc_limits() &&
#endif
      true;
  }

}
namespace number_tests {

  // ulp distance
  // Marc B. Reynolds, 2016-2019
  // Public Domain under http://unlicense.org, see link for details.
  // adapted by D. Lemire
  inline uint64_t f64_ulp_dist(double a, double b) {
    uint64_t ua, ub;
    memcpy(&ua, &a, sizeof(ua));
    memcpy(&ub, &b, sizeof(ub));
    if ((int64_t)(ub ^ ua) >= 0)
      return (int64_t)(ua - ub) >= 0 ? (ua - ub) : (ub - ua);
    return ua + ub + 0x80000000;
  }

  bool small_integers() {
    std::cout << __func__ << std::endl;
    for (int64_t m = 10; m < 20; m++) {
      for (int64_t i = -1024; i < 1024; i++) {
        return test_ondemand<int64_t>(std::to_string(i), [&](int64_t actual) {
          ASSERT_EQUAL(actual, i);
          return true;
        });
      }
    }
    return true;
  }

  bool powers_of_two() {
    std::cout << __func__ << std::endl;
    char buf[1024];
    uint64_t maxulp = 0;
    for (int i = -1075; i < 1024; ++i) {// large negative values should be zero.
      double expected = pow(2, i);
      size_t n = snprintf(buf, sizeof(buf), "%.*e", std::numeric_limits<double>::max_digits10 - 1, expected);
      if (n >= sizeof(buf)) { abort(); }
      fflush(NULL);
      return test_ondemand<double>(padded_string(buf, n), [&](double actual) {
        uint64_t ulp = f64_ulp_dist(actual,expected);
        if(ulp > maxulp) maxulp = ulp;
        if(ulp > 0) {
          std::cerr << "JSON '" << buf << " parsed to ";
          fprintf( stderr," %18.18g instead of %18.18g\n", actual, expected); // formatting numbers is easier with printf
          SIMDJSON_SHOW_DEFINE(FLT_EVAL_METHOD);
          return false;
        }
        return true;
      });
    }
    return true;
  }

  static const double testing_power_of_ten[] = {
      1e-307, 1e-306, 1e-305, 1e-304, 1e-303, 1e-302, 1e-301, 1e-300, 1e-299,
      1e-298, 1e-297, 1e-296, 1e-295, 1e-294, 1e-293, 1e-292, 1e-291, 1e-290,
      1e-289, 1e-288, 1e-287, 1e-286, 1e-285, 1e-284, 1e-283, 1e-282, 1e-281,
      1e-280, 1e-279, 1e-278, 1e-277, 1e-276, 1e-275, 1e-274, 1e-273, 1e-272,
      1e-271, 1e-270, 1e-269, 1e-268, 1e-267, 1e-266, 1e-265, 1e-264, 1e-263,
      1e-262, 1e-261, 1e-260, 1e-259, 1e-258, 1e-257, 1e-256, 1e-255, 1e-254,
      1e-253, 1e-252, 1e-251, 1e-250, 1e-249, 1e-248, 1e-247, 1e-246, 1e-245,
      1e-244, 1e-243, 1e-242, 1e-241, 1e-240, 1e-239, 1e-238, 1e-237, 1e-236,
      1e-235, 1e-234, 1e-233, 1e-232, 1e-231, 1e-230, 1e-229, 1e-228, 1e-227,
      1e-226, 1e-225, 1e-224, 1e-223, 1e-222, 1e-221, 1e-220, 1e-219, 1e-218,
      1e-217, 1e-216, 1e-215, 1e-214, 1e-213, 1e-212, 1e-211, 1e-210, 1e-209,
      1e-208, 1e-207, 1e-206, 1e-205, 1e-204, 1e-203, 1e-202, 1e-201, 1e-200,
      1e-199, 1e-198, 1e-197, 1e-196, 1e-195, 1e-194, 1e-193, 1e-192, 1e-191,
      1e-190, 1e-189, 1e-188, 1e-187, 1e-186, 1e-185, 1e-184, 1e-183, 1e-182,
      1e-181, 1e-180, 1e-179, 1e-178, 1e-177, 1e-176, 1e-175, 1e-174, 1e-173,
      1e-172, 1e-171, 1e-170, 1e-169, 1e-168, 1e-167, 1e-166, 1e-165, 1e-164,
      1e-163, 1e-162, 1e-161, 1e-160, 1e-159, 1e-158, 1e-157, 1e-156, 1e-155,
      1e-154, 1e-153, 1e-152, 1e-151, 1e-150, 1e-149, 1e-148, 1e-147, 1e-146,
      1e-145, 1e-144, 1e-143, 1e-142, 1e-141, 1e-140, 1e-139, 1e-138, 1e-137,
      1e-136, 1e-135, 1e-134, 1e-133, 1e-132, 1e-131, 1e-130, 1e-129, 1e-128,
      1e-127, 1e-126, 1e-125, 1e-124, 1e-123, 1e-122, 1e-121, 1e-120, 1e-119,
      1e-118, 1e-117, 1e-116, 1e-115, 1e-114, 1e-113, 1e-112, 1e-111, 1e-110,
      1e-109, 1e-108, 1e-107, 1e-106, 1e-105, 1e-104, 1e-103, 1e-102, 1e-101,
      1e-100, 1e-99,  1e-98,  1e-97,  1e-96,  1e-95,  1e-94,  1e-93,  1e-92,
      1e-91,  1e-90,  1e-89,  1e-88,  1e-87,  1e-86,  1e-85,  1e-84,  1e-83,
      1e-82,  1e-81,  1e-80,  1e-79,  1e-78,  1e-77,  1e-76,  1e-75,  1e-74,
      1e-73,  1e-72,  1e-71,  1e-70,  1e-69,  1e-68,  1e-67,  1e-66,  1e-65,
      1e-64,  1e-63,  1e-62,  1e-61,  1e-60,  1e-59,  1e-58,  1e-57,  1e-56,
      1e-55,  1e-54,  1e-53,  1e-52,  1e-51,  1e-50,  1e-49,  1e-48,  1e-47,
      1e-46,  1e-45,  1e-44,  1e-43,  1e-42,  1e-41,  1e-40,  1e-39,  1e-38,
      1e-37,  1e-36,  1e-35,  1e-34,  1e-33,  1e-32,  1e-31,  1e-30,  1e-29,
      1e-28,  1e-27,  1e-26,  1e-25,  1e-24,  1e-23,  1e-22,  1e-21,  1e-20,
      1e-19,  1e-18,  1e-17,  1e-16,  1e-15,  1e-14,  1e-13,  1e-12,  1e-11,
      1e-10,  1e-9,   1e-8,   1e-7,   1e-6,   1e-5,   1e-4,   1e-3,   1e-2,
      1e-1,   1e0,    1e1,    1e2,    1e3,    1e4,    1e5,    1e6,    1e7,
      1e8,    1e9,    1e10,   1e11,   1e12,   1e13,   1e14,   1e15,   1e16,
      1e17,   1e18,   1e19,   1e20,   1e21,   1e22,   1e23,   1e24,   1e25,
      1e26,   1e27,   1e28,   1e29,   1e30,   1e31,   1e32,   1e33,   1e34,
      1e35,   1e36,   1e37,   1e38,   1e39,   1e40,   1e41,   1e42,   1e43,
      1e44,   1e45,   1e46,   1e47,   1e48,   1e49,   1e50,   1e51,   1e52,
      1e53,   1e54,   1e55,   1e56,   1e57,   1e58,   1e59,   1e60,   1e61,
      1e62,   1e63,   1e64,   1e65,   1e66,   1e67,   1e68,   1e69,   1e70,
      1e71,   1e72,   1e73,   1e74,   1e75,   1e76,   1e77,   1e78,   1e79,
      1e80,   1e81,   1e82,   1e83,   1e84,   1e85,   1e86,   1e87,   1e88,
      1e89,   1e90,   1e91,   1e92,   1e93,   1e94,   1e95,   1e96,   1e97,
      1e98,   1e99,   1e100,  1e101,  1e102,  1e103,  1e104,  1e105,  1e106,
      1e107,  1e108,  1e109,  1e110,  1e111,  1e112,  1e113,  1e114,  1e115,
      1e116,  1e117,  1e118,  1e119,  1e120,  1e121,  1e122,  1e123,  1e124,
      1e125,  1e126,  1e127,  1e128,  1e129,  1e130,  1e131,  1e132,  1e133,
      1e134,  1e135,  1e136,  1e137,  1e138,  1e139,  1e140,  1e141,  1e142,
      1e143,  1e144,  1e145,  1e146,  1e147,  1e148,  1e149,  1e150,  1e151,
      1e152,  1e153,  1e154,  1e155,  1e156,  1e157,  1e158,  1e159,  1e160,
      1e161,  1e162,  1e163,  1e164,  1e165,  1e166,  1e167,  1e168,  1e169,
      1e170,  1e171,  1e172,  1e173,  1e174,  1e175,  1e176,  1e177,  1e178,
      1e179,  1e180,  1e181,  1e182,  1e183,  1e184,  1e185,  1e186,  1e187,
      1e188,  1e189,  1e190,  1e191,  1e192,  1e193,  1e194,  1e195,  1e196,
      1e197,  1e198,  1e199,  1e200,  1e201,  1e202,  1e203,  1e204,  1e205,
      1e206,  1e207,  1e208,  1e209,  1e210,  1e211,  1e212,  1e213,  1e214,
      1e215,  1e216,  1e217,  1e218,  1e219,  1e220,  1e221,  1e222,  1e223,
      1e224,  1e225,  1e226,  1e227,  1e228,  1e229,  1e230,  1e231,  1e232,
      1e233,  1e234,  1e235,  1e236,  1e237,  1e238,  1e239,  1e240,  1e241,
      1e242,  1e243,  1e244,  1e245,  1e246,  1e247,  1e248,  1e249,  1e250,
      1e251,  1e252,  1e253,  1e254,  1e255,  1e256,  1e257,  1e258,  1e259,
      1e260,  1e261,  1e262,  1e263,  1e264,  1e265,  1e266,  1e267,  1e268,
      1e269,  1e270,  1e271,  1e272,  1e273,  1e274,  1e275,  1e276,  1e277,
      1e278,  1e279,  1e280,  1e281,  1e282,  1e283,  1e284,  1e285,  1e286,
      1e287,  1e288,  1e289,  1e290,  1e291,  1e292,  1e293,  1e294,  1e295,
      1e296,  1e297,  1e298,  1e299,  1e300,  1e301,  1e302,  1e303,  1e304,
      1e305,  1e306,  1e307,  1e308};



  bool powers_of_ten() {
    std::cout << __func__ << std::endl;
    char buf[1024];

    bool is_pow_correct{1e-308 == std::pow(10,-308)};
    int start_point = is_pow_correct ? -10000 : -307;
    if(!is_pow_correct) {
      std::cout << "On your system, the pow function is busted. Sorry about that. " << std::endl;
    }
    for (int i = start_point; i <= 308; ++i) {// large negative values should be zero.
      size_t n = snprintf(buf, sizeof(buf), "1e%d", i);
      if (n >= sizeof(buf)) { abort(); }
      fflush(NULL);
      double expected = ((i >= -307) ? testing_power_of_ten[i + 307]: std::pow(10, i));
      return test_ondemand<double>(padded_string(buf, n), [&](double actual) {
        int ulp = (int) f64_ulp_dist(actual, expected);
        if(ulp > 0) {
          std::cerr << "JSON '" << buf << " parsed to ";
          fprintf( stderr," %18.18g instead of %18.18g\n", actual, expected); // formatting numbers is easier with printf
          SIMDJSON_SHOW_DEFINE(FLT_EVAL_METHOD);
          return false;
        }
        return true;
      });
    }
    printf("Powers of 10 can be parsed.\n");
    return true;
  }
  bool run() {
    return small_integers() &&
           powers_of_two() &&
           powers_of_ten();
  }
}


namespace parse_api_tests {
  using namespace std;
  using namespace simdjson;
  using namespace simdjson::dom;

  const padded_string BASIC_JSON = "[1,2,3]"_padded;
  const padded_string BASIC_NDJSON = "[1,2,3]\n[4,5,6]"_padded;
  const padded_string EMPTY_NDJSON = ""_padded;

  bool parser_iterate() {
    TEST_START();
    ondemand::parser parser;
    auto doc = parser.iterate(BASIC_JSON);
    ASSERT_SUCCESS( doc.get_array() );
    return true;
  }

#if SIMDJSON_EXCEPTIONS
  bool parser_iterate_exception() {
    TEST_START();
    ondemand::parser parser;
    auto doc = parser.iterate(BASIC_JSON);
    simdjson_unused ondemand::array array = doc;
    return true;
  }
#endif

  bool run() {
    return parser_iterate() &&
#if SIMDJSON_EXCEPTIONS
           parser_iterate_exception() &&
#endif
           true;
  }
}

namespace dom_api_tests {
  using namespace std;
  using namespace simdjson;
  using namespace simdjson::dom;

  bool iterate_object() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c": 3 })"_padded;
    const char* expected_key[] = { "a", "b", "c" };
    uint64_t expected_value[] = { 1, 2, 3 };
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      int i = 0;
      for (auto [ field, error ] : object) {
        ASSERT_SUCCESS(error);
        ASSERT_EQUAL( field.key(), expected_key[i]);
        ASSERT_EQUAL( field.value().get_uint64().first, expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object_result = doc_result.get_object();
      int i = 0;
      for (auto [ field, error ] : object_result) {
        ASSERT_SUCCESS(error);
        ASSERT_EQUAL( field.key(), expected_key[i] );
        ASSERT_EQUAL( field.value().get_uint64().first, expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_array() {
    TEST_START();
    auto json = R"([ 1, 10, 100 ])"_padded;
    uint64_t expected_value[] = { 1, 10, 100 };

    SUBTEST("ondemand::array", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::array array;
      ASSERT_SUCCESS( doc_result.get(array) );
      int i=0;
      for (simdjson_unused auto value : array) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::array>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::array> array = doc_result.get_array();
      int i=0;
      for (simdjson_unused auto value : array) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      int i=0;
      for (simdjson_unused auto value : doc) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      int i=0;
      for (simdjson_unused auto value : doc_result) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_empty_object() {
    TEST_START();
    auto json = R"({})"_padded;

    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      for (simdjson_unused auto field : object) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object_result = doc_result.get_object();
      for (simdjson_unused auto field : object_result) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_empty_array() {
    TEST_START();
    auto json = "[]"_padded;
    SUBTEST("ondemand::array", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::array array;
      ASSERT_SUCCESS( doc_result.get(array) );
      for (simdjson_unused auto value : array) { TEST_FAIL("Unexpected value"); }
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::array>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::array> array_result = doc_result.get_array();
      for (simdjson_unused auto value : array_result) { TEST_FAIL("Unexpected value"); }
      return true;
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      for (simdjson_unused auto value : doc) { TEST_FAIL("Unexpected value"); }
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      for (simdjson_unused auto value : doc_result) { TEST_FAIL("Unexpected value"); }
      return true;
    }));
    TEST_SUCCEED();
  }

  template<typename T>
  bool test_scalar_value(const padded_string &json, const T &expected) {
    cout << "- JSON: " << json << endl;
    SUBTEST( "simdjson_result<document>", test_ondemand_doc(json, [&](auto doc_result) {
      T actual;
      ASSERT_SUCCESS( doc_result.get(actual) );
      ASSERT_EQUAL( expected, actual );
      return true;
    }));
    SUBTEST( "document", test_ondemand_doc(json, [&](auto doc_result) {
      T actual;
      ASSERT_SUCCESS( doc_result.get(actual) );
      ASSERT_EQUAL( expected, actual );
      return true;
    }));
    padded_string array_json = std::string("[") + std::string(json) + "]";
    cout << "- JSON: " << array_json << endl;
    SUBTEST( "simdjson_result<ondemand::value>", test_ondemand_doc(array_json, [&](auto doc_result) {
      int count = 0;
      for (simdjson_result<ondemand::value> val_result : doc_result) {
        T actual;
        ASSERT_SUCCESS( val_result.get(actual) );
        ASSERT_EQUAL(expected, actual);
        count++;
      }
      ASSERT_EQUAL(count, 1);
      return true;
    }));
    SUBTEST( "ondemand::value", test_ondemand_doc(array_json, [&](auto doc_result) {
      int count = 0;
      for (simdjson_result<ondemand::value> val_result : doc_result) {
        ondemand::value val;
        ASSERT_SUCCESS( std::move(val_result).get(val) );
        T actual;
        ASSERT_SUCCESS( val.get(actual) );
        ASSERT_EQUAL(expected, actual);
        count++;
      }
      ASSERT_EQUAL(count, 1);
      return true;
    }));
    TEST_SUCCEED();
  }
  bool string_value() {
    TEST_START();
    return test_scalar_value(R"("hi")"_padded, std::string_view("hi"));
  }

  bool numeric_values() {
    TEST_START();
    if (!test_scalar_value<int64_t> ("0"_padded,   0)) { return false; }
    if (!test_scalar_value<uint64_t>("0"_padded,   0)) { return false; }
    if (!test_scalar_value<double>  ("0"_padded,   0)) { return false; }
    if (!test_scalar_value<int64_t> ("1"_padded,   1)) { return false; }
    if (!test_scalar_value<uint64_t>("1"_padded,   1)) { return false; }
    if (!test_scalar_value<double>  ("1"_padded,   1)) { return false; }
    if (!test_scalar_value<int64_t> ("-1"_padded,  -1)) { return false; }
    if (!test_scalar_value<double>  ("-1"_padded,  -1)) { return false; }
    if (!test_scalar_value<double>  ("1.1"_padded, 1.1)) { return false; }
    TEST_SUCCEED();
  }

  bool boolean_values() {
    TEST_START();
    if (!test_scalar_value<bool> ("true"_padded,  true)) { return false; }
    if (!test_scalar_value<bool> ("false"_padded, false)) { return false; }
    TEST_SUCCEED();
  }

  bool null_value() {
    TEST_START();
    auto json = "null"_padded;
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_EQUAL( doc.is_null(), true );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result.is_null(), true );
      return true;
    }));
    json = "[null]"_padded;
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc_result) {
      int count = 0;
      for (auto value_result : doc_result) {
        ondemand::value value;
        ASSERT_SUCCESS( std::move(value_result).get(value) );
        ASSERT_EQUAL( value.is_null(), true );
        count++;
      }
      ASSERT_EQUAL( count, 1 );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc_result) {
      int count = 0;
      for (auto value_result : doc_result) {
        ASSERT_EQUAL( value_result.is_null(), true );
        count++;
      }
      ASSERT_EQUAL( count, 1 );
      return true;
    }));
    return true;
  }

  bool object_index() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );

      ASSERT_EQUAL( object["a"].get_uint64().first, 1 );
      ASSERT_EQUAL( object["b"].get_uint64().first, 2 );
      ASSERT_EQUAL( object["c/d"].get_uint64().first, 3 );

      ASSERT_ERROR( object["a"], NO_SUCH_FIELD );
      ASSERT_ERROR( object["d"], NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result.get_object()["a"].get_uint64().first, 1 );
      return true;
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_EQUAL( doc["a"].get_uint64().first, 1 );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result["a"].get_uint64().first, 1 );
      return true;
    }));
    TEST_SUCCEED();
  }

#if SIMDJSON_EXCEPTIONS

  bool iterate_object_exception() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c": 3 })"_padded;
    const char* expected_key[] = { "a", "b", "c" };
    uint64_t expected_value[] = { 1, 2, 3 };
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      int i = 0;
      for (ondemand::field field : doc_result.get_object()) {
        ASSERT_EQUAL( field.key(), expected_key[i] );
        ASSERT_EQUAL( uint64_t(field.value()), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_array_exception() {
    TEST_START();
    auto json = R"([ 1, 10, 100 ])"_padded;
    uint64_t expected_value[] = { 1, 10, 100 };

    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      int i=0;
      for (int64_t actual : doc_result) { ASSERT_EQUAL(actual, expected_value[i]); i++; }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_empty_object_exception() {
    TEST_START();
    auto json = R"({})"_padded;

    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      for (simdjson_unused ondemand::field field : doc_result.get_object()) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));

    TEST_SUCCEED();
  }

  bool iterate_empty_array_exception() {
    TEST_START();
    auto json = "[]"_padded;

    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      for (simdjson_unused ondemand::value value : doc_result) { TEST_FAIL("Unexpected value"); }
      return true;
    }));

    TEST_SUCCEED();
  }

  template<typename T>
  bool test_scalar_value_exception(const padded_string &json, const T &expected) {
    cout << "- JSON: " << json << endl;
    SUBTEST( "document", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( expected, T(doc_result) );
      return true;
    }));
    padded_string array_json = std::string("[") + std::string(json) + "]";
    cout << "- JSON: " << array_json << endl;
    SUBTEST( "value", test_ondemand_doc(array_json, [&](auto doc_result) {
      int count = 0;
      for (T actual : doc_result) {
        ASSERT_EQUAL( expected, actual );
        count++;
      }
      ASSERT_EQUAL(count, 1);
      return true;
    }));
    TEST_SUCCEED();
  }
  bool string_value_exception() {
    TEST_START();
    return test_scalar_value_exception(R"("hi")"_padded, std::string_view("hi"));
  }

  bool numeric_values_exception() {
    TEST_START();
    if (!test_scalar_value_exception<int64_t> ("0"_padded,   0)) { return false; }
    if (!test_scalar_value_exception<uint64_t>("0"_padded,   0)) { return false; }
    if (!test_scalar_value_exception<double>  ("0"_padded,   0)) { return false; }
    if (!test_scalar_value_exception<int64_t> ("1"_padded,   1)) { return false; }
    if (!test_scalar_value_exception<uint64_t>("1"_padded,   1)) { return false; }
    if (!test_scalar_value_exception<double>  ("1"_padded,   1)) { return false; }
    if (!test_scalar_value_exception<int64_t> ("-1"_padded,  -1)) { return false; }
    if (!test_scalar_value_exception<double>  ("-1"_padded,  -1)) { return false; }
    if (!test_scalar_value_exception<double>  ("1.1"_padded, 1.1)) { return false; }
    TEST_SUCCEED();
  }

  bool boolean_values_exception() {
    TEST_START();
    if (!test_scalar_value_exception<bool> ("true"_padded,  true)) { return false; }
    if (!test_scalar_value_exception<bool> ("false"_padded, false)) { return false; }
    TEST_SUCCEED();
  }


  bool object_index_exception() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object = doc_result;

      ASSERT_EQUAL( uint64_t(object["a"]), 1 );
      ASSERT_EQUAL( uint64_t(object["b"]), 2 );
      ASSERT_EQUAL( uint64_t(object["c/d"]), 3 );

      return true;
    }));
    TEST_SUCCEED();
  }

#endif

  bool run() {
    return
           iterate_array() &&
           iterate_empty_array() &&
           iterate_object() &&
           iterate_empty_object() &&
           string_value() &&
           numeric_values() &&
           boolean_values() &&
           null_value() &&
           object_index() &&
#if SIMDJSON_EXCEPTIONS
           iterate_object_exception() &&
           iterate_array_exception() &&
           string_value_exception() &&
           numeric_values_exception() &&
           boolean_values_exception() &&
           object_index_exception() &&
#endif
           true;
  }
}


namespace ordering_tests {
  using namespace std;
  using namespace simdjson;
  using namespace simdjson::dom;
#if SIMDJSON_EXCEPTIONS

  auto json = "{\"coordinates\":[{\"x\":1.1,\"y\":2.2,\"z\":3.3}]}"_padded;

  bool in_order() {
    TEST_START();
    ondemand::parser parser{};
    auto doc = parser.iterate(json);
    double x{0};
    double y{0};
    double z{0};
    for (ondemand::object point_object : doc["coordinates"]) {
      x += double(point_object["x"]);
      y += double(point_object["y"]);
      z += double(point_object["z"]);
    }
    return (x == 1.1) && (y == 2.2) && (z == 3.3); 
  }

  bool out_of_order() {
    TEST_START();
    ondemand::parser parser{};
    auto doc = parser.iterate(json);
    double x{0};
    double y{0};
    double z{0};
    for (ondemand::object point_object : doc["coordinates"]) {
      z += double(point_object["z"]);
      try {
        x += double(point_object["x"]);
        return false;
      } catch(simdjson_error&) {}
      try {
        y += double(point_object["y"]);
        return false;
      } catch(simdjson_error&) {}
    }
    return (x == 0) && (y == 0) && (z == 3.3);     
  }

  bool robust_order() {
    TEST_START();
    ondemand::parser parser{};
    auto doc = parser.iterate(json);
    double x{0};
    double y{0};
    double z{0};
    for (ondemand::object point_object : doc["coordinates"]) {
      for (auto field : point_object) {
        if (field.key() == "z") { z += double(field.value()); }
        else if (field.key() == "x") { x += double(field.value()); }
        else if (field.key() == "y") { y += double(field.value()); }
      }
    }
    return (x == 1.1) && (y == 2.2) && (z == 3.3);     
  }
#endif

  bool run() {
    return
#if SIMDJSON_EXCEPTIONS
           in_order() &&
           out_of_order() &&
           robust_order() &&
#endif
           true;
  }

}

namespace twitter_tests {
  using namespace std;
  using namespace simdjson;
  using namespace simdjson::dom;

  bool twitter_count() {
    TEST_START();
    padded_string json;
    ASSERT_SUCCESS( padded_string::load(TWITTER_JSON).get(json) );
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      auto metadata = doc_result["search_metadata"].get_object();
      uint64_t count;
      ASSERT_SUCCESS( metadata["count"].get(count) );
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
      std::string_view screen_name;
      {
        ondemand::object user        = tweet["user"];
        screen_name                  = user["screen_name"];
      }
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
#endif

  bool twitter_default_profile() {
    TEST_START();
    padded_string json;
    ASSERT_SUCCESS( padded_string::load(TWITTER_JSON).get(json) );
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      // Print users with a default profile.
      set<string_view> default_users;
      ondemand::array tweets;
      ASSERT_SUCCESS( doc_result["statuses"].get(tweets) );
      for (auto tweet_value : tweets) {
        auto tweet = tweet_value.get_object();

        ondemand::object user;
        ASSERT_SUCCESS( tweet["user"].get(user) );

        // We have to get the screen name before default_profile because it appears first
        std::string_view screen_name;
        ASSERT_SUCCESS( user["screen_name"].get(screen_name) );

        bool default_profile;
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
      ondemand::array tweets;
      ASSERT_SUCCESS( doc_result["statuses"].get(tweets) );
      for (auto tweet_value : tweets) {
        auto tweet = tweet_value.get_object();
        auto entities = tweet["entities"].get_object();
        ondemand::array media;
        if (entities["media"].get(media) == SUCCESS) {
          for (auto image_value : media) {
            auto image = image_value.get_object();
            auto sizes = image["sizes"].get_object();
            for (auto size : sizes) {
              auto size_value = size.value().get_object();
              uint64_t width, height;
              ASSERT_SUCCESS( size_value["w"].get(width) );
              ASSERT_SUCCESS( size_value["h"].get(height) );
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
      auto metadata = doc_result["search_metadata"].get_object();
      uint64_t count;
      ASSERT_SUCCESS( metadata["count"].get(count) );
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
      auto tweets = doc_result["statuses"];
      for (ondemand::object tweet : tweets) {
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

  bool twitter_image_sizes_exception() {
    TEST_START();
    padded_string json = padded_string::load(TWITTER_JSON);
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      // Print image names and sizes
      set<pair<uint64_t, uint64_t>> image_sizes;
      for (ondemand::object tweet : doc_result["statuses"]) {
        ondemand::object entities = tweet["entities"];
        auto media = entities["media"];
        if (media.error() == SUCCESS) {
          for (ondemand::object image : media) {
            /**
             * Fun fact: id and id_str can differ:
             * 505866668485386240 and 505866668485386241.
             * Presumably, it is because doubles are used
             * at some point in the process and the number
             * 505866668485386241 cannot be represented as a double.
             * (not our fault)
             */
            uint64_t id_val = image["id"].get_uint64();
            std::cout << "id = " <<id_val << std::endl;
            auto id_string = std::string_view(image["id_str"]);
            std::cout << "id_string = " << id_string << std::endl;
            auto sizes = image["sizes"].get_object();
            for (auto size : sizes) {
              /**
               * We want to know the key that describes the size.
               */
              std::string_view raw_size_key_v = size.unescaped_key();
              std::cout << "Type of image size = " << raw_size_key_v << std::endl;
              ondemand::object size_value = size.value();
              int64_t width = size_value["w"];
              int64_t height = size_value["h"];
              std::cout <<  width << " x " << height << std::endl;
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

#endif

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
#endif
           true;
  }
}

namespace error_tests {
  using namespace std;
  using namespace simdjson;
  using namespace simdjson::builtin;

  bool empty_document_error() {
    TEST_START();
    ondemand::parser parser;
    ASSERT_ERROR( parser.iterate(""_padded), EMPTY );
    TEST_SUCCEED();
  }

  namespace wrong_type {

#define TEST_CAST_ERROR(JSON, TYPE, ERROR) \
    std::cout << "- Subtest: get_" << (#TYPE) << "() - JSON: " << (JSON) << std::endl; \
    if (!test_ondemand_doc((JSON##_padded), [&](auto doc_result) { \
      ASSERT_ERROR( doc_result.get_##TYPE(), (ERROR) ); \
      return true; \
    })) { \
      return false; \
    } \
    { \
      padded_string a_json(std::string(R"({ "a": )") + JSON + " })"); \
      std::cout << R"(- Subtest: get_)" << (#TYPE) << "() - JSON: " << a_json << std::endl; \
      if (!test_ondemand_doc(a_json, [&](auto doc_result) { \
        ASSERT_ERROR( doc_result["a"].get_##TYPE(), (ERROR) ); \
        return true; \
      })) { \
        return false; \
      }; \
    }

    bool wrong_type_array() {
      TEST_START();
      TEST_CAST_ERROR("[]", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("[]", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("[]", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("[]", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("[]", double, NUMBER_ERROR);
      TEST_CAST_ERROR("[]", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("[]", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_object() {
      TEST_START();
      TEST_CAST_ERROR("{}", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("{}", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("{}", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("{}", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("{}", double, NUMBER_ERROR);
      TEST_CAST_ERROR("{}", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("{}", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_true() {
      TEST_START();
      TEST_CAST_ERROR("true", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("true", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("true", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("true", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("true", double, NUMBER_ERROR);
      TEST_CAST_ERROR("true", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("true", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_false() {
      TEST_START();
      TEST_CAST_ERROR("false", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("false", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("false", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("false", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("false", double, NUMBER_ERROR);
      TEST_CAST_ERROR("false", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("false", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_null() {
      TEST_START();
      TEST_CAST_ERROR("null", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("null", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("null", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("null", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("null", double, NUMBER_ERROR);
      TEST_CAST_ERROR("null", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("null", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_1() {
      TEST_START();
      TEST_CAST_ERROR("1", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("1", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("1", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("1", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("1", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_negative_1() {
      TEST_START();
      TEST_CAST_ERROR("-1", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("-1", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("-1", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("-1", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("-1", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("-1", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_float() {
      TEST_START();
      TEST_CAST_ERROR("1.1", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("1.1", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("1.1", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("1.1", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("1.1", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("1.1", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("1.1", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_negative_int64_overflow() {
      TEST_START();
      TEST_CAST_ERROR("-9223372036854775809", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("-9223372036854775809", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("-9223372036854775809", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("-9223372036854775809", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("-9223372036854775809", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("-9223372036854775809", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("-9223372036854775809", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_int64_overflow() {
      TEST_START();
      TEST_CAST_ERROR("9223372036854775808", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("9223372036854775808", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("9223372036854775808", bool, INCORRECT_TYPE);
      // TODO BUG: this should be an error but is presently not
      // TEST_CAST_ERROR("9223372036854775808", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("9223372036854775808", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("9223372036854775808", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_uint64_overflow() {
      TEST_START();
      TEST_CAST_ERROR("18446744073709551616", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("18446744073709551616", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("18446744073709551616", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("18446744073709551616", int64, NUMBER_ERROR);
      // TODO BUG: this should be an error but is presently not
      // TEST_CAST_ERROR("18446744073709551616", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("18446744073709551616", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("18446744073709551616", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool run() {
      return
            wrong_type_1() &&
            wrong_type_array() &&
            wrong_type_false() &&
            wrong_type_float() &&
            wrong_type_int64_overflow() &&
            wrong_type_negative_1() &&
            wrong_type_negative_int64_overflow() &&
            wrong_type_null() &&
            wrong_type_object() &&
            wrong_type_true() &&
            wrong_type_uint64_overflow() &&
            true;
    }

  } // namespace wrong_type

  template<typename V, typename T>
  bool assert_iterate(T array, V *expected, size_t N, simdjson::error_code *expected_error, size_t N2) {
    size_t count = 0;
    for (auto elem : std::forward<T>(array)) {
      V actual;
      auto actual_error = elem.get(actual);
      if (count >= N) {
        ASSERT_ERROR(actual_error, expected_error[count - N]);
        ASSERT(count < (N+N2), "Extra error reported");
      } else {
        ASSERT_SUCCESS(actual_error);
        ASSERT_EQUAL(actual, expected[count]);
      }
      count++;
    }
    ASSERT_EQUAL(count, N+N2);
    return true;
  }

  template<typename V, size_t N, size_t N2, typename T>
  bool assert_iterate(T &array, V (&&expected)[N], simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate<V, T&>(array, expected, N, expected_error, N2);
  }

  template<size_t N2, typename T>
  bool assert_iterate(T &array, simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate<int64_t, T&>(array, nullptr, 0, expected_error, N2);
  }

  template<typename V, size_t N, typename T>
  bool assert_iterate(T &array, V (&&expected)[N]) {
    return assert_iterate<V, T&&>(array, expected, N, nullptr, 0);
  }

  template<typename V, size_t N, size_t N2, typename T>
  bool assert_iterate(T &&array, V (&&expected)[N], simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate<V, T&&>(std::forward<T>(array), expected, N, expected_error, N2);
  }

  template<size_t N2, typename T>
  bool assert_iterate(T &&array, simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate<int64_t, T&&>(std::forward<T>(array), nullptr, 0, expected_error, N2);
  }

  template<typename V, size_t N, typename T>
  bool assert_iterate(T &&array, V (&&expected)[N]) {
    return assert_iterate<V, T&&>(std::forward<T>(array), expected, N, nullptr, 0);
  }

  bool top_level_array_iterate_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing comma", "[1 1]",  assert_iterate(doc, { int64_t(1) }, { TAPE_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", "[1,,1]", assert_iterate(doc, { int64_t(1) }, { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", "[,]",    assert_iterate(doc,                 { NUMBER_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", "[,,]",   assert_iterate(doc,                 { NUMBER_ERROR, NUMBER_ERROR, TAPE_ERROR }));
    TEST_SUCCEED();
  }
  bool top_level_array_iterate_unclosed_error() {
    TEST_START();
    ONDEMAND_SUBTEST("unclosed extra comma", "[,", assert_iterate(doc,                 { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", "[1 ",    assert_iterate(doc, { int64_t(1) }, { TAPE_ERROR }));
    // TODO These pass the user values that may run past the end of the buffer if they aren't careful
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed extra comma", "[,,", assert_iterate(doc,                 { NUMBER_ERROR, NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", "[1,",    assert_iterate(doc, { int64_t(1) }, { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", "[1",     assert_iterate(doc,                 { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", "[",      assert_iterate(doc,                 { NUMBER_ERROR, TAPE_ERROR }));
    TEST_SUCCEED();
  }

  bool array_iterate_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing comma", R"({ "a": [1 1] })",  assert_iterate(doc["a"], { int64_t(1) }, { TAPE_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", R"({ "a": [1,,1] })", assert_iterate(doc["a"], { int64_t(1) }, { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", R"({ "a": [1,,] })",  assert_iterate(doc["a"], { int64_t(1) }, { NUMBER_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", R"({ "a": [,] })",    assert_iterate(doc["a"],                 { NUMBER_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", R"({ "a": [,,] })",   assert_iterate(doc["a"],                 { NUMBER_ERROR, NUMBER_ERROR, TAPE_ERROR }));
    TEST_SUCCEED();
  }
  bool array_iterate_unclosed_error() {
    TEST_START();
    ONDEMAND_SUBTEST("unclosed extra comma", R"({ "a": [,)", assert_iterate(doc["a"],                 { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed extra comma", R"({ "a": [,,)", assert_iterate(doc["a"],                 { NUMBER_ERROR, NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", R"({ "a": [1 )",     assert_iterate(doc["a"], { int64_t(1) }, { TAPE_ERROR }));
    // TODO These pass the user values that may run past the end of the buffer if they aren't careful
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed     ", R"({ "a": [1,)",     assert_iterate(doc["a"], { int64_t(1) }, { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", R"({ "a": [1)",      assert_iterate(doc["a"],                 { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", R"({ "a": [)",       assert_iterate(doc["a"],                 { NUMBER_ERROR, TAPE_ERROR }));
    TEST_SUCCEED();
  }

  template<typename V, typename T>
  bool assert_iterate_object(T &&object, const char **expected_key, V *expected, size_t N, simdjson::error_code *expected_error, size_t N2) {
    size_t count = 0;
    for (auto field : object) {
      V actual;
      auto actual_error = field.value().get(actual);
      if (count >= N) {
        ASSERT((count - N) < N2, "Extra error reported");
        ASSERT_ERROR(actual_error, expected_error[count - N]);
      } else {
        ASSERT_SUCCESS(actual_error);
        ASSERT_EQUAL(field.key().first, expected_key[count]);
        ASSERT_EQUAL(actual, expected[count]);
      }
      count++;
    }
    ASSERT_EQUAL(count, N+N2);
    return true;
  }

  template<typename V, size_t N, size_t N2, typename T>
  bool assert_iterate_object(T &&object, const char *(&&expected_key)[N], V (&&expected)[N], simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate_object<V, T>(std::forward<T>(object), expected_key, expected, N, expected_error, N2);
  }

  template<size_t N2, typename T>
  bool assert_iterate_object(T &&object, simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate_object<int64_t, T>(std::forward<T>(object), nullptr, nullptr, 0, expected_error, N2);
  }

  template<typename V, size_t N, typename T>
  bool assert_iterate_object(T &&object, const char *(&&expected_key)[N], V (&&expected)[N]) {
    return assert_iterate_object<V, T>(std::forward<T>(object), expected_key, expected, N, nullptr, 0);
  }

  bool object_iterate_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing colon", R"({ "a"  1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("missing key  ", R"({    : 1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("missing value", R"({ "a":  , "b": 2 })",    assert_iterate_object(doc.get_object(),                          { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("missing comma", R"({ "a": 1  "b": 2 })",    assert_iterate_object(doc.get_object(), { "a" }, { int64_t(1) }, { TAPE_ERROR }));
    TEST_SUCCEED();
  }
  bool object_iterate_wrong_key_type_error() {
    TEST_START();
    ONDEMAND_SUBTEST("wrong key type", R"({ 1:   1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ true: 1, "b": 2 })",   assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ false: 1, "b": 2 })",  assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ null: 1, "b": 2 })",   assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ []:  1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ {}:  1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    TEST_SUCCEED();
  }
  bool object_iterate_unclosed_error() {
    TEST_START();
    ONDEMAND_SUBTEST("unclosed", R"({ "a": 1,         )",    assert_iterate_object(doc.get_object(), { "a" }, { int64_t(1) }, { TAPE_ERROR }));
    // TODO These next two pass the user a value that may run past the end of the buffer if they aren't careful.
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed", R"({ "a": 1          )",    assert_iterate_object(doc.get_object(), { "a" }, { int64_t(1) }, { TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed", R"({ "a":            )",    assert_iterate_object(doc.get_object(),                          { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed", R"({ "a"             )",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed", R"({                 )",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    TEST_SUCCEED();
  }

  bool object_lookup_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing colon", R"({ "a"  1, "b": 2 })",    assert_error(doc["a"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing key  ", R"({    : 1, "b": 2 })",    assert_error(doc["a"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing value", R"({ "a":  , "b": 2 })",    assert_success(doc["a"]));
    ONDEMAND_SUBTEST("missing comma", R"({ "a": 1  "b": 2 })",    assert_success(doc["a"]));
    TEST_SUCCEED();
  }
  bool object_lookup_unclosed_error() {
    TEST_START();
    // TODO This one passes the user a value that may run past the end of the buffer if they aren't careful.
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed", R"({ "a":            )",    assert_success(doc["a"]));
    ONDEMAND_SUBTEST("unclosed", R"({ "a"             )",    assert_error(doc["a"], TAPE_ERROR));
    ONDEMAND_SUBTEST("unclosed", R"({                 )",    assert_error(doc["a"], TAPE_ERROR));
    TEST_SUCCEED();
  }

  bool object_lookup_miss_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing colon", R"({ "a"  1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing key  ", R"({    : 1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing value", R"({ "a":  , "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing comma", R"({ "a": 1  "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    TEST_SUCCEED();
  }
  bool object_lookup_miss_wrong_key_type_error() {
    TEST_START();
    ONDEMAND_SUBTEST("wrong key type", R"({ 1:   1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ true: 1, "b": 2 })",   assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ false: 1, "b": 2 })",  assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ null: 1, "b": 2 })",   assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ []:  1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ {}:  1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    TEST_SUCCEED();
  }
  bool object_lookup_miss_unclosed_error() {
    TEST_START();
    ONDEMAND_SUBTEST("unclosed", R"({ "a": 1,         )",    assert_error(doc["b"], TAPE_ERROR));
    // TODO These next two pass the user a value that may run past the end of the buffer if they aren't careful.
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed", R"({ "a": 1          )",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("unclosed", R"({ "a":            )",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("unclosed", R"({ "a"             )",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("unclosed", R"({                 )",    assert_error(doc["b"], TAPE_ERROR));
    TEST_SUCCEED();
  }
  bool object_lookup_miss_next_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing comma", R"({ "a": 1  "b": 2 })", ([&]() {
      auto obj = doc.get_object();
      return assert_result<int64_t>(obj["a"], 1) && assert_error(obj["b"], TAPE_ERROR);
    })());
    TEST_SUCCEED();
  }

  bool run() {
    return
           empty_document_error() &&
           top_level_array_iterate_error() &&
           top_level_array_iterate_unclosed_error() &&
           array_iterate_error() &&
           array_iterate_unclosed_error() &&
           wrong_type::run() &&
           object_iterate_error() &&
           object_iterate_wrong_key_type_error() &&
           object_iterate_unclosed_error() &&
           object_lookup_error() &&
           object_lookup_unclosed_error() &&
           object_lookup_miss_error() &&
           object_lookup_miss_unclosed_error() &&
           object_lookup_miss_wrong_key_type_error() &&
           object_lookup_miss_next_error() &&
           true;
  }
}

int main(int argc, char *argv[]) {
  std::cout << std::unitbuf;
  int c;
  while ((c = getopt(argc, argv, "a:")) != -1) {
    switch (c) {
    case 'a': {
      const simdjson::implementation *impl = simdjson::available_implementations[optarg];
      if (!impl) {
        fprintf(stderr, "Unsupported architecture value -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      simdjson::active_implementation = impl;
      break;
    }
    default:
      fprintf(stderr, "Unexpected argument %c\n", c);
      return EXIT_FAILURE;
    }
  }

  // this is put here deliberately to check that the documentation is correct (README),
  // should this fail to compile, you should update the documentation:
  if (simdjson::active_implementation->name() == "unsupported") {
    printf("unsupported CPU\n");
  }
  // We want to know what we are testing.
  // Next line would be the runtime dispatched implementation but that's not necessarily what gets tested.
  // std::cout << "Running tests against this implementation: " << simdjson::active_implementation->name();
  // Rather, we want to display builtin_implementation()->name().
  // In practice, by default, we often end up testing against fallback.
  std::cout << "builtin_implementation -- " << builtin_implementation()->name() << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;

  std::cout << "Running basic tests." << std::endl;
  if (
      parse_api_tests::run() &&
      dom_api_tests::run() &&
      twitter_tests::run() &&
      number_tests::run() &&
      error_tests::run() &&
      ordering_tests::run() &&
      key_string_tests::run() &&
      active_tests::run() &&
      true
  ) {
    std::cout << "Basic tests are ok." << std::endl;
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
