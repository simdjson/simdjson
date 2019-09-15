#ifndef SIMDJSON_NUMBERPARSING_H
#define SIMDJSON_NUMBERPARSING_H

#include "simdjson/common_defs.h"
#include "simdjson/jsoncharutils.h"
#include "simdjson/parsedjson.h"
#include "simdjson/portability.h"
#include <cmath>

#ifdef JSON_TEST_NUMBERS // for unit testing
void found_invalid_number(const uint8_t *buf);
void found_integer(int64_t result, const uint8_t *buf);
void found_unsigned_integer(uint64_t result, const uint8_t *buf);
void found_float(double result, const uint8_t *buf);
#endif

namespace simdjson {
// Allowable floating-point values range from
// std::numeric_limits<double>::lowest() to std::numeric_limits<double>::max(),
// so from -1.7976e308 all the way to 1.7975e308 in binary64. The lowest
// non-zero normal values is std::numeric_limits<double>::min() or
// about 2.225074e-308.
static const double power_of_ten[] = {
    1e-308, 1e-307, 1e-306, 1e-305, 1e-304, 1e-303, 1e-302, 1e-301, 1e-300,
    1e-299, 1e-298, 1e-297, 1e-296, 1e-295, 1e-294, 1e-293, 1e-292, 1e-291,
    1e-290, 1e-289, 1e-288, 1e-287, 1e-286, 1e-285, 1e-284, 1e-283, 1e-282,
    1e-281, 1e-280, 1e-279, 1e-278, 1e-277, 1e-276, 1e-275, 1e-274, 1e-273,
    1e-272, 1e-271, 1e-270, 1e-269, 1e-268, 1e-267, 1e-266, 1e-265, 1e-264,
    1e-263, 1e-262, 1e-261, 1e-260, 1e-259, 1e-258, 1e-257, 1e-256, 1e-255,
    1e-254, 1e-253, 1e-252, 1e-251, 1e-250, 1e-249, 1e-248, 1e-247, 1e-246,
    1e-245, 1e-244, 1e-243, 1e-242, 1e-241, 1e-240, 1e-239, 1e-238, 1e-237,
    1e-236, 1e-235, 1e-234, 1e-233, 1e-232, 1e-231, 1e-230, 1e-229, 1e-228,
    1e-227, 1e-226, 1e-225, 1e-224, 1e-223, 1e-222, 1e-221, 1e-220, 1e-219,
    1e-218, 1e-217, 1e-216, 1e-215, 1e-214, 1e-213, 1e-212, 1e-211, 1e-210,
    1e-209, 1e-208, 1e-207, 1e-206, 1e-205, 1e-204, 1e-203, 1e-202, 1e-201,
    1e-200, 1e-199, 1e-198, 1e-197, 1e-196, 1e-195, 1e-194, 1e-193, 1e-192,
    1e-191, 1e-190, 1e-189, 1e-188, 1e-187, 1e-186, 1e-185, 1e-184, 1e-183,
    1e-182, 1e-181, 1e-180, 1e-179, 1e-178, 1e-177, 1e-176, 1e-175, 1e-174,
    1e-173, 1e-172, 1e-171, 1e-170, 1e-169, 1e-168, 1e-167, 1e-166, 1e-165,
    1e-164, 1e-163, 1e-162, 1e-161, 1e-160, 1e-159, 1e-158, 1e-157, 1e-156,
    1e-155, 1e-154, 1e-153, 1e-152, 1e-151, 1e-150, 1e-149, 1e-148, 1e-147,
    1e-146, 1e-145, 1e-144, 1e-143, 1e-142, 1e-141, 1e-140, 1e-139, 1e-138,
    1e-137, 1e-136, 1e-135, 1e-134, 1e-133, 1e-132, 1e-131, 1e-130, 1e-129,
    1e-128, 1e-127, 1e-126, 1e-125, 1e-124, 1e-123, 1e-122, 1e-121, 1e-120,
    1e-119, 1e-118, 1e-117, 1e-116, 1e-115, 1e-114, 1e-113, 1e-112, 1e-111,
    1e-110, 1e-109, 1e-108, 1e-107, 1e-106, 1e-105, 1e-104, 1e-103, 1e-102,
    1e-101, 1e-100, 1e-99,  1e-98,  1e-97,  1e-96,  1e-95,  1e-94,  1e-93,
    1e-92,  1e-91,  1e-90,  1e-89,  1e-88,  1e-87,  1e-86,  1e-85,  1e-84,
    1e-83,  1e-82,  1e-81,  1e-80,  1e-79,  1e-78,  1e-77,  1e-76,  1e-75,
    1e-74,  1e-73,  1e-72,  1e-71,  1e-70,  1e-69,  1e-68,  1e-67,  1e-66,
    1e-65,  1e-64,  1e-63,  1e-62,  1e-61,  1e-60,  1e-59,  1e-58,  1e-57,
    1e-56,  1e-55,  1e-54,  1e-53,  1e-52,  1e-51,  1e-50,  1e-49,  1e-48,
    1e-47,  1e-46,  1e-45,  1e-44,  1e-43,  1e-42,  1e-41,  1e-40,  1e-39,
    1e-38,  1e-37,  1e-36,  1e-35,  1e-34,  1e-33,  1e-32,  1e-31,  1e-30,
    1e-29,  1e-28,  1e-27,  1e-26,  1e-25,  1e-24,  1e-23,  1e-22,  1e-21,
    1e-20,  1e-19,  1e-18,  1e-17,  1e-16,  1e-15,  1e-14,  1e-13,  1e-12,
    1e-11,  1e-10,  1e-9,   1e-8,   1e-7,   1e-6,   1e-5,   1e-4,   1e-3,
    1e-2,   1e-1,   1e0,    1e1,    1e2,    1e3,    1e4,    1e5,    1e6,
    1e7,    1e8,    1e9,    1e10,   1e11,   1e12,   1e13,   1e14,   1e15,
    1e16,   1e17,   1e18,   1e19,   1e20,   1e21,   1e22,   1e23,   1e24,
    1e25,   1e26,   1e27,   1e28,   1e29,   1e30,   1e31,   1e32,   1e33,
    1e34,   1e35,   1e36,   1e37,   1e38,   1e39,   1e40,   1e41,   1e42,
    1e43,   1e44,   1e45,   1e46,   1e47,   1e48,   1e49,   1e50,   1e51,
    1e52,   1e53,   1e54,   1e55,   1e56,   1e57,   1e58,   1e59,   1e60,
    1e61,   1e62,   1e63,   1e64,   1e65,   1e66,   1e67,   1e68,   1e69,
    1e70,   1e71,   1e72,   1e73,   1e74,   1e75,   1e76,   1e77,   1e78,
    1e79,   1e80,   1e81,   1e82,   1e83,   1e84,   1e85,   1e86,   1e87,
    1e88,   1e89,   1e90,   1e91,   1e92,   1e93,   1e94,   1e95,   1e96,
    1e97,   1e98,   1e99,   1e100,  1e101,  1e102,  1e103,  1e104,  1e105,
    1e106,  1e107,  1e108,  1e109,  1e110,  1e111,  1e112,  1e113,  1e114,
    1e115,  1e116,  1e117,  1e118,  1e119,  1e120,  1e121,  1e122,  1e123,
    1e124,  1e125,  1e126,  1e127,  1e128,  1e129,  1e130,  1e131,  1e132,
    1e133,  1e134,  1e135,  1e136,  1e137,  1e138,  1e139,  1e140,  1e141,
    1e142,  1e143,  1e144,  1e145,  1e146,  1e147,  1e148,  1e149,  1e150,
    1e151,  1e152,  1e153,  1e154,  1e155,  1e156,  1e157,  1e158,  1e159,
    1e160,  1e161,  1e162,  1e163,  1e164,  1e165,  1e166,  1e167,  1e168,
    1e169,  1e170,  1e171,  1e172,  1e173,  1e174,  1e175,  1e176,  1e177,
    1e178,  1e179,  1e180,  1e181,  1e182,  1e183,  1e184,  1e185,  1e186,
    1e187,  1e188,  1e189,  1e190,  1e191,  1e192,  1e193,  1e194,  1e195,
    1e196,  1e197,  1e198,  1e199,  1e200,  1e201,  1e202,  1e203,  1e204,
    1e205,  1e206,  1e207,  1e208,  1e209,  1e210,  1e211,  1e212,  1e213,
    1e214,  1e215,  1e216,  1e217,  1e218,  1e219,  1e220,  1e221,  1e222,
    1e223,  1e224,  1e225,  1e226,  1e227,  1e228,  1e229,  1e230,  1e231,
    1e232,  1e233,  1e234,  1e235,  1e236,  1e237,  1e238,  1e239,  1e240,
    1e241,  1e242,  1e243,  1e244,  1e245,  1e246,  1e247,  1e248,  1e249,
    1e250,  1e251,  1e252,  1e253,  1e254,  1e255,  1e256,  1e257,  1e258,
    1e259,  1e260,  1e261,  1e262,  1e263,  1e264,  1e265,  1e266,  1e267,
    1e268,  1e269,  1e270,  1e271,  1e272,  1e273,  1e274,  1e275,  1e276,
    1e277,  1e278,  1e279,  1e280,  1e281,  1e282,  1e283,  1e284,  1e285,
    1e286,  1e287,  1e288,  1e289,  1e290,  1e291,  1e292,  1e293,  1e294,
    1e295,  1e296,  1e297,  1e298,  1e299,  1e300,  1e301,  1e302,  1e303,
    1e304,  1e305,  1e306,  1e307,  1e308};

static inline bool is_integer(char c) {
  return (c >= '0' && c <= '9');
  // this gets compiled to (uint8_t)(c - '0') <= 9 on all decent compilers
}

// We need to check that the character following a zero is valid. This is
// probably frequent and it is hard than it looks. We are building all of this
// just to differentiate between 0x1 (invalid), 0,1 (valid) 0e1 (valid)...
const bool structural_or_whitespace_or_exponent_or_decimal_negated[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

really_inline bool
is_not_structural_or_whitespace_or_exponent_or_decimal(unsigned char c) {
  return structural_or_whitespace_or_exponent_or_decimal_negated[c];
}
} // namespace simdjson
#ifndef SIMDJSON_DISABLE_SWAR_NUMBER_PARSING
#define SWAR_NUMBER_PARSING
#endif

#ifdef SWAR_NUMBER_PARSING

namespace simdjson {
// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
static inline bool is_made_of_eight_digits_fast(const char *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING);
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}
} // namespace simdjson
#ifdef IS_X86_64
TARGET_WESTMERE
namespace simdjson {
static inline uint32_t parse_eight_digits_unrolled(const char *chars) {
  // this actually computes *16* values so we are being wasteful.
  const __m128i ascii0 = _mm_set1_epi8('0');
  const __m128i mul_1_10 =
      _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
  const __m128i mul_1_100 = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
  const __m128i mul_1_10000 =
      _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1);
  const __m128i input = _mm_sub_epi8(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(chars)), ascii0);
  const __m128i t1 = _mm_maddubs_epi16(input, mul_1_10);
  const __m128i t2 = _mm_madd_epi16(t1, mul_1_100);
  const __m128i t3 = _mm_packus_epi32(t2, t2);
  const __m128i t4 = _mm_madd_epi16(t3, mul_1_10000);
  return _mm_cvtsi128_si32(
      t4); // only captures the sum of the first 8 digits, drop the rest
}
} // namespace simdjson
UNTARGET_REGION
#endif

