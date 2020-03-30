#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <set>
#include <string_view>
#include <sstream>
#include <utility>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "simdjson.h"

#ifndef JSON_TEST_PATH
#define JSON_TEST_PATH "jsonexamples/twitter.json"
#endif
#ifndef NDJSON_TEST_PATH
#define NDJSON_TEST_PATH "jsonexamples/amazon_cellphones.ndjson"
#endif

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
    char buf[1024];
    simdjson::dom::parser parser;
    for (int m = 10; m < 20; m++) {
      for (int i = -1024; i < 1024; i++) {
        auto n = sprintf(buf, "%*d", m, i);
        buf[n] = '\0';
        fflush(NULL);

        auto [actual, error] = parser.parse(buf, n).get<int64_t>();
        if (error) { std::cerr << error << std::endl; return false; }
        if (actual != i) {
          std::cerr << "JSON '" << buf << " parsed to " << actual << " instead of " << i << std::endl;
          return false;
        }
      } 
    }
    return true;
  }

  bool powers_of_two() {
    std::cout << __func__ << std::endl;
    char buf[1024];
    simdjson::dom::parser parser;
    int maxulp = 0;
    for (int i = -1075; i < 1024; ++i) {// large negative values should be zero.
      double expected = pow(2, i);
      auto n = sprintf(buf, "%.*e", std::numeric_limits<double>::max_digits10 - 1, expected);
      buf[n] = '\0';
      fflush(NULL);
      auto [actual, error] = parser.parse(buf, n).get<double>();
      if (error) { std::cerr << error << std::endl; return false; }
      int ulp = f64_ulp_dist(actual,expected);  
      if(ulp > maxulp) maxulp = ulp;
      if(ulp > 0) {
        std::cerr << "JSON '" << buf << " parsed to " << actual << " instead of " << expected << std::endl;
        return false;
      }
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
    simdjson::dom::parser parser;
    for (int i = -1000000; i <= 308; ++i) {// large negative values should be zero.
      auto n = sprintf(buf,"1e%d", i);
      buf[n] = '\0';
      fflush(NULL);

      auto [actual, error] = parser.parse(buf, n).get<double>();
      if (error) { std::cerr << error << std::endl; return false; }
      double expected = ((i >= -307) ? testing_power_of_ten[i + 307]: std::pow(10, i));
      int ulp = (int) f64_ulp_dist(actual, expected);
      if(ulp > 0) {
        std::cerr << "JSON '" << buf << " parsed to " << actual << " instead of " << expected << std::endl;
        return false;
      }
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

namespace document_tests {
  // adversarial example that once triggred overruns, see https://github.com/lemire/simdjson/issues/345
  bool bad_example() {
    std::cout << __func__ << std::endl;
    simdjson::padded_string badjson = "[7,7,7,7,6,7,7,7,6,7,7,6,[7,7,7,7,6,7,7,7,6,7,7,6,7,7,7,7,7,7,6"_padded;
    simdjson::dom::parser parser;
    auto [doc, error] = parser.parse(badjson);
    if (!error) {
      printf("This json should not be valid %s.\n", badjson.data());
      return false;
    }
    return true;
  }
  // returns true if successful
  bool stable_test() {
    std::cout << __func__ << std::endl;
    simdjson::padded_string json = "{"
          "\"Image\":{"
              "\"Width\":800,"
              "\"Height\":600,"
              "\"Title\":\"View from 15th Floor\","
              "\"Thumbnail\":{"
              "\"Url\":\"http://www.example.com/image/481989943\","
              "\"Height\":125,"
              "\"Width\":100"
              "},"
              "\"Animated\":false,"
              "\"IDs\":[116,943.3,234,38793]"
            "}"
        "}"_padded;
    simdjson::dom::parser parser;
    std::ostringstream myStream;
    myStream << parser.parse(json);
    std::string newjson = myStream.str();
    if(static_cast<std::string>(json) != newjson) {
      std::cout << "serialized json differs!" << std::endl;
      std::cout << static_cast<std::string>(json) << std::endl;
      std::cout << newjson << std::endl;
    }
    return newjson == static_cast<std::string>(json);
  }
  // returns true if successful
  bool skyprophet_test() {
    std::cout << "Running " << __func__ << std::endl;
    const size_t n_records = 100000;
    std::vector<std::string> data;
    char buf[1024];
    for (size_t i = 0; i < n_records; ++i) {
      auto n = sprintf(buf,
                      "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                      "\"school\": {\"id\": %zu, \"name\": \"school%zu\"}}",
                      i, i, (i % 2) ? "male" : "female", i % 10, i % 10);
      data.emplace_back(std::string(buf, n));
    }
    for (size_t i = 0; i < n_records; ++i) {
      auto n = sprintf(buf, "{\"counter\": %f, \"array\": [%s]}", i * 3.1416,
                      (i % 2) ? "true" : "false");
      data.emplace_back(std::string(buf, n));
    }
    for (size_t i = 0; i < n_records; ++i) {
      auto n = sprintf(buf, "{\"number\": %e}", i * 10000.31321321);
      data.emplace_back(std::string(buf, n));
    }
    data.emplace_back(std::string("true"));
    data.emplace_back(std::string("false"));
    data.emplace_back(std::string("null"));
    data.emplace_back(std::string("0.1"));
    size_t maxsize = 0;
    for (auto &s : data) {
      if (maxsize < s.size())
        maxsize = s.size();
    }
    simdjson::dom::parser parser;
    size_t counter = 0;
    for (auto &rec : data) {
      if ((counter % 10000) == 0) {
        printf(".");
        fflush(NULL);
      }
      counter++;
      auto [doc1, res1] = parser.parse(rec.c_str(), rec.length());
      if (res1 != simdjson::error_code::SUCCESS) {
        printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
        printf("Parsing failed. Error is %s\n", simdjson::error_message(res1));
        return false;
      }
      auto [doc2, res2] = parser.parse(rec.c_str(), rec.length());
      if (res2 != simdjson::error_code::SUCCESS) {
        printf("Something is wrong in skyprophet_test: %s.\n", rec.c_str());
        printf("Parsing failed. Error is %s\n", simdjson::error_message(res2));
        return false;
      }
    }
    printf("\n");
    return true;
  }
  bool lots_of_brackets() {
    std::string input;
    for(size_t i = 0; i < 200; i++) {
      input += "[";
    }
    for(size_t i = 0; i < 200; i++) {
      input += "]";
    }
    simdjson::dom::parser parser;
    auto [doc, error] = parser.parse(input);
    if (error) { std::cerr << "Error: " << simdjson::error_message(error) << std::endl; return false; }
    return true;
  }
  bool run() {
    return bad_example() &&
           stable_test() &&
           skyprophet_test() &&
           lots_of_brackets();
  }
}

namespace document_stream_tests {

  static simdjson::dom::document_stream parse_many_stream_return(simdjson::dom::parser &parser, simdjson::padded_string &str) {
    return parser.parse_many(str);
  }
  // this is a compilation test
  UNUSED static void parse_many_stream_assign() {
      simdjson::dom::parser parser;
      simdjson::padded_string str("{}",2);
      simdjson::dom::document_stream s1 = parse_many_stream_return(parser, str);
  }

  static bool parse_json_message_issue467(simdjson::padded_string &json, size_t expectedcount) {
    simdjson::dom::parser parser;
    size_t count = 0;
    for (auto [doc, error] : parser.parse_many(json)) {
      if (error) {
          std::cerr << "Failed with simdjson error= " << error << std::endl;
          return false;
      }
      count++;
    }
    if(count != expectedcount) {
        std::cerr << "bad count" << std::endl;
        return false;
    }
    return true;
  }

  bool json_issue467() {
    std::cout << "Running " << __func__ << std::endl;
    auto single_message = R"({"error":[],"result":{"token":"xxx"}})"_padded;
    auto two_messages = R"({"error":[],"result":{"token":"xxx"}}{"error":[],"result":{"token":"xxx"}})"_padded;

    if(!parse_json_message_issue467(single_message, 1)) {
      return false;
    }
    if(!parse_json_message_issue467(two_messages, 2)) {
      return false;
    }
    return true;
  }

  // returns true if successful
  bool document_stream_test() {
    std::cout << "Running " << __func__ << std::endl;
    fflush(NULL);
    const size_t n_records = 10000;
    std::string data;
    char buf[1024];
    for (size_t i = 0; i < n_records; ++i) {
      auto n = sprintf(buf,
                      "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                      "\"ete\": {\"id\": %zu, \"name\": \"eventail%zu\"}}",
                      i, i, (i % 2) ? "homme" : "femme", i % 10, i % 10);
      data += std::string(buf, n);
    }
    for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
      printf(".");
      fflush(NULL);
      simdjson::padded_string str(data);
      simdjson::dom::parser parser;
      size_t count = 0;
      for (auto [doc, error] : parser.parse_many(str, batch_size)) {
        if (error) {
          printf("Error at on document %zd at batch size %zu: %s\n", count, batch_size, simdjson::error_message(error));
          return false;
        }

        auto [keyid, error2] = doc["id"].get<int64_t>();
        if (error2) {
          printf("Error getting id as int64 on document %zd at batch size %zu: %s\n", count, batch_size, simdjson::error_message(error2));
          return false;
        }

        if (keyid != int64_t(count)) {
          printf("key does not match %" PRId64 ", expected %zd on document %zd at batch size %zu\n", keyid, count, count, batch_size);
          return false;
        }

        count++;
      }
      if(count != n_records) {
        printf("Found wrong number of documents %zd, expected %zd at batch size %zu\n", count, n_records, batch_size);
        return false;
      }
    }
    printf("ok\n");
    return true;
  }

  // returns true if successful
  bool document_stream_utf8_test() {
    std::cout << "Running " << __func__ << std::endl;
    fflush(NULL);
    const size_t n_records = 10000;
    std::string data;
    char buf[1024];
    for (size_t i = 0; i < n_records; ++i) {
      auto n = sprintf(buf,
                      "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                      "\"été\": {\"id\": %zu, \"name\": \"éventail%zu\"}}",
                      i, i, (i % 2) ? "⺃" : "⺕", i % 10, i % 10);
      data += std::string(buf, n);
    }
    for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
      printf(".");
      fflush(NULL);
      simdjson::padded_string str(data);
      simdjson::dom::parser parser;
      size_t count = 0;
      for (auto [doc, error] : parser.parse_many(str, batch_size)) {
        if (error) {
          printf("Error at on document %zd at batch size %zu: %s\n", count, batch_size, simdjson::error_message(error));
          return false;
        }

        auto [keyid, error2] = doc["id"].get<int64_t>();
        if (error2) {
          printf("Error getting id as int64 on document %zd at batch size %zu: %s\n", count, batch_size, simdjson::error_message(error2));
          return false;
        }

        if (keyid != int64_t(count)) {
          printf("key does not match %" PRId64 ", expected %zd on document %zd at batch size %zu\n", keyid, count, count, batch_size);
          return false;
        }

        count++;
      }
      if(count != n_records) {
        printf("Found wrong number of documents %zd, expected %zd at batch size %zu\n", count, n_records, batch_size);
        return false;
      }
    }
    printf("ok\n");
    return true;
  }

  bool run() {
    return document_stream_test() &&
           document_stream_utf8_test();
  }
}

namespace parse_api_tests {
  using namespace std;
  using namespace simdjson;
  using namespace simdjson::dom;

  const padded_string BASIC_JSON = string("[1,2,3]");
  const padded_string BASIC_NDJSON = string("[1,2,3]\n[4,5,6]");
  // const padded_string EMPTY_NDJSON = string("");

  bool parser_parse() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [doc, error] = parser.parse(BASIC_JSON);
    if (error) { cerr << error << endl; return false; }
    if (!doc.is<dom::array>()) { cerr << "Document did not parse as an array" << endl; return false; }
    return true;
  }
  bool parser_parse_many() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    int count = 0;
    for (auto [doc, error] : parser.parse_many(BASIC_NDJSON)) {
      if (error) { cerr << error << endl; return false; }
      if (!doc.is<dom::array>()) { cerr << "Document did not parse as an array" << endl; return false; }
      count++;
    }
    if (count != 2) { cerr << "parse_many returned " << count << " documents, expected 2" << endl; return false; }
    return true;
  }
  // bool parser_parse_many_empty() {
  //   std::cout << "Running " << __func__ << std::endl;
  //   dom::parser parser;
  //   int count = 0;
  //   for (auto [doc, error] : parser.parse_many(EMPTY_NDJSON)) {
  //     if (error) { cerr << error << endl; return false; }
  //     count++;
  //   }
  //   if (count != 0) { cerr << "parse_many returned " << count << " documents, expected 0" << endl; return false; }
  //   return true;
  // }

  bool parser_load() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [doc, error] = parser.load(JSON_TEST_PATH);
    if (error) { cerr << error << endl; return false; }
    if (!doc.is<dom::object>()) { cerr << "Document did not parse as an object" << endl; return false; }
    return true;
  }
  bool parser_load_many() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    int count = 0;
    for (auto [doc, error] : parser.load_many(NDJSON_TEST_PATH)) {
      if (error) { cerr << error << endl; return false; }
      if (!doc.is<dom::array>()) { cerr << "Document did not parse as an array" << endl; return false; }
      count++;
    }
    if (count != 793) { cerr << "Expected 793 documents, but load_many loaded " << count << " documents." << endl; return false; }
    return true;
  }

