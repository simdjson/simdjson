#include <cmath>
#include <cstring>
#include <limits>
#include <random>
#include <string>
#include "simdjson.h"
#include "test_ondemand.h"

// Reference conversions go through strtof() in the C locale, following what
// fast_float does in its own test suite (fast_float/tests/*.cpp) and what
// tests/dom/numberparsingcheck.cpp already does for binary64: a plain strtof()
// picks up the ambient locale, so it may not even accept '.' as the decimal
// separator, and Visual Studio spells the locale-aware entry point differently.
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) ||     \
    defined(sun) || defined(__sun)
// Anything related to cygwin, msys or the Solaris runtimes cannot be relied on
// to have a correctly rounded strtof(), so we read through a C++ stream imbued
// with the classic locale instead. This mirrors fast_float's workaround.
#define SIMDJSON_STRTOF_VIA_STREAM 1
#include <locale>
#include <sstream>
#else
#include <locale.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <xlocale.h>
#endif
#endif

using namespace simdjson;

namespace float_tests {

  // strtof() in the C locale.
  float reference_strtof(const char *st) {
#if defined(SIMDJSON_STRTOF_VIA_STREAM)
    float d;
    std::stringstream ss;
    ss.imbue(std::locale::classic());
    ss << st;
    ss >> d;
    return d;
#elif defined(_WIN32)
    static _locale_t c_locale = _create_locale(LC_ALL, "C");
    return _strtof_l(st, nullptr, c_locale);
#else
    static locale_t c_locale = newlocale(LC_ALL_MASK, "C", NULL);
    return strtof_l(st, nullptr, c_locale);
#endif
  }