namespace simdjson {
#ifdef IS_ARM64
// we don't have SSE, so let us use a scalar function
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
static inline uint32_t parse_eight_digits_unrolled(const char *chars) {
  uint64_t val;
  memcpy(&val, chars, sizeof(uint64_t));
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return (val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32;
}
#endif

#endif

  // the mantissas of powers of five from -308 to 308, extended out to sixty four bits
  typedef struct {
    uint64_t mantissa;
    int32_t exp;
  } components;

  static const components power_of_ten_components[] = {{0xe61acf033d1a45df, -1087}, {0x8fd0c16206306bab, -1083}, {0xb3c4f1ba87bc8696, -1080}, {0xe0b62e2929aba83c, -1077}, {0x8c71dcd9ba0b4925, -1073}, {0xaf8e5410288e1b6f, -1070}, {0xdb71e91432b1a24a, -1067}, {0x892731ac9faf056e, -1063}, {0xab70fe17c79ac6ca, -1060}, {0xd64d3d9db981787d, -1057}, {0x85f0468293f0eb4e, -1053}, {0xa76c582338ed2621, -1050}, {0xd1476e2c07286faa, -1047}, {0x82cca4db847945ca, -1043}, {0xa37fce126597973c, -1040}, {0xcc5fc196fefd7d0c, -1037}, {0xff77b1fcbebcdc4f, -1034}, {0x9faacf3df73609b1, -1030}, {0xc795830d75038c1d, -1027}, {0xf97ae3d0d2446f25, -1024}, {0x9becce62836ac577, -1020}, {0xc2e801fb244576d5, -1017}, {0xf3a20279ed56d48a, -1014}, {0x9845418c345644d6, -1010}, {0xbe5691ef416bd60c, -1007}, {0xedec366b11c6cb8f, -1004}, {0x94b3a202eb1c3f39, -1000}, {0xb9e08a83a5e34f07, -997}, {0xe858ad248f5c22c9, -994}, {0x91376c36d99995be, -990}, {0xb58547448ffffb2d, -987}, {0xe2e69915b3fff9f9, -984}, {0x8dd01fad907ffc3b, -980}, {0xb1442798f49ffb4a, -977}, {0xdd95317f31c7fa1d, -974}, {0x8a7d3eef7f1cfc52, -970}, {0xad1c8eab5ee43b66, -967}, {0xd863b256369d4a40, -964}, {0x873e4f75e2224e68, -960}, {0xa90de3535aaae202, -957}, {0xd3515c2831559a83, -954}, {0x8412d9991ed58091, -950}, {0xa5178fff668ae0b6, -947}, {0xce5d73ff402d98e3, -944}, {0x80fa687f881c7f8e, -940}, {0xa139029f6a239f72, -937}, {0xc987434744ac874e, -934}, {0xfbe9141915d7a922, -931}, {0x9d71ac8fada6c9b5, -927}, {0xc4ce17b399107c22, -924}, {0xf6019da07f549b2b, -921}, {0x99c102844f94e0fb, -917}, {0xc0314325637a1939, -914}, {0xf03d93eebc589f88, -911}, {0x96267c7535b763b5, -907}, {0xbbb01b9283253ca2, -904}, {0xea9c227723ee8bcb, -901}, {0x92a1958a7675175f, -897}, {0xb749faed14125d36, -894}, {0xe51c79a85916f484, -891}, {0x8f31cc0937ae58d2, -887}, {0xb2fe3f0b8599ef07, -884}, {0xdfbdcece67006ac9, -881}, {0x8bd6a141006042bd, -877}, {0xaecc49914078536d, -874}, {0xda7f5bf590966848, -871}, {0x888f99797a5e012d, -867}, {0xaab37fd7d8f58178, -864}, {0xd5605fcdcf32e1d6, -861}, {0x855c3be0a17fcd26, -857}, {0xa6b34ad8c9dfc06f, -854}, {0xd0601d8efc57b08b, -851}, {0x823c12795db6ce57, -847}, {0xa2cb1717b52481ed, -844}, {0xcb7ddcdda26da268, -841}, {0xfe5d54150b090b02, -838}, {0x9efa548d26e5a6e1, -834}, {0xc6b8e9b0709f109a, -831}, {0xf867241c8cc6d4c0, -828}, {0x9b407691d7fc44f8, -824}, {0xc21094364dfb5636, -821}, {0xf294b943e17a2bc4, -818}, {0x979cf3ca6cec5b5a, -814}, {0xbd8430bd08277231, -811}, {0xece53cec4a314ebd, -808}, {0x940f4613ae5ed136, -804}, {0xb913179899f68584, -801}, {0xe757dd7ec07426e5, -798}, {0x9096ea6f3848984f, -794}, {0xb4bca50b065abe63, -791}, {0xe1ebce4dc7f16dfb, -788}, {0x8d3360f09cf6e4bd, -784}, {0xb080392cc4349dec, -781}, {0xdca04777f541c567, -778}, {0x89e42caaf9491b60, -774}, {0xac5d37d5b79b6239, -771}, {0xd77485cb25823ac7, -768}, {0x86a8d39ef77164bc, -764}, {0xa8530886b54dbdeb, -761}, {0xd267caa862a12d66, -758}, {0x8380dea93da4bc60, -754}, {0xa46116538d0deb78, -751}, {0xcd795be870516656, -748}, {0x806bd9714632dff6, -744}, {0xa086cfcd97bf97f3, -741}, {0xc8a883c0fdaf7df0, -738}, {0xfad2a4b13d1b5d6c, -735}, {0x9cc3a6eec6311a63, -731}, {0xc3f490aa77bd60fc, -728}, {0xf4f1b4d515acb93b, -725}, {0x991711052d8bf3c5, -721}, {0xbf5cd54678eef0b6, -718}, {0xef340a98172aace4, -715}, {0x9580869f0e7aac0e, -711}, {0xbae0a846d2195712, -708}, {0xe998d258869facd7, -705}, {0x91ff83775423cc06, -701}, {0xb67f6455292cbf08, -698}, {0xe41f3d6a7377eeca, -695}, {0x8e938662882af53e, -691}, {0xb23867fb2a35b28d, -688}, {0xdec681f9f4c31f31, -685}, {0x8b3c113c38f9f37e, -681}, {0xae0b158b4738705e, -678}, {0xd98ddaee19068c76, -675}, {0x87f8a8d4cfa417c9, -671}, {0xa9f6d30a038d1dbc, -668}, {0xd47487cc8470652b, -665}, {0x84c8d4dfd2c63f3b, -661}, {0xa5fb0a17c777cf09, -658}, {0xcf79cc9db955c2cc, -655}, {0x81ac1fe293d599bf, -651}, {0xa21727db38cb002f, -648}, {0xca9cf1d206fdc03b, -645}, {0xfd442e4688bd304a, -642}, {0x9e4a9cec15763e2e, -638}, {0xc5dd44271ad3cdba, -635}, {0xf7549530e188c128, -632}, {0x9a94dd3e8cf578b9, -628}, {0xc13a148e3032d6e7, -625}, {0xf18899b1bc3f8ca1, -622}, {0x96f5600f15a7b7e5, -618}, {0xbcb2b812db11a5de, -615}, {0xebdf661791d60f56, -612}, {0x936b9fcebb25c995, -608}, {0xb84687c269ef3bfb, -605}, {0xe65829b3046b0afa, -602}, {0x8ff71a0fe2c2e6dc, -598}, {0xb3f4e093db73a093, -595}, {0xe0f218b8d25088b8, -592}, {0x8c974f7383725573, -588}, {0xafbd2350644eeacf, -585}, {0xdbac6c247d62a583, -582}, {0x894bc396ce5da772, -578}, {0xab9eb47c81f5114f, -575}, {0xd686619ba27255a2, -572}, {0x8613fd0145877585, -568}, {0xa798fc4196e952e7, -565}, {0xd17f3b51fca3a7a0, -562}, {0x82ef85133de648c4, -558}, {0xa3ab66580d5fdaf5, -555}, {0xcc963fee10b7d1b3, -552}, {0xffbbcfe994e5c61f, -549}, {0x9fd561f1fd0f9bd3, -545}, {0xc7caba6e7c5382c8, -542}, {0xf9bd690a1b68637b, -539}, {0x9c1661a651213e2d, -535}, {0xc31bfa0fe5698db8, -532}, {0xf3e2f893dec3f126, -529}, {0x986ddb5c6b3a76b7, -525}, {0xbe89523386091465, -522}, {0xee2ba6c0678b597f, -519}, {0x94db483840b717ef, -515}, {0xba121a4650e4ddeb, -512}, {0xe896a0d7e51e1566, -509}, {0x915e2486ef32cd60, -505}, {0xb5b5ada8aaff80b8, -502}, {0xe3231912d5bf60e6, -499}, {0x8df5efabc5979c8f, -495}, {0xb1736b96b6fd83b3, -492}, {0xddd0467c64bce4a0, -489}, {0x8aa22c0dbef60ee4, -485}, {0xad4ab7112eb3929d, -482}, {0xd89d64d57a607744, -479}, {0x87625f056c7c4a8b, -475}, {0xa93af6c6c79b5d2d, -472}, {0xd389b47879823479, -469}, {0x843610cb4bf160cb, -465}, {0xa54394fe1eedb8fe, -462}, {0xce947a3da6a9273e, -459}, {0x811ccc668829b887, -455}, {0xa163ff802a3426a8, -452}, {0xc9bcff6034c13052, -449}, {0xfc2c3f3841f17c67, -446}, {0x9d9ba7832936edc0, -442}, {0xc5029163f384a931, -439}, {0xf64335bcf065d37d, -436}, {0x99ea0196163fa42e, -432}, {0xc06481fb9bcf8d39, -429}, {0xf07da27a82c37088, -426}, {0x964e858c91ba2655, -422}, {0xbbe226efb628afea, -419}, {0xeadab0aba3b2dbe5, -416}, {0x92c8ae6b464fc96f, -412}, {0xb77ada0617e3bbcb, -409}, {0xe55990879ddcaabd, -406}, {0x8f57fa54c2a9eab6, -402}, {0xb32df8e9f3546564, -399}, {0xdff9772470297ebd, -396}, {0x8bfbea76c619ef36, -392}, {0xaefae51477a06b03, -389}, {0xdab99e59958885c4, -386}, {0x88b402f7fd75539b, -382}, {0xaae103b5fcd2a881, -379}, {0xd59944a37c0752a2, -376}, {0x857fcae62d8493a5, -372}, {0xa6dfbd9fb8e5b88e, -369}, {0xd097ad07a71f26b2, -366}, {0x825ecc24c873782f, -362}, {0xa2f67f2dfa90563b, -359}, {0xcbb41ef979346bca, -356}, {0xfea126b7d78186bc, -353}, {0x9f24b832e6b0f436, -349}, {0xc6ede63fa05d3143, -346}, {0xf8a95fcf88747d94, -343}, {0x9b69dbe1b548ce7c, -339}, {0xc24452da229b021b, -336}, {0xf2d56790ab41c2a2, -333}, {0x97c560ba6b0919a5, -329}, {0xbdb6b8e905cb600f, -326}, {0xed246723473e3813, -323}, {0x9436c0760c86e30b, -319}, {0xb94470938fa89bce, -316}, {0xe7958cb87392c2c2, -313}, {0x90bd77f3483bb9b9, -309}, {0xb4ecd5f01a4aa828, -306}, {0xe2280b6c20dd5232, -303}, {0x8d590723948a535f, -299}, {0xb0af48ec79ace837, -296}, {0xdcdb1b2798182244, -293}, {0x8a08f0f8bf0f156b, -289}, {0xac8b2d36eed2dac5, -286}, {0xd7adf884aa879177, -283}, {0x86ccbb52ea94baea, -279}, {0xa87fea27a539e9a5, -276}, {0xd29fe4b18e88640e, -273}, {0x83a3eeeef9153e89, -269}, {0xa48ceaaab75a8e2b, -266}, {0xcdb02555653131b6, -263}, {0x808e17555f3ebf11, -259}, {0xa0b19d2ab70e6ed6, -256}, {0xc8de047564d20a8b, -253}, {0xfb158592be068d2e, -250}, {0x9ced737bb6c4183d, -246}, {0xc428d05aa4751e4c, -243}, {0xf53304714d9265df, -240}, {0x993fe2c6d07b7fab, -236}, {0xbf8fdb78849a5f96, -233}, {0xef73d256a5c0f77c, -230}, {0x95a8637627989aad, -226}, {0xbb127c53b17ec159, -223}, {0xe9d71b689dde71af, -220}, {0x9226712162ab070d, -216}, {0xb6b00d69bb55c8d1, -213}, {0xe45c10c42a2b3b05, -210}, {0x8eb98a7a9a5b04e3, -206}, {0xb267ed1940f1c61c, -203}, {0xdf01e85f912e37a3, -200}, {0x8b61313bbabce2c6, -196}, {0xae397d8aa96c1b77, -193}, {0xd9c7dced53c72255, -190}, {0x881cea14545c7575, -186}, {0xaa242499697392d2, -183}, {0xd4ad2dbfc3d07787, -180}, {0x84ec3c97da624ab4, -176}, {0xa6274bbdd0fadd61, -173}, {0xcfb11ead453994ba, -170}, {0x81ceb32c4b43fcf4, -166}, {0xa2425ff75e14fc31, -163}, {0xcad2f7f5359a3b3e, -160}, {0xfd87b5f28300ca0d, -157}, {0x9e74d1b791e07e48, -153}, {0xc612062576589dda, -150}, {0xf79687aed3eec551, -147}, {0x9abe14cd44753b52, -143}, {0xc16d9a0095928a27, -140}, {0xf1c90080baf72cb1, -137}, {0x971da05074da7bee, -133}, {0xbce5086492111aea, -130}, {0xec1e4a7db69561a5, -127}, {0x9392ee8e921d5d07, -123}, {0xb877aa3236a4b449, -120}, {0xe69594bec44de15b, -117}, {0x901d7cf73ab0acd9, -113}, {0xb424dc35095cd80f, -110}, {0xe12e13424bb40e13, -107}, {0x8cbccc096f5088cb, -103}, {0xafebff0bcb24aafe, -100}, {0xdbe6fecebdedd5be, -97}, {0x89705f4136b4a597, -93}, {0xabcc77118461cefc, -90}, {0xd6bf94d5e57a42bc, -87}, {0x8637bd05af6c69b5, -83}, {0xa7c5ac471b478423, -80}, {0xd1b71758e219652b, -77}, {0x83126e978d4fdf3b, -73}, {0xa3d70a3d70a3d70a, -70}, {0xcccccccccccccccc, -67}, {0x8000000000000000, -63}, {0xa000000000000000, -60}, {0xc800000000000000, -57}, {0xfa00000000000000, -54}, {0x9c40000000000000, -50}, {0xc350000000000000, -47}, {0xf424000000000000, -44}, {0x9896800000000000, -40}, {0xbebc200000000000, -37}, {0xee6b280000000000, -34}, {0x9502f90000000000, -30}, {0xba43b74000000000, -27}, {0xe8d4a51000000000, -24}, {0x9184e72a00000000, -20}, {0xb5e620f480000000, -17}, {0xe35fa931a0000000, -14}, {0x8e1bc9bf04000000, -10}, {0xb1a2bc2ec5000000, -7}, {0xde0b6b3a76400000, -4}, {0x8ac7230489e80000, 0}, {0xad78ebc5ac620000, 3}, {0xd8d726b7177a8000, 6}, {0x878678326eac9000, 10}, {0xa968163f0a57b400, 13}, {0xd3c21bcecceda100, 16}, {0x84595161401484a0, 20}, {0xa56fa5b99019a5c8, 23}, {0xcecb8f27f4200f3a, 26}, {0x813f3978f8940984, 30}, {0xa18f07d736b90be5, 33}, {0xc9f2c9cd04674ede, 36}, {0xfc6f7c4045812296, 39}, {0x9dc5ada82b70b59d, 43}, {0xc5371912364ce305, 46}, {0xf684df56c3e01bc6, 49}, {0x9a130b963a6c115c, 53}, {0xc097ce7bc90715b3, 56}, {0xf0bdc21abb48db20, 59}, {0x96769950b50d88f4, 63}, {0xbc143fa4e250eb31, 66}, {0xeb194f8e1ae525fd, 69}, {0x92efd1b8d0cf37be, 73}, {0xb7abc627050305ad, 76}, {0xe596b7b0c643c719, 79}, {0x8f7e32ce7bea5c6f, 83}, {0xb35dbf821ae4f38b, 86}, {0xe0352f62a19e306e, 89}, {0x8c213d9da502de45, 93}, {0xaf298d050e4395d6, 96}, {0xdaf3f04651d47b4c, 99}, {0x88d8762bf324cd0f, 103}, {0xab0e93b6efee0053, 106}, {0xd5d238a4abe98068, 109}, {0x85a36366eb71f041, 113}, {0xa70c3c40a64e6c51, 116}, {0xd0cf4b50cfe20765, 119}, {0x82818f1281ed449f, 123}, {0xa321f2d7226895c7, 126}, {0xcbea6f8ceb02bb39, 129}, {0xfee50b7025c36a08, 132}, {0x9f4f2726179a2245, 136}, {0xc722f0ef9d80aad6, 139}, {0xf8ebad2b84e0d58b, 142}, {0x9b934c3b330c8577, 146}, {0xc2781f49ffcfa6d5, 149}, {0xf316271c7fc3908a, 152}, {0x97edd871cfda3a56, 156}, {0xbde94e8e43d0c8ec, 159}, {0xed63a231d4c4fb27, 162}, {0x945e455f24fb1cf8, 166}, {0xb975d6b6ee39e436, 169}, {0xe7d34c64a9c85d44, 172}, {0x90e40fbeea1d3a4a, 176}, {0xb51d13aea4a488dd, 179}, {0xe264589a4dcdab14, 182}, {0x8d7eb76070a08aec, 186}, {0xb0de65388cc8ada8, 189}, {0xdd15fe86affad912, 192}, {0x8a2dbf142dfcc7ab, 196}, {0xacb92ed9397bf996, 199}, {0xd7e77a8f87daf7fb, 202}, {0x86f0ac99b4e8dafd, 206}, {0xa8acd7c0222311bc, 209}, {0xd2d80db02aabd62b, 212}, {0x83c7088e1aab65db, 216}, {0xa4b8cab1a1563f52, 219}, {0xcde6fd5e09abcf26, 222}, {0x80b05e5ac60b6178, 226}, {0xa0dc75f1778e39d6, 229}, {0xc913936dd571c84c, 232}, {0xfb5878494ace3a5f, 235}, {0x9d174b2dcec0e47b, 239}, {0xc45d1df942711d9a, 242}, {0xf5746577930d6500, 245}, {0x9968bf6abbe85f20, 249}, {0xbfc2ef456ae276e8, 252}, {0xefb3ab16c59b14a2, 255}, {0x95d04aee3b80ece5, 259}, {0xbb445da9ca61281f, 262}, {0xea1575143cf97226, 265}, {0x924d692ca61be758, 269}, {0xb6e0c377cfa2e12e, 272}, {0xe498f455c38b997a, 275}, {0x8edf98b59a373fec, 279}, {0xb2977ee300c50fe7, 282}, {0xdf3d5e9bc0f653e1, 285}, {0x8b865b215899f46c, 289}, {0xae67f1e9aec07187, 292}, {0xda01ee641a708de9, 295}, {0x884134fe908658b2, 299}, {0xaa51823e34a7eede, 302}, {0xd4e5e2cdc1d1ea96, 305}, {0x850fadc09923329e, 309}, {0xa6539930bf6bff45, 312}, {0xcfe87f7cef46ff16, 315}, {0x81f14fae158c5f6e, 319}, {0xa26da3999aef7749, 322}, {0xcb090c8001ab551c, 325}, {0xfdcb4fa002162a63, 328}, {0x9e9f11c4014dda7e, 332}, {0xc646d63501a1511d, 335}, {0xf7d88bc24209a565, 338}, {0x9ae757596946075f, 342}, {0xc1a12d2fc3978937, 345}, {0xf209787bb47d6b84, 348}, {0x9745eb4d50ce6332, 352}, {0xbd176620a501fbff, 355}, {0xec5d3fa8ce427aff, 358}, {0x93ba47c980e98cdf, 362}, {0xb8a8d9bbe123f017, 365}, {0xe6d3102ad96cec1d, 368}, {0x9043ea1ac7e41392, 372}, {0xb454e4a179dd1877, 375}, {0xe16a1dc9d8545e94, 378}, {0x8ce2529e2734bb1d, 382}, {0xb01ae745b101e9e4, 385}, {0xdc21a1171d42645d, 388}, {0x899504ae72497eba, 392}, {0xabfa45da0edbde69, 395}, {0xd6f8d7509292d603, 398}, {0x865b86925b9bc5c2, 402}, {0xa7f26836f282b732, 405}, {0xd1ef0244af2364ff, 408}, {0x8335616aed761f1f, 412}, {0xa402b9c5a8d3a6e7, 415}, {0xcd036837130890a1, 418}, {0x802221226be55a64, 422}, {0xa02aa96b06deb0fd, 425}, {0xc83553c5c8965d3d, 428}, {0xfa42a8b73abbf48c, 431}, {0x9c69a97284b578d7, 435}, {0xc38413cf25e2d70d, 438}, {0xf46518c2ef5b8cd1, 441}, {0x98bf2f79d5993802, 445}, {0xbeeefb584aff8603, 448}, {0xeeaaba2e5dbf6784, 451}, {0x952ab45cfa97a0b2, 455}, {0xba756174393d88df, 458}, {0xe912b9d1478ceb17, 461}, {0x91abb422ccb812ee, 465}, {0xb616a12b7fe617aa, 468}, {0xe39c49765fdf9d94, 471}, {0x8e41ade9fbebc27d, 475}, {0xb1d219647ae6b31c, 478}, {0xde469fbd99a05fe3, 481}, {0x8aec23d680043bee, 485}, {0xada72ccc20054ae9, 488}, {0xd910f7ff28069da4, 491}, {0x87aa9aff79042286, 495}, {0xa99541bf57452b28, 498}, {0xd3fa922f2d1675f2, 501}, {0x847c9b5d7c2e09b7, 505}, {0xa59bc234db398c25, 508}, {0xcf02b2c21207ef2e, 511}, {0x8161afb94b44f57d, 515}, {0xa1ba1ba79e1632dc, 518}, {0xca28a291859bbf93, 521}, {0xfcb2cb35e702af78, 524}, {0x9defbf01b061adab, 528}, {0xc56baec21c7a1916, 531}, {0xf6c69a72a3989f5b, 534}, {0x9a3c2087a63f6399, 538}, {0xc0cb28a98fcf3c7f, 541}, {0xf0fdf2d3f3c30b9f, 544}, {0x969eb7c47859e743, 548}, {0xbc4665b596706114, 551}, {0xeb57ff22fc0c7959, 554}, {0x9316ff75dd87cbd8, 558}, {0xb7dcbf5354e9bece, 561}, {0xe5d3ef282a242e81, 564}, {0x8fa475791a569d10, 568}, {0xb38d92d760ec4455, 571}, {0xe070f78d3927556a, 574}, {0x8c469ab843b89562, 578}, {0xaf58416654a6babb, 581}, {0xdb2e51bfe9d0696a, 584}, {0x88fcf317f22241e2, 588}, {0xab3c2fddeeaad25a, 591}, {0xd60b3bd56a5586f1, 594}, {0x85c7056562757456, 598}, {0xa738c6bebb12d16c, 601}, {0xd106f86e69d785c7, 604}, {0x82a45b450226b39c, 608}, {0xa34d721642b06084, 611}, {0xcc20ce9bd35c78a5, 614}, {0xff290242c83396ce, 617}, {0x9f79a169bd203e41, 621}, {0xc75809c42c684dd1, 624}, {0xf92e0c3537826145, 627}, {0x9bbcc7a142b17ccb, 631}, {0xc2abf989935ddbfe, 634}, {0xf356f7ebf83552fe, 637}, {0x98165af37b2153de, 641}, {0xbe1bf1b059e9a8d6, 644}, {0xeda2ee1c7064130c, 647}, {0x9485d4d1c63e8be7, 651}, {0xb9a74a0637ce2ee1, 654}, {0xe8111c87c5c1ba99, 657}, {0x910ab1d4db9914a0, 661}, {0xb54d5e4a127f59c8, 664}, {0xe2a0b5dc971f303a, 667}, {0x8da471a9de737e24, 671}, {0xb10d8e1456105dad, 674}, {0xdd50f1996b947518, 677}, {0x8a5296ffe33cc92f, 681}, {0xace73cbfdc0bfb7b, 684}, {0xd8210befd30efa5a, 687}, {0x8714a775e3e95c78, 691}, {0xa8d9d1535ce3b396, 694}, {0xd31045a8341ca07c, 697}, {0x83ea2b892091e44d, 701}, {0xa4e4b66b68b65d60, 704}, {0xce1de40642e3f4b9, 707}, {0x80d2ae83e9ce78f3, 711}, {0xa1075a24e4421730, 714}, {0xc94930ae1d529cfc, 717}, {0xfb9b7cd9a4a7443c, 720}, {0x9d412e0806e88aa5, 724}, {0xc491798a08a2ad4e, 727}, {0xf5b5d7ec8acb58a2, 730}, {0x9991a6f3d6bf1765, 734}, {0xbff610b0cc6edd3f, 737}, {0xeff394dcff8a948e, 740}, {0x95f83d0a1fb69cd9, 744}, {0xbb764c4ca7a4440f, 747}, {0xea53df5fd18d5513, 750}, {0x92746b9be2f8552c, 754}, {0xb7118682dbb66a77, 757}, {0xe4d5e82392a40515, 760}, {0x8f05b1163ba6832d, 764}, {0xb2c71d5bca9023f8, 767}, {0xdf78e4b2bd342cf6, 770}, {0x8bab8eefb6409c1a, 774}, {0xae9672aba3d0c320, 777}, {0xda3c0f568cc4f3e8, 780}, {0x8865899617fb1871, 784}, {0xaa7eebfb9df9de8d, 787}, {0xd51ea6fa85785631, 790}, {0x8533285c936b35de, 794}, {0xa67ff273b8460356, 797}, {0xd01fef10a657842c, 800}, {0x8213f56a67f6b29b, 804}, {0xa298f2c501f45f42, 807}, {0xcb3f2f7642717713, 810}, {0xfe0efb53d30dd4d7, 813}, {0x9ec95d1463e8a506, 817}, {0xc67bb4597ce2ce48, 820}, {0xf81aa16fdc1b81da, 823}, {0x9b10a4e5e9913128, 827}, {0xc1d4ce1f63f57d72, 830}, {0xf24a01a73cf2dccf, 833}, {0x976e41088617ca01, 837}, {0xbd49d14aa79dbc82, 840}, {0xec9c459d51852ba2, 843}, {0x93e1ab8252f33b45, 847}, {0xb8da1662e7b00a17, 850}, {0xe7109bfba19c0c9d, 853}, {0x906a617d450187e2, 857}, {0xb484f9dc9641e9da, 860}, {0xe1a63853bbd26451, 863}, {0x8d07e33455637eb2, 867}, {0xb049dc016abc5e5f, 870}, {0xdc5c5301c56b75f7, 873}, {0x89b9b3e11b6329ba, 877}, {0xac2820d9623bf429, 880}, {0xd732290fbacaf133, 883}, {0x867f59a9d4bed6c0, 887}, {0xa81f301449ee8c70, 890}, {0xd226fc195c6a2f8c, 893}, {0x83585d8fd9c25db7, 897}, {0xa42e74f3d032f525, 900}, {0xcd3a1230c43fb26f, 903}, {0x80444b5e7aa7cf85, 907}, {0xa0555e361951c366, 910}, {0xc86ab5c39fa63440, 913}, {0xfa856334878fc150, 916}, {0x9c935e00d4b9d8d2, 920}, {0xc3b8358109e84f07, 923}, {0xf4a642e14c6262c8, 926}, {0x98e7e9cccfbd7dbd, 930}, {0xbf21e44003acdd2c, 933}, {0xeeea5d5004981478, 936}, {0x95527a5202df0ccb, 940}, {0xbaa718e68396cffd, 943}, {0xe950df20247c83fd, 946}, {0x91d28b7416cdd27e, 950}, {0xb6472e511c81471d, 953}, {0xe3d8f9e563a198e5, 956}, {0x8e679c2f5e44ff8f, 960}};


  static const uint32_t powers_ext[] = {0x6fb92487, 0xa5d3b6d4, 0x8f48a489, 0x331acdab, 0x9ff0c08b, 0x7ecf0ae, 0xc9e82cd9, 0xbe311c08, 0x6dbd630a, 0x92cbbcc, 0x25bbf560, 0xaf2af2b8, 0x1af5af66, 0x50d98d9f, 0xe50ff107, 0x1e53ed49, 0x25e8e89c, 0x77b19161, 0xd59df5b9, 0x4b057328, 0x4ee367f9, 0x229c41f7, 0x6b435275, 0x830a1389, 0x23cc986b, 0x2cbfbe86, 0x7bf7d714, 0xdaf5ccd9, 0xd1b3400f, 0x23100809, 0xabd40a0c, 0x16c90c8f, 0xae3da7d9, 0x99cd11cf, 0x40405643, 0x482835ea, 0xda324365, 0x90bed43e, 0x5a7744a6, 0x711515d0, 0xd5a5b44, 0xe858790a, 0x626e974d, 0xfb0a3d21, 0x7ce66634, 0x1c1fffc1, 0xa327ffb2, 0x4bf1ff9f, 0x6f773fc3, 0xcb550fb4, 0x7e2a53a1, 0x2eda7444, 0xfa911155, 0x793555ab, 0x4bc1558b, 0x9eb1aaed, 0x465e15a9, 0xbfacd89, 0xcef980ec, 0x82b7e127, 0xd1b2ecb8, 0x861fa7e6, 0x67a791e0, 0xe0c8bb2c, 0x58fae9f7, 0xaf39a475, 0x6d8406c9, 0xc8e5087b, 0xfb1e4a9a, 0x5cf2eea0, 0xf42faa48, 0xf13b94da, 0x76c53d08, 0x54768c4b, 0xa9942f5d, 0xd3f93b35, 0xc47bc501, 0x359ab641, 0xc30163d2, 0x79e0de63, 0x985915fc, 0x3e6f5b7b, 0xa705992c, 0x50c6ff78, 0xa4f8bf56, 0x871b7795, 0x28e2557b, 0x331aeada, 0x3ff0d2c8, 0xfed077a, 0xd3e84959, 0x64712dd7, 0xbd8d794d, 0xecf0d7a0, 0xf41686c4, 0x311c2875, 0x7d633293, 0xae5dff9c, 0xd9f57f83, 0xd072df63, 0x4247cb9e, 0x52d9be85, 0x67902e27, 0xba1cd8, 0x80e8a40e, 0x6122cd12, 0x796b8057, 0xcbe33036, 0xbedbfc44, 0xee92fb55, 0x751bdd15, 0xd262d45a, 0x86fb8971, 0xd45d35e6, 0x89748360, 0x2bd1a438, 0x7b6306a3, 0x1a3bc84c, 0x20caba5f, 0x547eb47b, 0xe99e619a, 0x6405fa00, 0xde83bc40, 0x9624ab50, 0x3badd624, 0xe54ca5d7, 0x5e9fcf4c, 0x7647c320, 0x29ecd9f4, 0xf4681071, 0x7182148d, 0xc6f14cd8, 0xb8ada00e, 0xa6d90811, 0x908f4a16, 0x9a598e4e, 0x40eff1e1, 0xd12bee59, 0x82bb74f8, 0xe36a5236, 0xdc44e6c3, 0x29ab103a, 0x7415d448, 0x111b495b, 0xcab10dd9, 0x3d5d514f, 0xcb4a5a3, 0x47f0e785, 0x59ed2167, 0x306869c1, 0x1e414218, 0xe5d1929e, 0xdf45f746, 0x6b8bba8c, 0x66ea92f, 0xc80a537b, 0xbd06742c, 0x2c481138, 0xf75a1586, 0x9a984d73, 0xc13e60d0, 0x318df905, 0xfdf17746, 0xfeb6ea8b, 0xfe64a52e, 0x3dfdce7a, 0x6bea10c, 0x486e494f, 0x5a89dba3, 0xf8962946, 0xf6bbb397, 0x746aa07d, 0xa8c2a44e, 0x92f34d62, 0x77b020ba, 0xace1474, 0xd819992, 0x10e1fff6, 0xca8d3ffa, 0xbd308ff8, 0xac7cb3f6, 0x6bcdf07a, 0x86c16c98, 0xe871c7bf, 0x11471cd7, 0xd598e40d, 0x4aff1d10, 0xcedf722a, 0xc2974eb4, 0x733d2262, 0x806357d, 0xca07c2dc, 0xfc89b393, 0xbbac2078, 0xd54b944b, 0xa9e795e, 0x4d4617b5, 0x504bced1, 0xe45ec286, 0x5d767327, 0x3a6a07f8, 0x890489f7, 0x2b45ac74, 0x3b0b8bc9, 0x9ce6ebb, 0xcc420a6a, 0x9fa94682, 0x47939822, 0x59787e2b, 0x57eb4edb, 0xede62292, 0xe95fab36, 0x11dbcb02, 0xd652bdc2, 0x4be76d33, 0x6f70a440, 0xcb4ccd50, 0x7e2000a4, 0x8ed40066, 0x72890080, 0x4f2b40a0, 0xe2f610c8, 0xdd9ca7d, 0x91503d1c, 0x75a44c63, 0xc986afbe, 0xfbe85bad, 0xfae27299, 0xdccd879f, 0x5400e987, 0x290123e9, 0xf9a0b672, 0xf808e40e, 0xb60b1d12, 0xb1c6f22b, 0x1e38aeb6, 0x25c6da63, 0x579c487e, 0x2d835a9d, 0xf8e43145, 0x1b8e9ecb, 0xe272467e, 0x5b0ed81d, 0x98e94712, 0x3f2398d7, 0x8eec7f0d, 0x1953cf68, 0x5fa8c342, 0x3792f412, 0xe2bbd88b, 0x5b6aceae, 0xf245825a, 0xeed6e2f0, 0x55464dd6, 0xaa97e14c, 0xd53dd99f, 0xe546a803, 0xde985204, 0x963e6685, 0xdde70013, 0x5560c018, 0xaab8f01e, 0xcab39613, 0x3d607b97, 0x8cb89a7d, 0x77f3608e, 0x55f038b2, 0x6b6c46de, 0x2323ac4b, 0xabec975e, 0x96e7bd35, 0x7e50d641, 0xdde50bd1, 0x955e4ec6, 0xbd5af13b, 0xecb1ad8a, 0x67de18ed, 0x80eacf94, 0xa1258379, 0x96ee458, 0x8bca9d6e, 0x775ea264, 0x95364afe, 0x3a83ddbd, 0xc4926a96, 0x75b7053c, 0x5324c68b, 0xd3f6fc16, 0x88f4bb1c, 0x2b31e9e3, 0x3aff322e, 0x9befeb9, 0x4c2ebe68, 0xf9d3701, 0x538484c1, 0x2865a5f2, 0xf93f87b7, 0xf78f69a5, 0xb573440e, 0x31680a88, 0xfdc20d2b, 0x3d329076, 0xa63f9a49, 0xfcf80dc, 0xd3c36113, 0x645a1cac, 0x3d70a3d7, 0xcccccccc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x40000000, 0x50000000, 0xa4000000, 0x4d000000, 0xf0200000, 0x6c280000, 0xc7320000, 0x3c7f4000, 0x4b9f1000, 0x1e86d400, 0x13144480, 0x17d955a0, 0x5dcfab08, 0x5aa1cae5, 0xf14a3d9e, 0x6d9ccd05, 0xe4820023, 0xdda2802c, 0xd50b2037, 0x4526f422, 0x9670b12b, 0x3c0cdd76, 0xa5880a69, 0x8eea0d04, 0x72a49045, 0x47a6da2b, 0x999090b6, 0xfff4b4e3, 0xbff8f10e, 0xaff72d52, 0x9bf4f8a6, 0x2f236d0, 0x1d76242, 0x424d3ad2, 0xd2e08987, 0x63cc55f4, 0x3cbf6b71, 0x8bef464e, 0x97758bf0, 0x3d52eeed, 0x4ca7aaa8, 0x8fe8caa9, 0xb3e2fd53, 0x60dbbca8, 0xbc8955e9, 0x6babab63, 0xc696963c, 0xfc1e1de5, 0x3b25a55f, 0x49ef0eb7, 0x6e356932, 0x49c2c37f, 0xdc33745e, 0x69a028bb, 0xc40832ea, 0xf50a3fa4, 0x792667c6, 0x577001b8, 0xed4c0226, 0x544f8158, 0x696361ae, 0x3bc3a19, 0x4ab48a0, 0x62eb0d64, 0x3ba5d0bd, 0xca8f44ec, 0x7e998b13, 0x9e3fedd8, 0xc5cfe94e, 0xbba1f1d1, 0x2a8a6e45, 0xf52d09d7, 0x593c2626, 0x6f8b2fb0, 0xb6dfb9c, 0x4724bd41, 0x58edec91, 0x2f2967b6, 0xbd79e0d2, 0xecd85906, 0xe80e6f48, 0x3109058d, 0xbd4b46f0, 0x6c9e18ac, 0x3e2cf6b, 0x84db8346, 0xe6126418, 0x4fcb7e8f, 0xe3be5e33, 0x5cadf5bf, 0x73d9732f, 0x2867e7fd, 0xb281e1fd, 0x1f225a7c, 0x3375788d, 0x52d6b1, 0xc0678c5d, 0xf840b7ba, 0xb650e5a9, 0xa3e51f13, 0xc66f336c, 0xb80b0047, 0xa60dc059, 0x87c89837, 0x29babe45, 0xf4296dd6, 0x1899e4a6, 0x5ec05dcf, 0x76707543, 0x6a06494a, 0x487db9d, 0x45a9d284, 0xb8a2392, 0x8e6cac77, 0x3207d795, 0x7f44e6bd, 0x5f16206c, 0x36dba887, 0xc2494954, 0xf2db9baa, 0x6f928294, 0xcb772339, 0xff2a7604, 0xfef51385, 0x7eb25866, 0xef2f773f, 0xaafb550f, 0x95ba2a53, 0xdd945a74, 0x94f97111, 0x7a37cd56, 0xac62e055, 0x577b986b, 0xed5a7e85, 0x14588f13, 0x596eb2d8, 0x6fca5f8e, 0x25de7bb9, 0xaf561aa7, 0x1b2ba151, 0x90fb44d2, 0x353a1607, 0x42889b89, 0x69956135, 0x43fab983, 0x94f967e4, 0x1d1be0ee, 0x6462d92a, 0x7d7b8f75, 0x5cda7352, 0x3a088813, 0x88aaa18, 0x8aad549e, 0x36ac54e2, 0x84576a1b, 0x656d44a2, 0x9f644ae5, 0x873d5d9f, 0xa90cb506, 0x9a7f124, 0xc11ed6d, 0x8f1668c8, 0xf96e017d, 0x37c981dc, 0x85bbe253, 0x93956d74, 0x387ac8d1, 0x6997b05, 0x441fece3, 0xd527e81c, 0x8a71e223, 0xf6872d56, 0xb428f8ac, 0xe13336d7, 0xecc00246, 0x27f002d7, 0x31ec038d, 0x7e670471, 0xf0062c6, 0x52c07b78, 0xa7709a56, 0x88a66076, 0x6acff893, 0x583f6b8, 0xc3727a33, 0x744f18c0, 0x1162def0, 0x8addcb56, 0x6d953e2b, 0xc8fa8db6, 0x1d9c9892, 0x2503beb6, 0x2e44ae64, 0x5ceaecfe, 0x7425a83e, 0xd12f124e, 0x82bd6b70, 0x636cc64d, 0x3c47f7e0, 0x65acfaec, 0x7f1839a7, 0x1ede4811, 0x934aed0a, 0xf81da84d, 0x36251260, 0xc1d72b7c, 0xb24cf65b, 0xdee033f2, 0x169840ef, 0x8e1f2895, 0xf1a6f2ba, 0xae10af69, 0xacca6da1, 0x17fd090a, 0xddfc4b4c, 0x4abdaf10, 0x9d6d1ad4, 0x84c86189, 0x32fd3cf5, 0x3fbc8c33, 0xfabaf3f, 0x29cb4d87, 0x743e20e9, 0x914da924, 0x1ad089b6, 0xa184ac24, 0xc9e5d72d, 0x7e2fa67c, 0xddbb901b, 0x552a7422, 0xd53a8895, 0x8a892aba, 0x2d2b7569, 0x9c3b2962, 0x8349f3ba, 0x241c70a9, 0xed238cd3, 0xf4363804, 0xb143c605, 0xdd94b786, 0xca7cf2b4, 0xfd1c2f61, 0xbc633b39, 0xd5be0503, 0x4b2d8644, 0xddf8e7d6, 0xcabb90e5, 0x3d6a751f, 0xcc51267, 0x27fb2b80, 0xb1f9f660, 0x5e7873f8, 0xdb0b487b, 0x91ce1a9a, 0x7641a140, 0xa9e904c8, 0x546345fa, 0xa97c1779, 0x49ed8eab, 0x5c68f256, 0x73832eec, 0xc831fd53, 0xba3e7ca8, 0x28ce1bd2, 0x7980d163, 0xd7e105bc, 0x8dd9472b, 0xb14f98f6, 0x6ed1bf9a, 0xa862f80, 0xcd27bb61, 0x8038d51c, 0xe0470a63, 0x1858ccfc, 0xf37801e, 0xd3056025, 0x47c6b82e, 0x4cdc331d, 0xe0133fe4, 0x58180fdd, 0x570f09ea};


  // handle case where strtod finds an invalid number. won't we have a buffer overflow if it's just numbers past the end?
  static really_inline bool compute_float_64(uint64_t power_index, uint64_t i, bool negative, double *dd) {
    components c = power_of_ten_components[power_index];
    uint64_t factor_mantissa = c.mantissa;
    int lz = leading_zeroes(i);
    i <<= lz;
    __uint128_t large_mantissa = (__uint128_t)i * factor_mantissa;
    uint64_t upper = large_mantissa >> 64;
    if (unlikely((upper & 0x1FF) == 0x1FF)) {
      return false;
    }
    uint64_t mantissa = 0;
    if (upper & (1ULL << 63)) {
      mantissa = upper >> 10;
    } else {
      mantissa = upper >> 9;
      lz++;
    }
    mantissa += mantissa & 1;
    mantissa >>= 1;
    mantissa &= ~(1ULL << 52);
    uint64_t real_exponent = c.exp + 1023 + (127 - lz);
    mantissa |= real_exponent << 52;
    mantissa |= (((uint64_t)negative) << 63);
    double d;
    memcpy(&d, &mantissa, sizeof(d));
    *dd = d;
    return true;
  }

  static const int powersOf10[] = {1, 10, 100, 1000};

  static double never_inline compute_float_128(uint64_t power_index, uint64_t i_64, bool negative, bool *success) {
    components c = power_of_ten_components[power_index];
    uint64_t factor_mantissa = c.mantissa;
    __uint128_t i = i_64;
    int i_lz = leading_zeroes(i_64);
    i <<= i_lz;
    __uint128_t m = ((__uint128_t)factor_mantissa << 32) | powers_ext[power_index];
    const uint64_t first_forty_eight_mask = (1ULL << 48) - 1;
    __uint128_t lower_part = i * (m & first_forty_eight_mask);
    __uint128_t upper128 = i * (m >> 48);
    uint64_t upper = upper128 >> (128 - 48);
    __uint128_t lower = lower_part + (upper128 << 48);
    if (lower < lower_part) { // overflow occurred
      upper++;
    }
    int lz = leading_zeroes(upper);
    int safeBits = 192 - lz - 54;
    __uint128_t max_lower = lower + i;
    __uint128_t unsafe_mask = (~((__uint128_t)0)) << safeBits;
    if ((max_lower & unsafe_mask) != (lower & unsafe_mask)) {
      *success = false;
      return 0;
    }
    uint64_t mantissa = upper << (128 - safeBits) | lower >> safeBits;
    mantissa += mantissa & 1;
    mantissa >>= 1;
    mantissa &= ~(1ULL << 52);
    uint64_t real_exponent = c.exp - 32 - i_lz + 1023 + safeBits + 53;
    mantissa |= real_exponent << 52; // lz > 64?
    mantissa |= ((uint64_t)negative) << 63; // is this safe? is this bool in [0, 1]?
    double d;
    memcpy(&d, &mantissa, sizeof(d));
    *success = true;
    return d;
  }


  static really_inline bool compute_float_double(int64_t power_index, uint64_t i, bool negative, double *dd) {
    double double_threshold = 9007199254740991.0; // 2 ** 53 - 1
    if (i > (uint64_t)double_threshold) {
      return false;
    }
    double d = i;
    if (308 + 22 < power_index && power_index < 308 + 22 + 16) {
      d *= power_of_ten[power_index - 22];
      power_index = 22 + 308;
    }
    if (308 + -22 <= power_index && power_index <= 308 + 22 && d <= double_threshold) {
      if (power_index < 308) {
        d = d / power_of_ten[308 * 2 - power_index];
      } else {
        d = d * power_of_ten[power_index];
      }
      if (negative) {
        d = -d;
      }
      *dd = d;
    }
    return false;
  }


  static uint32_t parse_float_strtod(const uint8_t *const buf, ParsedJson &pj,
                                              const uint32_t offset, const char *float_end) {
    double d = strtod((char *)(buf + offset), NULL);
    pj.write_tape_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_float(d, buf + offset);
#endif
    return is_structural_or_whitespace(*float_end);
  }

// called by parse_number when we know that the output is an integer,
// but where there might be some integer overflow.
// we want to catch overflows!
// Do not call this function directly as it skips some of the checks from
// parse_number
//
// This function will almost never be called!!!
//
static never_inline bool parse_large_integer(const uint8_t *const buf,
                                             ParsedJson &pj,
                                             const uint32_t offset,
                                             bool found_minus) {
  const char *p = reinterpret_cast<const char *>(buf + offset);

  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
  }
  uint64_t i;
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    i = 0;
  } else {
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      if (mul_overflow(i, 10, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false; // overflow
      }
      if (add_overflow(i, digit, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false; // overflow
      }
      ++p;
    }
  }
  if (negative) {
    if (i > 0x8000000000000000) {
       // overflows!
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false; // overflow
    } else if (i == 0x8000000000000000) {
      // In two's complement, we cannot represent 0x8000000000000000
      // as a positive signed integer, but the negative version is 
      // possible.
      constexpr int64_t signed_answer = INT64_MIN;
      pj.write_tape_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, buf + offset);
#endif
    } else {
      // we can negate safely
      int64_t signed_answer = -static_cast<int64_t>(i);
      pj.write_tape_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, buf + offset);
#endif
    }
  } else {
    // we have a positive integer, the contract is that
    // we try to represent it as a signed integer and only 
    // fallback on unsigned integers if absolutely necessary.
    if(i < 0x8000000000000000) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(i, buf + offset);
#endif
      pj.write_tape_s64(i);
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_unsigned_integer(i, buf + offset);
#endif
      pj.write_tape_u64(i);
    }
  }
  return is_structural_or_whitespace(*p);
}