#if SIMDJSON_EXCEPTIONS

  bool parser_parse_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(BASIC_JSON);
    if (!doc.is<dom::array>()) { cerr << "Document did not parse as an array" << endl; return false; }
    return true;
  }
  bool parser_parse_many_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    int count = 0;
    for (const element doc : parser.parse_many(BASIC_NDJSON)) {
      if (!doc.is<dom::array>()) { cerr << "Document did not parse as an array" << endl; return false; }
      count++;
    }
    if (count != 2) { cerr << "parse_many returned " << count << " documents, expected 2" << endl; return false; }
    return true;
  }

  bool parser_load_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    const element doc = parser.load(JSON_TEST_PATH);
    if (!doc.is<dom::object>()) { cerr << "Document did not parse as an object" << endl; return false; }
    return true;
  }
  bool parser_load_many_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    int count = 0;
    for (const element doc : parser.load_many(NDJSON_TEST_PATH)) {
      if (!doc.is<dom::array>()) { cerr << "Document did not parse as an array" << endl; return false; }
      count++;
    }
    if (count != 793) { cerr << "Expected 1 document, but load_many loaded " << count << " documents." << endl; return false; }
    return true;
  }
#endif

  bool run() {
    return parser_parse() &&
           parser_parse_many() &&
//           parser_parse_many_empty() &&
           parser_load() &&
           parser_load_many() &&
#if SIMDJSON_EXCEPTIONS
           parser_parse_exception() &&
           parser_parse_many_exception() &&
           parser_load_exception() &&
           parser_load_many_exception() &&
#endif
           true;
  }
}

