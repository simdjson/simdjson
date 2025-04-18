#include <cmath>
#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace number_tests {

  bool small_integers() {
    TEST_START();
    for (int64_t m = 10; m < 20; m++) {
      for (int64_t i = -1024; i < 1024; i++) {
        if(!test_ondemand<int64_t>(std::to_string(i),
                                   [&](int64_t actual) {
             ASSERT_EQUAL(actual, i);
             return true;
           })) {
          return false;
        } // if
      } // for i
    } // for m
    return true;
  }

  bool powers_of_two() {
    TEST_START();

    // converts the double "expected" to a padded string
    auto format_into_padded=[](const double expected) -> padded_string
    {
      std::vector<char> buf(1024);
      const auto n = std::snprintf(buf.data(),
                               buf.size(),
                               "%.*e",
                               std::numeric_limits<double>::max_digits10 - 1,
                               expected);
      const auto nz=static_cast<size_t>(n);
      if (n<0 || nz >= buf.size()) { std::abort(); }
      return padded_string(buf.data(), nz);
    };

    for (int i = -1075; i < 1024; ++i) {// large negative values should be zero.
      const double expected = std::pow(2, i);
      const auto buf=format_into_padded(expected);
      std::fflush(nullptr);
      if(!test_ondemand<double>(buf,
                                [&](double actual) {
                                if(actual!=expected) {
                                  std::cerr << "JSON '" << buf << " parsed to ";
                                  std::fprintf( stderr," %18.18g instead of %18.18g\n", actual, expected); // formatting numbers is easier with printf
                                  SIMDJSON_SHOW_DEFINE(FLT_EVAL_METHOD);
                                  return false;
                                }
                                return true;
                              })) {
        return false;
      } // if
    } // for i
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
    TEST_START();
    std::vector<char> buf(1024);

    const bool is_pow_correct{1e-308 == std::pow(10,-308)};
    const int start_point = is_pow_correct ? -10000 : -307;
    if(!is_pow_correct) {
      std::cout << "On your system, the pow function is busted. Sorry about that. " << std::endl;
    }
    for (int i = start_point; i <= 308; ++i) {// large negative values should be zero.
      const size_t n = std::snprintf(buf.data(), buf.size(), "1e%d", i);
      if (n >= buf.size()) { std::abort(); }
      std::fflush(nullptr);
      const double expected = ((i >= -307) ? testing_power_of_ten[i + 307]: std::pow(10, i));

      if(!test_ondemand<double>(padded_string(buf.data(), n), [&](double actual) {
                                if(actual!=expected) {
                                  std::cerr << "JSON '" << buf.data() << " parsed to ";
                                  std::fprintf( stderr," %18.18g instead of %18.18g\n", actual, expected); // formatting numbers is easier with printf
                                  SIMDJSON_SHOW_DEFINE(FLT_EVAL_METHOD);
                                  return false;
                                }
                                return true;
                              })) {
        return false;
      } // if
    } // for i
    return true;
  }

  void github_issue_1273() {
    padded_string bad(std::string_view("0.0300000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000024000000000000000000000000000000000000000000000000000000000000122978293824"));
    simdjson::ondemand::parser parser;
    simdjson_unused auto blah=parser.iterate(bad);
    double x;
    simdjson_unused auto blah2=blah.get(x);
  }

  bool issue_1898() {
    TEST_START();
    padded_string negative_zero_string(std::string_view("-1e-999"));
    simdjson::ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(negative_zero_string).get(doc));
    double x;
    ASSERT_SUCCESS(doc.get(x));
    // should be minus 0
    ASSERT_TRUE(std::signbit(x));
    ASSERT_TRUE(x == -0);
    TEST_SUCCEED();
  }

  bool old_crashes() {
    TEST_START();
    github_issue_1273();
    TEST_SUCCEED();
  }


  bool is_alive_root_array() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata = R"([1,2,3)"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_TRUE(doc.is_alive());
    size_t count{0};
    for(simdjson_unused simdjson_result<ondemand::value> val : doc) {
      ASSERT_ERROR(val.error(), INCOMPLETE_ARRAY_OR_OBJECT);
      count++;
    }
    ASSERT_EQUAL(count, 1);
    ASSERT_FALSE(doc.is_alive());
    TEST_SUCCEED();
  }

  bool get_number_tests() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata = R"([1.0, 3, 1, 3.1415,-13231232,9999999999999999999,-9223372036854775807,-9223372036854775808,12345678901234567890123])"_padded;
    ondemand::number_type expectedtypes[] = {ondemand::number_type::floating_point_number,
                                 ondemand::number_type::signed_integer,
                                 ondemand::number_type::signed_integer,
                                 ondemand::number_type::floating_point_number,
                                 ondemand::number_type::signed_integer,
                                 ondemand::number_type::unsigned_integer,
                                 ondemand::number_type::signed_integer,
                                 ondemand::number_type::signed_integer,
                                 ondemand::number_type::big_integer
                                 };
    bool is_negative[] = {false, false, false, false, true, false, true, true, false};
    bool is_integer[] = {false, true, true, false, true, true, true, true, true};

    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::array arr;
    ASSERT_SUCCESS(doc.get_array().get(arr));
    size_t counter{0};
    for(simdjson_result<ondemand::value> valr : arr) {
      ondemand::value val;
      ASSERT_SUCCESS(valr.get(val));
      ondemand::number_type nt{};
      auto r = val.get_number_type().get(nt);
      ASSERT_SUCCESS(r);
      ASSERT_EQUAL(expectedtypes[counter], nt);
      bool intvalue{};
      auto res = val.is_integer().get(intvalue);
      ASSERT_SUCCESS(res);
      ondemand::number num;
      auto rr = val.get_number().get(num);
      if(counter == 8) {
        ASSERT_EQUAL(rr, BIGINT_ERROR);
        ASSERT_EQUAL(nt, ondemand::number_type::big_integer);
      } else {
        ASSERT_SUCCESS(rr);
        ondemand::number_type t = num.get_number_type();
        ASSERT_EQUAL(expectedtypes[counter], t);
      }
      ASSERT_EQUAL(is_integer[counter], intvalue);
      ASSERT_EQUAL(is_negative[counter], val.is_negative());
      switch(nt) {
        case ondemand::number_type::signed_integer:
          ASSERT_TRUE(num.is_int64());
          break;
        case ondemand::number_type::unsigned_integer:
          ASSERT_TRUE(num.is_uint64());
          break;
        case ondemand::number_type::floating_point_number:
          ASSERT_TRUE(num.is_double());
          break;
        case ondemand::number_type::big_integer:
          break;
      }
      switch(counter) {
        case 0:
          ASSERT_EQUAL(num.get_double(), 1.0);
          ASSERT_EQUAL((double)num, 1.0);
          break;
        case 1:
          ASSERT_EQUAL(num.get_int64(), 3);
          ASSERT_EQUAL((int64_t)num, 3);
          break;
        case 2:
          ASSERT_EQUAL(num.get_int64(), 1);
          ASSERT_EQUAL((int64_t)num, 1);
          break;
        case 3:
          ASSERT_EQUAL(num.get_double(), 3.1415);
          ASSERT_EQUAL((double)num, 3.1415);
          break;
        case 4:
          ASSERT_EQUAL(num.get_int64(), -13231232);
          ASSERT_EQUAL((int64_t)num, -13231232);
          break;
        case 5:
          ASSERT_EQUAL(num.get_uint64(), UINT64_C(9999999999999999999));
          ASSERT_EQUAL((uint64_t)num, UINT64_C(9999999999999999999));
          break;
        case 8:
          ASSERT_EQUAL(val.raw_json_token(), "12345678901234567890123");
          break;
        default:
          break;
      }
      counter++;
    }
    TEST_SUCCEED();
  }

  bool issue2099() {
    TEST_START();
    ondemand::parser parser;
    auto json = "1000000000.000000001"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::number number;
    ASSERT_SUCCESS(doc.get_number().get(number));
    ASSERT_EQUAL(number.get_number_type(), ondemand::number_type::floating_point_number);
    ASSERT_EQUAL(number.get_double(), 1e9);
    TEST_SUCCEED();
  }

  bool minus_zero() {
    TEST_START();
    ondemand::parser parser;
    auto json = "-0"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::number number;
    ASSERT_SUCCESS(doc.get_number().get(number));
    #if SIMDJSON_MINUS_ZERO_AS_FLOAT
    ASSERT_EQUAL(number.get_number_type(), ondemand::number_type::floating_point_number);
    #else
    ASSERT_EQUAL(number.get_number_type(), ondemand::number_type::signed_integer);
    #endif
    ondemand::number_type nt{};
    ASSERT_SUCCESS(doc.get_number_type().get(nt));
    #if SIMDJSON_MINUS_ZERO_AS_FLOAT
    ASSERT_EQUAL(nt, ondemand::number_type::floating_point_number);
    #else
    ASSERT_EQUAL(nt, ondemand::number_type::signed_integer);
    #endif
    TEST_SUCCEED();
  }

  bool issue1878() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"(123_abc)"_padded;
    for (char ch : {'_', '%', 'z', '&', '\\', '/', '*'}) {
      json.data()[3] = ch;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ASSERT_ERROR(doc.get_int64().error(), NUMBER_ERROR);
    }
    for (char ch : {'[', ']', '{', '}', ',', ' '}) {
      json.data()[3] = ch;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ASSERT_ERROR(doc.get_int64().error(), TRAILING_CONTENT);
    }
    for (char ch : {'_', '%', 'z', '&', '\\', '/', '*'}) {
      json.data()[3] = ch;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ASSERT_ERROR(doc.get_uint64().error(), NUMBER_ERROR);
    }
    for (char ch : {'[', ']', '{', '}', ',', ' '}) {
      json.data()[3] = ch;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ASSERT_ERROR(doc.get_uint64().error(), TRAILING_CONTENT);
    }
    for (char ch : {'_', '%', 'z', '&', '\\', '/', '*'}) {
      json.data()[3] = ch;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ASSERT_ERROR(doc.get_double().error(), NUMBER_ERROR);
    }
    for (char ch : {'[', ']', '{', '}', ',', ' '}) {
      json.data()[3] = ch;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ASSERT_ERROR(doc.get_double().error(), TRAILING_CONTENT);
    }
    for (char ch : {'_', '%', 'z', '&', '\\', '/', '*'}) {
      json.data()[3] = ch;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ASSERT_ERROR(doc.get_number().error(), NUMBER_ERROR);
    }
    for (char ch : {'[', ']', '{', '}', ',', ' '}) {
      json.data()[3] = ch;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ASSERT_ERROR(doc.get_number().error(), TRAILING_CONTENT);
    }
    TEST_SUCCEED();
  }



  bool get_root_number_tests() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    padded_string docdata;
    ondemand::number number;
    ondemand::number_type nt{};

    bool intvalue{};

    docdata = R"(1.0)"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_FALSE(doc.is_negative());
    ASSERT_SUCCESS(doc.get_number_type().get(nt));
    ASSERT_EQUAL(nt, ondemand::number_type::floating_point_number);
    ASSERT_SUCCESS(doc.is_integer().get(intvalue));
    ASSERT_SUCCESS(doc.get_number().get(number));
    ASSERT_FALSE(intvalue);
    ASSERT_EQUAL(number.get_number_type(), ondemand::number_type::floating_point_number);
    ASSERT_EQUAL(number.get_double(), 1.0);

    docdata = R"(-3.132321)"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_TRUE(doc.is_negative());
    ASSERT_SUCCESS(doc.get_number_type().get(nt));
    ASSERT_EQUAL(nt, ondemand::number_type::floating_point_number);
    ASSERT_SUCCESS(doc.is_integer().get(intvalue));
    ASSERT_SUCCESS(doc.get_number().get(number));
    ASSERT_FALSE(intvalue);
    ASSERT_EQUAL(number.get_number_type(), ondemand::number_type::floating_point_number);
    ASSERT_EQUAL(number.get_double(), -3.132321);

    docdata = R"(1233)"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_FALSE(doc.is_negative());
    ASSERT_SUCCESS(doc.get_number_type().get(nt));
    ASSERT_EQUAL(nt, ondemand::number_type::signed_integer);
    ASSERT_SUCCESS(doc.is_integer().get(intvalue));
    ASSERT_SUCCESS(doc.get_number().get(number));
    ASSERT_TRUE(intvalue);
    ASSERT_EQUAL(number.get_number_type(), ondemand::number_type::signed_integer);
    ASSERT_EQUAL(number.get_int64(), 1233);

    docdata = R"(9999999999999999999)"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_FALSE(doc.is_negative());
    ASSERT_SUCCESS(doc.get_number_type().get(nt));
    ASSERT_EQUAL(nt, ondemand::number_type::unsigned_integer);
    ASSERT_SUCCESS(doc.is_integer().get(intvalue));
    ASSERT_SUCCESS(doc.get_number().get(number));
    ASSERT_TRUE(intvalue);
    ASSERT_EQUAL(number.get_number_type(), ondemand::number_type::unsigned_integer);
    ASSERT_EQUAL(number.get_uint64(), UINT64_C(9999999999999999999));

    TEST_SUCCEED();
  }

  bool issue2017() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    padded_string docdata = R"({"score":0.8825149536132812})"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    double score;
    ASSERT_SUCCESS(doc["score"].get_double().get(score));
    ASSERT_EQUAL(score, 0.8825149536132812);
    TEST_SUCCEED();
  }

  bool issue2045() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    padded_string docdata = R"(-18446744073709551615)"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    int64_t bigint;
    ASSERT_ERROR(doc.get_int64().get(bigint), INCORRECT_TYPE);
    TEST_SUCCEED();
  }


  bool issue2093() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    padded_string docdata = R"(0.95000000000000000000)"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    double d;
    ASSERT_SUCCESS(doc.get_double().get(d));
    ASSERT_EQUAL(d, 0.95);
    TEST_SUCCEED();
  }

  bool big_int_not_zero() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    // This is not a big integer, it is a mistake
    padded_string docdata = R"(09500000000000000000000000000000000000)"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_ERROR(doc.get_number(), NUMBER_ERROR);
    TEST_SUCCEED();
  }

  bool negative_big_int() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    // This is not a big integer, it is a mistake
    padded_string docdata = R"(-18446744073709551616)"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_ERROR(doc.get_number(), BIGINT_ERROR);
    std::string_view my_big;
    ASSERT_SUCCESS(doc.raw_json_token().get(my_big));
    ASSERT_EQUAL(my_big, "-18446744073709551616");
    TEST_SUCCEED();
  }

  bool gigantic_big_int() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    std::string number(2000, '1');
    // This is not a big integer, it is a mistake
    ASSERT_SUCCESS(parser.iterate(number).get(doc));
    ASSERT_ERROR(doc.get_number(), BIGINT_ERROR);
    std::string_view my_big;
    ASSERT_SUCCESS(doc.raw_json_token().get(my_big));
    ASSERT_EQUAL(my_big, number);
    TEST_SUCCEED();
  }

  bool run() {
    return minus_zero() &&
           gigantic_big_int() &&
           big_int_not_zero() &&
           negative_big_int() &&
           issue2099() &&
           issue2093() &&
           issue2045() &&
           issue2017() &&
           issue_1898() &&
           issue1878() &&
           get_root_number_tests() &&
           get_number_tests()&&
           small_integers() &&
           powers_of_two() &&
           powers_of_ten() &&
           old_crashes();
  }

} // namespace number_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, number_tests::run);
}