// parse the number at buf + offset
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0 or 1) at high speed.
static really_inline bool parse_number(const uint8_t *const buf, ParsedJson &pj,
                                       const uint32_t offset,
                                       bool found_minus) {
#ifdef SIMDJSON_SKIPNUMBERPARSING // for performance analysis, it is sometimes
                                  // useful to skip parsing
  pj.write_tape_s64(0);           // always write zero
  return true;                    // always succeeds
#else
  const char *p = reinterpret_cast<const char *>(buf + offset);
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
    if (!is_integer(*p)) { // a negative sign must be followed by an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
  }
  const char *const start_digits = p;

  uint64_t i;      // an unsigned int avoids signed overflows (which are bad)
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    if (is_not_structural_or_whitespace_or_exponent_or_decimal(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    i = 0;
  } else {
    if (!(is_integer(*p))) { // must start with an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      // a multiplication by 10 is cheaper than an arbitrary integer
      // multiplication
      i = 10 * i + digit; // might overflow, we will handle the overflow later
      ++p;
    }
  }
  int64_t exponent = 0;
  bool is_float = false;
  const char *first_after_period = NULL;
  if ('.' == *p) {
    is_float = true; // At this point we know that we have a float
    // we continue with the fiction that we have an integer. If the
    // floating point number is representable as x * 10^z for some integer
    // z that fits in 53 bits, then we will be able to convert back the
    // the integer into a float in a lossless manner.
    ++p;
    first_after_period = p;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // might overflow + multiplication by 10 is likely
                          // cheaper than arbitrary mult.
      // we will handle the overflow later
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
#ifdef SWAR_NUMBER_PARSING
    // this helps if we have lots of decimals!
    // this turns out to be frequent enough.
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
    }
#endif
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
                          // because we have parse_highprecision_float later.
    }
    exponent = first_after_period - p;
  }
  int digit_count =
      p - start_digits - 1; // used later to guard against overflows
  int64_t exp_number = 0;   // exponential part
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(buf + offset);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    exp_number = digit;
    p++;
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (is_integer(*p)) {
      if (exp_number > 0x100000000) { // we need to check for overflows
                                      // we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(buf + offset);
#endif
        return false;
      }
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    exponent += (neg_exp ? -exp_number : exp_number);
  }
  if (is_float) {
    uint64_t power_index = 308 + exponent;
    if (unlikely((digit_count >= 19))) { // this is uncommon
      // It is possible that the integer had an overflow.
      // We have to handle the case where we have 0.0000somenumber.
      const char *start = start_digits;
      while ((*start == '0') || (*start == '.')) {
        start++;
      }
      // we over-decrement by one when there is a '.'
      digit_count -= (start - start_digits);
      if (digit_count >= 19) {
        return parse_float_strtod(buf, pj, offset, p);
      }
    }
    if (unlikely((power_index > 2 * 308))) { // this is uncommon!!!
      // this is almost never going to get called!!!
      return parse_float_strtod(buf, pj, offset, p);
    }
    double d = 0;
    if (likely(i != 0)) {
      if (!compute_float_double(power_index, i, negative, &d)) {
        if (!compute_float_64(power_index, i, negative, &d)) {
          bool success = true;
          d = compute_float_128(power_index, i, negative, &success);
          if (!success) {
            return parse_float_strtod(buf, pj, offset, p);
          }
        }
      }
    }
    pj.write_tape_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_float(d, buf + offset);
#endif
  } else {
    if (unlikely(digit_count >= 18)) { // this is uncommon!!!
      // there is a good chance that we had an overflow, so we need
      // need to recover: we parse the whole thing again.
      return parse_large_integer(buf, pj, offset, found_minus);
    }
    i = negative ? 0 - i : i;
    pj.write_tape_s64(i);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_integer(i, buf + offset);
#endif
  }
  return is_structural_or_whitespace(*p);
#endif // SIMDJSON_SKIPNUMBERPARSING
}
} // simdjson
#endif