namespace dom_api_tests {
  using namespace std;
  using namespace simdjson;
  using namespace simdjson::dom;

  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING
  // returns true if successful
  bool ParsedJson_Iterator_test() {
    std::cout << "Running " << __func__ << std::endl;
    simdjson::padded_string json = R"({
          "Image": {
              "Width":  800,
              "Height": 600,
              "Title":  "View from 15th Floor",
              "Thumbnail": {
                  "Url":    "http://www.example.com/image/481989943",
                  "Height": 125,
                  "Width":  100
              },
              "Animated" : false,
              "IDs": [116, 943, 234, 38793]
            }
        })"_padded;
    simdjson::ParsedJson pj = build_parsed_json(json);
    if (pj.error) {
      printf("Could not parse '%s': %s\n", json.data(), simdjson::error_message(pj.error));
      return false;
    }
    simdjson::ParsedJson::Iterator iter(pj);
    if (!iter.is_object()) {
      printf("Root should be object\n");
      return false;
    }
    if (iter.move_to_key("bad key")) {
      printf("We should not move to a non-existing key\n");
      return false;
    }
    if (!iter.is_object()) {
      printf("We should have remained at the object.\n");
      return false;
    }
    if (iter.move_to_key_insensitive("bad key")) {
      printf("We should not move to a non-existing key\n");
      return false;    
    }
    if (!iter.is_object()) {
      printf("We should have remained at the object.\n");
      return false;
    }
    if (!iter.down()) {
      printf("Root should not be emtpy\n");
      return false;
    }
    if (!iter.is_string()) {
      printf("Object should start with string key\n");
      return false;
    }
    if (iter.prev()) {
      printf("We should not be able to go back from the start of the scope.\n");
      return false;
    }
    if (strcmp(iter.get_string(),"Image")!=0) {
      printf("There should be a single key, image.\n");
      return false;
    }
    iter.move_to_value();
    if(!iter.is_object()) {
      printf("Value of image should be object\n");
      return false;
    }
    if(!iter.down()) {
      printf("Image key should not be emtpy\n");
      return false;
    }
    if(!iter.next()) {
      printf("key should have a value\n");
      return false;
    }
    if(!iter.prev()) {
      printf("We should go back to the key.\n");
      return false;
    }
    if (strcmp(iter.get_string(),"Width")!=0) {
      printf("There should be a  key Width.\n");
      return false;
    }
    if (!iter.up()) {
      return false;
    }
    if (!iter.move_to_key("IDs")) {
      printf("We should be able to move to an existing key\n");
      return false;    
    }
    if (!iter.is_array()) {
      printf("Value of IDs should be array, it is %c \n", iter.get_type());
      return false;
    }
    if (iter.move_to_index(4)) {
      printf("We should not be able to move to a non-existing index\n");
      return false;    
    }
    if (!iter.is_array()) {
      printf("We should have remained at the array\n");
      return false;
    }
    return true;
  }
  SIMDJSON_POP_DISABLE_WARNINGS

  bool object_iterator() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"({ "a": 1, "b": 2, "c": 3 })");
    const char* expected_key[] = { "a", "b", "c" };
    uint64_t expected_value[] = { 1, 2, 3 };
    int i = 0;

    dom::parser parser;
    auto [object, error] = parser.parse(json).get<dom::object>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    for (auto [key, value] : object) {
      if (key != expected_key[i] || value.get<uint64_t>().first != expected_value[i]) { cerr << "Expected " << expected_key[i] << " = " << expected_value[i] << ", got " << key << "=" << value << endl; return false; }
      i++;
    }
    if (i*sizeof(uint64_t) != sizeof(expected_value)) { cout << "Expected " << sizeof(expected_value) << " values, got " << i << endl; return false; }
    return true;
  }

  bool array_iterator() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ 1, 10, 100 ])");
    uint64_t expected_value[] = { 1, 10, 100 };
    int i=0;

    dom::parser parser;
    auto [array, error] = parser.parse(json).get<dom::array>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    for (auto value : array) {
      if (value.get<uint64_t>().first != expected_value[i]) { cerr << "Expected " << expected_value[i] << ", got " << value << endl; return false; }
      i++;
    }
    if (i*sizeof(uint64_t) != sizeof(expected_value)) { cout << "Expected " << sizeof(expected_value) << " values, got " << i << endl; return false; }
    return true;
  }

  bool object_iterator_empty() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"({})");
    int i = 0;

    dom::parser parser;
    auto [object, error] = parser.parse(json).get<dom::object>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    for (auto [key, value] : object) {
      cout << "Unexpected " << key << " = " << value << endl;
      i++;
    }
    if (i > 0) { cout << "Expected 0 values, got " << i << endl; return false; }
    return true;
  }

  bool array_iterator_empty() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([])");
    int i=0;

    dom::parser parser;
    auto [array, error] = parser.parse(json).get<dom::array>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    for (auto value : array) {
      cout << "Unexpected value " << value << endl;
      i++;
    }
    if (i > 0) { cout << "Expected 0 values, got " << i << endl; return false; }
    return true;
  }

  bool string_value() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ "hi", "has backslash\\" ])");
    dom::parser parser;
    auto [array, error] = parser.parse(json).get<dom::array>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    auto val = array.begin();

    if ((*val).get<std::string_view>().first != "hi") { cerr << "Expected value to be \"hi\", was " << (*val).get<std::string_view>().first << endl; return false; }
    ++val;
    if ((*val).get<std::string_view>().first != "has backslash\\") { cerr << "Expected string_view(\"has backslash\\\\\") to be \"has backslash\\\", was " << (*val).get<std::string_view>().first << endl; return false; }
    return true;
  }

  bool numeric_values() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ 0, 1, -1, 1.1 ])");
    dom::parser parser;
    auto [array, error] = parser.parse(json).get<dom::array>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    auto val = array.begin();

    if ((*val).get<uint64_t>().first != 0) { cerr << "Expected uint64_t(0) to be 0, was " << (*val) << endl; return false; }
    if ((*val).get<int64_t>().first != 0) { cerr << "Expected int64_t(0) to be 0, was " << (*val).get<int64_t>().first << endl; return false; }
    if ((*val).get<double>().first != 0) { cerr << "Expected double(0) to be 0, was " << (*val).get<double>().first << endl; return false; }
    ++val;
    if ((*val).get<uint64_t>().first != 1) { cerr << "Expected uint64_t(1) to be 1, was " << (*val) << endl; return false; }
    if ((*val).get<int64_t>().first != 1) { cerr << "Expected int64_t(1) to be 1, was " << (*val).get<int64_t>().first << endl; return false; }
    if ((*val).get<double>().first != 1) { cerr << "Expected double(1) to be 1, was " << (*val).get<double>().first << endl; return false; }
    ++val;
    if ((*val).get<int64_t>().first != -1) { cerr << "Expected int64_t(-1) to be -1, was " << (*val).get<int64_t>().first << endl; return false; }
    if ((*val).get<double>().first != -1) { cerr << "Expected double(-1) to be -1, was " << (*val).get<double>().first << endl; return false; }
    ++val;
    if ((*val).get<double>().first != 1.1) { cerr << "Expected double(1.1) to be 1.1, was " << (*val).get<double>().first << endl; return false; }
    return true;
  }

  bool boolean_values() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ true, false ])");
    dom::parser parser;
    auto [array, error] = parser.parse(json).get<dom::array>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    auto val = array.begin();

    if ((*val).get<bool>().first != true) { cerr << "Expected bool(true) to be true, was " << (*val) << endl; return false; }
    ++val;
    if ((*val).get<bool>().first != false) { cerr << "Expected bool(false) to be false, was " << (*val) << endl; return false; }
    return true;
  }

  bool null_value() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ null ])");
    dom::parser parser;
    auto [array, error] = parser.parse(json).get<dom::array>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    auto val = array.begin();
    if (!(*val).is_null()) { cerr << "Expected null to be null!" << endl; return false; }
    return true;
  }

  bool document_object_index() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"({ "a": 1, "b": 2, "c/d": 3})");
    dom::parser parser;
    auto [doc, error] = parser.parse(json);
    if (doc["a"].get<uint64_t>().first != 1) { cerr << "Expected uint64_t(doc[\"a\"]) to be 1, was " << doc["a"].first << endl; return false; }
    if (doc["b"].get<uint64_t>().first != 2) { cerr << "Expected uint64_t(doc[\"b\"]) to be 2, was " << doc["b"].first << endl; return false; }
    if (doc["c/d"].get<uint64_t>().first != 3) { cerr << "Expected uint64_t(doc[\"c/d\"]) to be 3, was " << doc["c"].first << endl; return false; }
    // Check all three again in backwards order, to ensure we can go backwards
    if (doc["c/d"].get<uint64_t>().first != 3) { cerr << "Expected uint64_t(doc[\"c/d\"]) to be 3, was " << doc["c"].first << endl; return false; }
    if (doc["b"].get<uint64_t>().first != 2) { cerr << "Expected uint64_t(doc[\"b\"]) to be 2, was " << doc["b"].first << endl; return false; }
    if (doc["a"].get<uint64_t>().first != 1) { cerr << "Expected uint64_t(doc[\"a\"]) to be 1, was " << doc["a"].first << endl; return false; }

    UNUSED element val;
    // tie(val, error) = doc["d"]; fails with "no viable overloaded '='" on Apple clang version 11.0.0	    tie(val, error) = doc["d"];
    doc["d"].tie(val, error);
    if (error != simdjson::NO_SUCH_FIELD) { cerr << "Expected NO_SUCH_FIELD error for uint64_t(doc[\"d\"]), got " << error << endl; return false; }
    return true;
  }

  bool object_index() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"({ "obj": { "a": 1, "b": 2, "c/d": 3 } })");
    dom::parser parser;
    auto [doc, error] = parser.parse(json);
    if (error) { cerr << "Error: " << error << endl; return false; }
    if (doc["obj"]["a"].get<uint64_t>().first != 1) { cerr << "Expected uint64_t(doc[\"obj\"][\"a\"]) to be 1, was " << doc["obj"]["a"].first << endl; return false; }

    object obj;
    doc.get<dom::object>().tie(obj, error); //  tie(...) = fails with "no viable overloaded '='" on Apple clang version 11.0.0
    if (error) { cerr << "Error: " << error << endl; return false; }
    if (obj["obj"]["a"].get<uint64_t>().first != 1) { cerr << "Expected uint64_t(doc[\"obj\"][\"a\"]) to be 1, was " << doc["obj"]["a"].first << endl; return false; }

    obj["obj"].get<dom::object>().tie(obj, error); //  tie(...) = fails with "no viable overloaded '='" on Apple clang version 11.0.0
    if (obj["a"].get<uint64_t>().first != 1) { cerr << "Expected uint64_t(obj[\"a\"]) to be 1, was " << obj["a"].first << endl; return false; }
    if (obj["b"].get<uint64_t>().first != 2) { cerr << "Expected uint64_t(obj[\"b\"]) to be 2, was " << obj["b"].first << endl; return false; }
    if (obj["c/d"].get<uint64_t>().first != 3) { cerr << "Expected uint64_t(obj[\"c\"]) to be 3, was " << obj["c"].first << endl; return false; }
    // Check all three again in backwards order, to ensure we can go backwards
    if (obj["c/d"].get<uint64_t>().first != 3) { cerr << "Expected uint64_t(obj[\"c\"]) to be 3, was " << obj["c"].first << endl; return false; }
    if (obj["b"].get<uint64_t>().first != 2) { cerr << "Expected uint64_t(obj[\"b\"]) to be 2, was " << obj["b"].first << endl; return false; }
    if (obj["a"].get<uint64_t>().first != 1) { cerr << "Expected uint64_t(obj[\"a\"]) to be 1, was " << obj["a"].first << endl; return false; }

    UNUSED element val;
    doc["d"].tie(val, error); // tie(...) = fails with "no viable overloaded '='" on Apple clang version 11.0.0;
    if (error != simdjson::NO_SUCH_FIELD) { cerr << "Expected NO_SUCH_FIELD error for uint64_t(obj[\"d\"]), got " << error << endl; return false; }
    return true;
  }

  bool twitter_count() {
    std::cout << "Running " << __func__ << std::endl;
    // Prints the number of results in twitter.json
    dom::parser parser;
    auto [result_count, error] = parser.load(JSON_TEST_PATH)["search_metadata"]["count"].get<uint64_t>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    if (result_count != 100) { cerr << "Expected twitter.json[metadata_count][count] = 100, got " << result_count << endl; return false; }
    return true;
  }

  bool twitter_default_profile() {
    std::cout << "Running " << __func__ << std::endl;
    // Print users with a default profile.
    set<string_view> default_users;
    dom::parser parser;
    auto [tweets, error] = parser.load(JSON_TEST_PATH)["statuses"].get<dom::array>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    for (auto tweet : tweets) {
      object user;
      tweet["user"].get<dom::object>().tie(user, error); // tie(...) = fails with "no viable overloaded '='" on Apple clang version 11.0.0;
      if (error) { cerr << "Error: " << error << endl; return false; }
      bool default_profile;
      user["default_profile"].get<bool>().tie(default_profile, error); // tie(...) = fails with "no viable overloaded '='" on Apple clang version 11.0.0;
      if (error) { cerr << "Error: " << error << endl; return false; }
      if (default_profile) {
        std::string_view screen_name;
        user["screen_name"].get<std::string_view>().tie(screen_name, error); // tie(...) = fails with "no viable overloaded '='" on Apple clang version 11.0.0;
        if (error) { cerr << "Error: " << error << endl; return false; }
        default_users.insert(screen_name);
      }
    }
    if (default_users.size() != 86) { cerr << "Expected twitter.json[statuses][user] to contain 86 default_profile users, got " << default_users.size() << endl; return false; }
    return true;
  }

  bool twitter_image_sizes() {
    std::cout << "Running " << __func__ << std::endl;
    // Print image names and sizes
    set<pair<uint64_t, uint64_t>> image_sizes;
    dom::parser parser;
    auto [tweets, error] = parser.load(JSON_TEST_PATH)["statuses"].get<dom::array>();
    if (error) { cerr << "Error: " << error << endl; return false; }
    for (auto tweet : tweets) {
      auto [media, not_found] = tweet["entities"]["media"].get<dom::array>();
      if (!not_found) {
        for (auto image : media) {
          object sizes;
          image["sizes"].get<dom::object>().tie(sizes, error); // tie(...) = fails with "no viable overloaded '='" on Apple clang version 11.0.0;
          if (error) { cerr << "Error: " << error << endl; return false; }
          for (auto [key, size] : sizes) {
            uint64_t width, height;
            size["w"].get<uint64_t>().tie(width, error); // tie(...) = fails with "no viable overloaded '='" on Apple clang version 11.0.0;
            if (error) { cerr << "Error: " << error << endl; return false; }
            size["h"].get<uint64_t>().tie(height, error); // tie(...) = fails with "no viable overloaded '='" on Apple clang version 11.0.0;
            if (error) { cerr << "Error: " << error << endl; return false; }
            image_sizes.insert(make_pair(width, height));
          }
        }
      }
    }
    if (image_sizes.size() != 15) { cerr << "Expected twitter.json[statuses][entities][media][sizes] to contain 15 different sizes, got " << image_sizes.size() << endl; return false; }
    return true;
  }