  // Compare binary32 values bit by bit: we want the *nearest* float, so 'close
  // enough' is not good enough, and this also pins down the sign of zero.
  simdjson_inline uint32_t float_bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    return u;
  }

  bool assert_float_bits(float actual, uint32_t expected_bits, const std::string &json) {
    if (float_bits(actual) != expected_bits) {
      float expected;
      std::memcpy(&expected, &expected_bits, sizeof(expected));
      std::cerr << "Parsing " << json << " gave " << actual << " (0x"
                << std::hex << float_bits(actual) << ") instead of " << std::dec
                << expected << " (0x" << std::hex << expected_bits
                << std::dec << ")" << std::endl;
      return false;
    }
    return true;
  }

  bool assert_float_equal(float actual, float expected, const std::string &json) {
    return assert_float_bits(actual, float_bits(expected), json);
  }

  // Parses [<num>] and checks the array element against strtof.
  bool check_value(const std::string &num) {
    std::string json = "[" + num + "]";
    padded_string doc_data(json);
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(doc_data).get(doc));
    ondemand::array arr;
    ASSERT_SUCCESS(doc.get_array().get(arr));
    for (auto val : arr) {
      float actual;
      ASSERT_SUCCESS(val.get_float().get(actual));
      if (!assert_float_equal(actual, reference_strtof(num.c_str()), json)) { return false; }
    }
    return true;
  }

  // Parses [<num>] and checks the array element against a known bit pattern.
  // Use this instead of check_value() when strtof() cannot be trusted as an
  // oracle. Exact ties are such a case: Microsoft's runtime turns
  // 4.915311914062500...e+04, which is exactly halfway between 0x4740011e and
  // 0x4740011f, into the odd 0x4740011f rather than the even 0x4740011e that
  // IEEE 754 asks for.
  bool check_value_bits(const std::string &num, uint32_t expected_bits) {
    std::string json = "[" + num + "]";
    padded_string doc_data(json);
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(doc_data).get(doc));
    ondemand::array arr;
    ASSERT_SUCCESS(doc.get_array().get(arr));
    for (auto val : arr) {
      float actual;
      ASSERT_SUCCESS(val.get_float().get(actual));
      if (!assert_float_bits(actual, expected_bits, json)) { return false; }
    }
    return true;
  }

  // Parses <num> as a whole document (the root code path).
  bool check_root_value(const std::string &num) {
    padded_string doc_data(num);
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(doc_data).get(doc));
    float actual;
    ASSERT_SUCCESS(doc.get_float().get(actual));
    return assert_float_equal(actual, reference_strtof(num.c_str()), num);
  }

  // Parses ["<num>"] and checks the array element against strtof.
  bool check_value_in_string(const std::string &num) {
    std::string json = "[\"" + num + "\"]";
    padded_string doc_data(json);
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(doc_data).get(doc));
    ondemand::array arr;
    ASSERT_SUCCESS(doc.get_array().get(arr));
    for (auto val : arr) {
      float actual;
      ASSERT_SUCCESS(val.get_float_in_string().get(actual));
      if (!assert_float_equal(actual, reference_strtof(num.c_str()), json)) { return false; }
    }
    return true;
  }

  bool basic_values() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    padded_string docdata;
    float val;

    docdata = "0.0"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(float_bits(val), float_bits(0.0f));

    docdata = "-0.0"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(float_bits(val), float_bits(-0.0f));

    docdata = "1.5"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(val, 1.5f);

    docdata = "-3.25e2"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(val, -325.0f);

    // Integers are accepted by get_float(), just like they are by get_double().
    docdata = "17"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(val, 17.0f);

    // 2**24 + 1 is not representable: it must round to 2**24.
    docdata = "16777217"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(val, 16777216.0f);

    TEST_SUCCEED();
  }

  bool out_of_range() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    padded_string docdata;
    float val;

    // Larger than FLT_MAX: simdjson refuses infinite values.
    docdata = "1e39"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_ERROR(doc.get_float().get(val), NUMBER_ERROR);

    docdata = "-1e39"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_ERROR(doc.get_float().get(val), NUMBER_ERROR);

    docdata = "3.5e38"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_ERROR(doc.get_float().get(val), NUMBER_ERROR);

    // These are finite doubles but infinite floats.
    docdata = "1e300"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_ERROR(doc.get_float().get(val), NUMBER_ERROR);

    // Smaller than the smallest subnormal: underflows to zero (with sign).
    docdata = "1e-46"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(float_bits(val), float_bits(0.0f));

    docdata = "-1e-46"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(float_bits(val), float_bits(-0.0f));

    // Below the binary64 range entirely, so also zero as a float.
    docdata = "1e-400"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(float_bits(val), float_bits(0.0f));

    // Largest finite float, and the first value that rounds up to infinity.
    docdata = "3.4028234663852886e38"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_SUCCESS(doc.get_float().get(val));
    ASSERT_EQUAL(val, (std::numeric_limits<float>::max)());

    TEST_SUCCEED();
  }

  bool subnormals() {
    TEST_START();
    // Walk the smallest floats, including every subnormal near zero, printing
    // them with enough digits to round-trip.
    for (uint32_t u = 0; u < 4096; u++) {
      float f;
      std::memcpy(&f, &u, sizeof(f));
      char buf[64];
      std::snprintf(buf, sizeof(buf), "%.9g", double(f));
      if (!check_value(buf)) { return false; }
      std::snprintf(buf, sizeof(buf), "-%.9g", double(f));
      if (!check_value(buf)) { return false; }
    }
    // The smallest subnormal is 1.4012984643e-45; half of it rounds to zero,
    // and just above half rounds up to the smallest subnormal.
    if (!check_value("7.0064923216240854e-46")) { return false; }
    if (!check_value("7.006492321624086e-46")) { return false; }
    TEST_SUCCEED();
  }

  // Rounding to binary64 and then to binary32 rounds twice and does not always
  // land on the float nearest to the decimal input. These are inputs where the
  // two-step conversion is demonstrably wrong; get_float() must get them right.
  bool no_double_rounding() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    float val;

    // Each of these is a hair above the midpoint between two consecutive
    // floats, so it must round *up*. Rounding to binary64 first collapses it
    // onto the midpoint exactly, which then rounds half-to-even back *down*.
    struct { const char *json; uint32_t expected_bits; } cases[] = {
      {"1.0000000596046447753906250000000000000000000000000000000000001", 0x3f800001},
      {"1.0000002980232238769531250000000000000000000000000000000000001", 0x3f800003},
      {"1.0000005364418029785156250000000000000000000000000000000000001", 0x3f800005},
      {"1.0000007748603820800781250000000000000000000000000000000000001", 0x3f800007},
    };
    for (auto &c : cases) {
      padded_string docdata(std::string(c.json));
      ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
      ASSERT_SUCCESS(doc.get_float().get(val));
      ASSERT_EQUAL(float_bits(val), c.expected_bits);
      // Confirm the premise: going through binary64 gives a different answer.
      double as_double;
      ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
      ASSERT_SUCCESS(doc.get_double().get(as_double));
      ASSERT_TRUE(float_bits(static_cast<float>(as_double)) != c.expected_bits);
    }
    TEST_SUCCEED();
  }

  // Strips the trailing zeros from the significand of a "d.dddde+XX" string, so
  // that an exact value printed with a generous precision comes back in minimal
  // form. simdjson counts every digit it sees, trailing zeros included, when it
  // decides between the fast path and the fallback, so this is what lets a short
  // tie reach the fast path.
  std::string trim_zeros(const std::string &s) {
    size_t e = s.find('e');
    if (e == std::string::npos || s.find('.') == std::string::npos) { return s; }
    size_t last = s.find_last_not_of('0', e - 1);
    if (s[last] == '.') { last--; }
    std::string out = s.substr(0, last + 1);
    out.append(s, e, std::string::npos);
    return out;
  }

  // Counts the digits in the significand of a "d.dddde+XX" string.
  size_t significand_digits(const std::string &s) {
    size_t n = 0;
    for (char c : s) {
      if (c == 'e') { break; }
      if (c >= '0' && c <= '9') { n++; }
    }
    return n;
  }

  // Midpoints between consecutive floats: an exact midpoint must round to even,
  // and a value a hair off one must round to the neighbour it is closest to.
  // This is the sweep fast_float runs in tests/exhaustive32_midpoint.cpp, and it
  // is the hardest input for the fast path, which bails out to the fallback
  // precisely when it cannot tell which side of a midpoint it is on.
  bool round_to_even() {
    TEST_START();
    for (uint32_t u = 1; u < 0x7F7FFFFF; u += 0x00400001) {
      float lo, hi;
      uint32_t uh = u + 1;
      std::memcpy(&lo, &u, sizeof(lo));
      std::memcpy(&hi, &uh, sizeof(hi));
      // The midpoint has a 25-bit significand, so it is exact as a binary64.
      double mid = (double(lo) + double(hi)) / 2;
      char buf[512];
      // The exact midpoint must be printed in full: the smallest one is
      // 3 * 2**-150, whose decimal expansion runs to 106 significant digits, and
      // a *rounded* midpoint is no longer a tie.
      std::snprintf(buf, sizeof(buf), "%.170e", mid);
      std::string exact = trim_zeros(buf);
      // A tie rounds to whichever neighbour has an even significand, and exactly
      // one of u and u + 1 has its low bit clear. (That holds across a binade
      // boundary too, where the larger value has an all-zero significand.) We
      // work the answer out here instead of asking strtof(), because a tie is
      // exactly where the platform runtimes stop agreeing with each other; see
      // check_value_bits().
      uint32_t even = (u & 1) ? uh : u;
      if (!check_value_bits(buf, even)) { return false; }   // zero-padded: fallback path
      if (!check_value_bits(exact, even)) { return false; } // minimal: fast path when short
      // fast_float prints the midpoint at binary64 round-trip precision, which
      // for most exponents lands just off the tie. strtof() is a sound oracle
      // there, and it covers the same hard-rounding ground with a second set of
      // inputs. Skip it when 17 digits happen to spell the tie exactly.
      if (significand_digits(exact) >
          size_t(std::numeric_limits<double>::max_digits10)) {
        std::snprintf(buf, sizeof(buf), "%.*e",
                      std::numeric_limits<double>::max_digits10 - 1, mid);
        if (!check_value(buf)) { return false; }
      }
    }
    TEST_SUCCEED();
  }

  // More than 19 significant digits sends us down the slow fallback path.
  bool many_digits() {
    TEST_START();
    std::mt19937_64 rng(1234);
    for (int trial = 0; trial < 2000; trial++) {
      std::string s;
      if (rng() & 1) { s += "-"; }
      s += char('1' + int(rng() % 9));
      s += ".";
      int ndigits = 20 + int(rng() % 40);
      for (int k = 0; k < ndigits; k++) { s += char('0' + int(rng() % 10)); }
      // Appended in two steps: 'e' + std::to_string(...) inserts into a
      // temporary and trips a bogus -Wrestrict warning with GCC 12.
      s += "e";
      s += std::to_string(int(rng() % 60) - 40);
      if (!check_value(s)) { return false; }
    }
    // A long run of digits that is well within range.
    if (!check_value("3.14159265358979323846264338327950288419716939937510")) { return false; }
    if (!check_value("0.000000000000000000000000000000000000000000001401298464324817")) { return false; }
    TEST_SUCCEED();
  }

  // Random bit patterns, printed so that they round-trip, over every exponent.
  bool round_trip() {
    TEST_START();
    std::mt19937_64 rng(20240815);
    for (int trial = 0; trial < 20000; trial++) {
      uint32_t u = uint32_t(rng());
      float f;
      std::memcpy(&f, &u, sizeof(f));
      if (!std::isfinite(f)) { continue; }
      char buf[64];
      std::snprintf(buf, sizeof(buf), "%.9g", double(f));
      if (!check_value(buf)) { return false; }
      if (!check_root_value(buf)) { return false; }
      if (!check_value_in_string(buf)) { return false; }
    }
    TEST_SUCCEED();
  }

  bool wrong_type() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    padded_string docdata;
    float val;

    docdata = "\"hello\""_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_ERROR(doc.get_float().get(val), INCORRECT_TYPE);

    docdata = "true"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_ERROR(doc.get_float().get(val), INCORRECT_TYPE);

    docdata = "null"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_ERROR(doc.get_float().get(val), INCORRECT_TYPE);

    // Malformed numbers.
    docdata = "[1.]"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    for (auto v : doc.get_array()) {
      ASSERT_ERROR(v.get_float().get(val), NUMBER_ERROR);
    }

    docdata = "[01.5]"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    for (auto v : doc.get_array()) {
      ASSERT_ERROR(v.get_float().get(val), NUMBER_ERROR);
    }

    docdata = "[1e]"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    for (auto v : doc.get_array()) {
      ASSERT_ERROR(v.get_float().get(val), NUMBER_ERROR);
    }

    TEST_SUCCEED();
  }

  // get<float>() must use the direct binary32 path, not get_double() + cast.
  bool get_template() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;

    padded_string docdata =
        "1.0000000596046447753906250000000000000000000000000000000000001"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    float val;
    ASSERT_SUCCESS(doc.get<float>().get(val));
    ASSERT_EQUAL(float_bits(val), 0x3f800001u);

    docdata = "[1.5, 2.5, 3.5]"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::array arr;
    ASSERT_SUCCESS(doc.get_array().get(arr));
    float sum = 0;
    for (auto v : arr) {
      float f;
      ASSERT_SUCCESS(v.get<float>().get(f));
      sum += f;
    }
    ASSERT_EQUAL(sum, 7.5f);

#if SIMDJSON_SUPPORTS_CONCEPTS
    // The std::vector<float> deserializer must round to binary32 directly too.
    docdata = "[1.0000000596046447753906250000000000000000000000000000000000001]"_padded;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    std::vector<float> vec;
    ASSERT_SUCCESS(doc.get<std::vector<float>>().get(vec));
    ASSERT_EQUAL(vec.size(), 1);
    ASSERT_EQUAL(float_bits(vec[0]), 0x3f800001u);
#endif

    TEST_SUCCEED();
  }

  bool run() {
    return basic_values() &&
           out_of_range() &&
           subnormals() &&
           no_double_rounding() &&
           round_to_even() &&
           many_digits() &&
           round_trip() &&
           wrong_type() &&
           get_template();
  }

} // namespace float_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, float_tests::run);
}