#if SIMDJSON_EXCEPTIONS

  bool object_iterator_exception() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"({ "a": 1, "b": 2, "c": 3 })");
    const char* expected_key[] = { "a", "b", "c" };
    uint64_t expected_value[] = { 1, 2, 3 };
    int i = 0;

    dom::parser parser;
    element doc = parser.parse(json);
    for (auto [key, value] : doc.get<dom::object>()) {
      if (key != expected_key[i] || uint64_t(value) != expected_value[i]) { cerr << "Expected " << expected_key[i] << " = " << expected_value[i] << ", got " << key << "=" << uint64_t(value) << endl; return false; }
      i++;
    }
    if (i*sizeof(uint64_t) != sizeof(expected_value)) { cout << "Expected " << sizeof(expected_value) << " values, got " << i << endl; return false; }
    return true;
  }

  bool array_iterator_exception() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ 1, 10, 100 ])");
    uint64_t expected_value[] = { 1, 10, 100 };
    int i=0;

    dom::parser parser;
    element doc = parser.parse(json);
    for (uint64_t value : doc.get<dom::array>()) {
      if (value != expected_value[i]) { cerr << "Expected " << expected_value[i] << ", got " << value << endl; return false; }
      i++;
    }
    if (i*sizeof(uint64_t) != sizeof(expected_value)) { cout << "Expected " << sizeof(expected_value) << " values, got " << i << endl; return false; }
    return true;
  }

  bool string_value_exception() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ "hi", "has backslash\\" ])");
    dom::parser parser;
    auto val = parser.parse(json).get<dom::array>().begin();

    if (strcmp((const char*)*val, "hi")) { cerr << "Expected const char*(\"hi\") to be \"hi\", was " << (const char*)*val << endl; return false; }
    if (string_view(*val) != "hi") { cerr << "Expected string_view(\"hi\") to be \"hi\", was " << string_view(*val) << endl; return false; }
    ++val;
    if (strcmp((const char*)*val, "has backslash\\")) { cerr << "Expected const char*(\"has backslash\\\\\") to be \"has backslash\\\", was " << (const char*)*val << endl; return false; }
    if (string_view(*val) != "has backslash\\") { cerr << "Expected string_view(\"has backslash\\\\\") to be \"has backslash\\\", was " << string_view(*val) << endl; return false; }
    return true;
  }

  bool numeric_values_exception() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ 0, 1, -1, 1.1 ])");
    dom::parser parser;
    auto val = parser.parse(json).get<dom::array>().begin();

    if (uint64_t(*val) != 0) { cerr << "Expected uint64_t(0) to be 0, was " << uint64_t(*val) << endl; return false; }
    if (int64_t(*val) != 0) { cerr << "Expected int64_t(0) to be 0, was " << int64_t(*val) << endl; return false; }
    if (double(*val) != 0) { cerr << "Expected double(0) to be 0, was " << double(*val) << endl; return false; }
    ++val;
    if (uint64_t(*val) != 1) { cerr << "Expected uint64_t(1) to be 1, was " << uint64_t(*val) << endl; return false; }
    if (int64_t(*val) != 1) { cerr << "Expected int64_t(1) to be 1, was " << int64_t(*val) << endl; return false; }
    if (double(*val) != 1) { cerr << "Expected double(1) to be 1, was " << double(*val) << endl; return false; }
    ++val;
    if (int64_t(*val) != -1) { cerr << "Expected int64_t(-1) to be -1, was " << int64_t(*val) << endl; return false; }
    if (double(*val) != -1) { cerr << "Expected double(-1) to be -1, was " << double(*val) << endl; return false; }
    ++val;
    if (double(*val) != 1.1) { cerr << "Expected double(1.1) to be 1.1, was " << double(*val) << endl; return false; }
    return true;
  }

  bool boolean_values_exception() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ true, false ])");
    dom::parser parser;
    auto val = parser.parse(json).get<dom::array>().begin();

    if (bool(*val) != true) { cerr << "Expected bool(true) to be true, was " << bool(*val) << endl; return false; }
    ++val;
    if (bool(*val) != false) { cerr << "Expected bool(false) to be false, was " << bool(*val) << endl; return false; }
    return true;
  }

  bool null_value_exception() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"([ null ])");
    dom::parser parser;
    auto val = parser.parse(json).get<dom::array>().begin();

    if (!(*val).is_null()) { cerr << "Expected null to be null!" << endl; return false; }
    return true;
  }

  bool document_object_index_exception() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"({ "a": 1, "b": 2, "c": 3})");
    dom::parser parser;
    element doc = parser.parse(json);
    if (uint64_t(doc["a"]) != 1) { cerr << "Expected uint64_t(doc[\"a\"]) to be 1, was " << uint64_t(doc["a"]) << endl; return false; }
    return true;
  }

  bool object_index_exception() {
    std::cout << "Running " << __func__ << std::endl;
    string json(R"({ "obj": { "a": 1, "b": 2, "c": 3 } })");
    dom::parser parser;
    object obj = parser.parse(json)["obj"];
    if (uint64_t(obj["a"]) != 1) { cerr << "Expected uint64_t(doc[\"a\"]) to be 1, was " << uint64_t(obj["a"]) << endl; return false; }
    return true;
  }

  bool twitter_count_exception() {
    std::cout << "Running " << __func__ << std::endl;
    // Prints the number of results in twitter.json
    dom::parser parser;
    element doc = parser.load(JSON_TEST_PATH);
    uint64_t result_count = doc["search_metadata"]["count"];
    if (result_count != 100) { cerr << "Expected twitter.json[metadata_count][count] = 100, got " << result_count << endl; return false; }
    return true;
  }

  bool twitter_default_profile_exception() {
    std::cout << "Running " << __func__ << std::endl;
    // Print users with a default profile.
    set<string_view> default_users;
    dom::parser parser;
    element doc = parser.load(JSON_TEST_PATH);
    for (object tweet : doc["statuses"].get<dom::array>()) {
      object user = tweet["user"];
      if (user["default_profile"]) {
        default_users.insert(user["screen_name"]);
      }
    }
    if (default_users.size() != 86) { cerr << "Expected twitter.json[statuses][user] to contain 86 default_profile users, got " << default_users.size() << endl; return false; }
    return true;
  }

  bool twitter_image_sizes_exception() {
    std::cout << "Running " << __func__ << std::endl;
    // Print image names and sizes
    set<pair<uint64_t, uint64_t>> image_sizes;
    dom::parser parser;
    element doc = parser.load(JSON_TEST_PATH);
    for (object tweet : doc["statuses"].get<dom::array>()) {
      auto [media, not_found] = tweet["entities"]["media"];
      if (!not_found) {
        for (object image : media.get<dom::array>()) {
          for (auto [key, size] : image["sizes"].get<dom::object>()) {
            image_sizes.insert(make_pair(size["w"], size["h"]));
          }
        }
      }
    }
    if (image_sizes.size() != 15) { cerr << "Expected twitter.json[statuses][entities][media][sizes] to contain 15 different sizes, got " << image_sizes.size() << endl; return false; }
    return true;
  }

#endif

  bool run() {
    return ParsedJson_Iterator_test() &&
           object_iterator() &&
           array_iterator() &&
           object_iterator_empty() &&
           array_iterator_empty() &&
           string_value() &&
           numeric_values() &&
           boolean_values() &&
           null_value() &&
           document_object_index() &&
           object_index() &&
           twitter_count() &&
           twitter_default_profile() &&
           twitter_image_sizes() &&
#if SIMDJSON_EXCEPTIONS
           object_iterator_exception() &&
           array_iterator_exception() &&
           string_value_exception() &&
           numeric_values_exception() &&
           boolean_values_exception() &&
           null_value_exception() &&
           document_object_index() &&
           twitter_count_exception() &&
           twitter_default_profile_exception() &&
           twitter_image_sizes_exception() &&
#endif
           true;
  }
}

namespace format_tests {
  using namespace simdjson;
  using namespace simdjson::dom;
  using namespace std;
  const padded_string DOCUMENT = R"({ "foo" : 1, "bar" : [ 1, 2, 3 ], "baz": { "a": 1, "b": 2, "c": 3 } })"_padded;
  const string MINIFIED(R"({"foo":1,"bar":[1,2,3],"baz":{"a":1,"b":2,"c":3}})");
  bool assert_minified(ostringstream &actual, const std::string &expected=MINIFIED) {
    if (actual.str() != expected) {
      cerr << "Failed to correctly minify " << DOCUMENT.data() << endl;
      cerr << "Expected: " << expected << endl;
      cerr << "Actual:   " << actual.str() << endl;
      return false;
    }
    return true;
  }

  bool print_parser_parse() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [doc, error] = parser.parse(DOCUMENT);
    if (error) { cerr << error << endl; return false; }
    ostringstream s;
    s << doc;
    return assert_minified(s);
  }
  bool print_minify_parser_parse() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [doc, error] = parser.parse(DOCUMENT);
    if (error) { cerr << error << endl; return false; }
    ostringstream s;
    s << minify(doc);
    return assert_minified(s);
  }

  bool print_element() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [value, error] = parser.parse(DOCUMENT)["foo"];
    ostringstream s;
    s << value;
    return assert_minified(s, "1");
  }
  bool print_minify_element() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [value, error] = parser.parse(DOCUMENT)["foo"];
    ostringstream s;
    s << minify(value);
    return assert_minified(s, "1");
  }

  bool print_array() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [value, error] = parser.parse(DOCUMENT)["bar"].get<dom::array>();
    ostringstream s;
    s << value;
    return assert_minified(s, "[1,2,3]");
  }
  bool print_minify_array() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [value, error] = parser.parse(DOCUMENT)["bar"].get<dom::array>();
    ostringstream s;
    s << minify(value);
    return assert_minified(s, "[1,2,3]");
  }

  bool print_object() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [value, error] = parser.parse(DOCUMENT)["baz"].get<dom::object>();
    ostringstream s;
    s << value;
    return assert_minified(s, R"({"a":1,"b":2,"c":3})");
  }
  bool print_minify_object() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    auto [value, error] = parser.parse(DOCUMENT)["baz"].get<dom::object>();
    ostringstream s;
    s << minify(value);
    return assert_minified(s, R"({"a":1,"b":2,"c":3})");
  }

#if SIMDJSON_EXCEPTIONS

  bool print_parser_parse_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    ostringstream s;
    s << parser.parse(DOCUMENT);
    return assert_minified(s);
  }
  bool print_minify_parser_parse_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    ostringstream s;
    s << minify(parser.parse(DOCUMENT));
    return assert_minified(s);
  }

  bool print_element_result_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(DOCUMENT);
    ostringstream s;
    s << doc["foo"];
    return assert_minified(s, "1");
  }
  bool print_minify_element_result_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(DOCUMENT);
    ostringstream s;
    s << minify(doc["foo"]);
    return assert_minified(s, "1");
  }

  bool print_element_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(DOCUMENT);
    element value = doc["foo"];
    ostringstream s;
    s << value;
    return assert_minified(s, "1");
  }
  bool print_minify_element_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(DOCUMENT);
    element value = doc["foo"];
    ostringstream s;
    s << minify(value);
    return assert_minified(s, "1");
  }

  bool print_array_result_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(DOCUMENT);
    ostringstream s;
    s << doc["bar"].get<dom::array>();
    return assert_minified(s, "[1,2,3]");
  }
  bool print_minify_array_result_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(DOCUMENT);
    ostringstream s;
    s << minify(doc["bar"].get<dom::array>());
    return assert_minified(s, "[1,2,3]");
  }

  bool print_object_result_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(DOCUMENT);
    ostringstream s;
    s << doc["baz"].get<dom::object>();
    return assert_minified(s, R"({"a":1,"b":2,"c":3})");
  }
  bool print_minify_object_result_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(DOCUMENT);
    ostringstream s;
    s << minify(doc["baz"].get<dom::object>());
    return assert_minified(s, R"({"a":1,"b":2,"c":3})");
  }

  bool print_array_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    ostringstream s;
    s << parser.parse(DOCUMENT)["bar"];
    return assert_minified(s, "[1,2,3]");
  }
  bool print_minify_array_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    ostringstream s;
    s << minify(parser.parse(DOCUMENT)["bar"]);
    return assert_minified(s, "[1,2,3]");
  }

  bool print_object_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    ostringstream s;
    s << parser.parse(DOCUMENT)["baz"];
    return assert_minified(s, R"({"a":1,"b":2,"c":3})");
  }
  bool print_minify_object_exception() {
    std::cout << "Running " << __func__ << std::endl;
    dom::parser parser;
    element doc = parser.parse(DOCUMENT);
    object value = doc["baz"];
    ostringstream s;
    s << minify(value);
    return assert_minified(s, R"({"a":1,"b":2,"c":3})");
  }
#endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return print_parser_parse() && print_minify_parser_parse() &&
           print_element() && print_minify_element() &&
           print_array() && print_minify_array() &&
           print_object() && print_minify_object() &&
#if SIMDJSON_EXCEPTIONS
           print_parser_parse_exception() && print_minify_parser_parse_exception() &&
           print_element_result_exception() && print_minify_element_result_exception() &&
           print_array_result_exception() && print_minify_array_result_exception() &&
           print_object_result_exception() && print_minify_object_result_exception() &&
           print_element_exception() && print_minify_element_exception() &&
           print_array_exception() && print_minify_array_exception() &&
           print_object_exception() && print_minify_object_exception() &&
#endif
           true;
  }
}

bool error_messages_in_correct_order() {
  std::cout << "Running " << __func__ << std::endl;
  using namespace simdjson;
  using namespace simdjson::internal;
  using namespace std;
  if ((sizeof(error_codes)/sizeof(error_code_info)) != NUM_ERROR_CODES) {
    cerr << "error_codes does not have all codes in error_code enum (or too many)" << endl;
    return false;
  }
  for (int i=0; i<NUM_ERROR_CODES; i++) {
    if (error_codes[i].code != i) {
      cerr << "Error " << int(error_codes[i].code) << " at wrong position (" << i << "): " << error_codes[i].message << endl;
      return false;
    }
  }
  return true;
}

int main(int argc, char *argv[]) {
  std::cout << std::unitbuf;
#ifndef _MSC_VER
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
#else
  int optind = 1;
#endif

  // this is put here deliberately to check that the documentation is correct (README),
  // should this fail to compile, you should update the documentation:
  if (simdjson::active_implementation->name() == "unsupported") { 
    printf("unsupported CPU\n"); 
  }
  std::cout << "Running basic tests." << std::endl;
  if (parse_api_tests::run() &&
      dom_api_tests::run() &&
      format_tests::run() &&
      document_tests::run() &&
      number_tests::run() &&
      document_stream_tests::run() &&
      error_messages_in_correct_order()
  ) {
    std::cout << "Basic tests are ok." << std::endl;
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
