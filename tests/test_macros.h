#ifndef TEST_MACROS_H
#define TEST_MACROS_H

#ifndef SIMDJSON_BENCHMARK_DATA_DIR
#define SIMDJSON_BENCHMARK_DATA_DIR "jsonexamples/"
#endif
const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";
const char *TWITTER_TIMELINE_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter_timeline.json";
const char *REPEAT_JSON = SIMDJSON_BENCHMARK_DATA_DIR "repeat.json";
const char *CANADA_JSON = SIMDJSON_BENCHMARK_DATA_DIR "canada.json";
const char *MESH_JSON = SIMDJSON_BENCHMARK_DATA_DIR "mesh.json";
const char *APACHE_JSON = SIMDJSON_BENCHMARK_DATA_DIR "apache_builds.json";
const char *GSOC_JSON = SIMDJSON_BENCHMARK_DATA_DIR "gsoc-2018.json";

const char *AMAZON_CELLPHONES_NDJSON = SIMDJSON_BENCHMARK_DATA_DIR "amazon_cellphones.ndjson";

#define SIMDJSON_BENCHMARK_SMALLDATA_DIR SIMDJSON_BENCHMARK_DATA_DIR "small/"

const char *ADVERSARIAL_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "adversarial.json";
const char *FLATADVERSARIAL_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "flatadversarial.json";
const char *DEMO_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "demo.json";
const char *SMALLDEMO_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "smalldemo.json";
const char *TRUENULL_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "truenull.json";

// For the ASSERT_EQUAL macro
template<typename T, typename S>
simdjson_really_inline bool equals_expected(T actual, S expected) {
  return actual == T(expected);
}
template<>
simdjson_really_inline bool equals_expected<const char *, const char *>(const char *actual, const char *expected) {
  return !strcmp(actual, expected);
}

simdjson_really_inline simdjson::error_code to_error_code(simdjson::error_code error) {
  return error;
}
template<typename T>
simdjson_really_inline simdjson::error_code to_error_code(const simdjson::simdjson_result<T> &result) {
  return result.error();
}

#define TEST_START() { std::cout << "Running " << __func__ << " ..." << std::endl; }
#define SUBTEST(NAME, TEST) \
{ \
  std::cout << "- Subtest " << (NAME) << " ..." << std::endl; \
  bool succeeded = (TEST); \
  ASSERT(succeeded, "Subtest " NAME " failed"); \
}
#define ASSERT_EQUAL(ACTUAL, EXPECTED)        \
do {                                          \
  auto _actual = (ACTUAL);                    \
  auto _expected = (EXPECTED);                \
  if (!equals_expected(_actual, _expected)) { \
    std::cerr << "Expected " << (#ACTUAL) << " to be " << _expected << ", got " << _actual << " instead!" << std::endl; \
    return false;                             \
  }                                           \
} while(0);
#define ASSERT_ERROR(ACTUAL, EXPECTED) do { auto _actual = to_error_code(ACTUAL); auto _expected = to_error_code(EXPECTED); if (_actual != _expected) { std::cerr << "FAIL: Unexpected error \"" << _actual << "\" (expected \"" << _expected << "\")" << std::endl; return false; } } while (0);
#define ASSERT_TRUE(RESULT) if (!(RESULT)) { std::cerr << "False invariant: " << #RESULT << std::endl; return false; }
#define ASSERT(RESULT, MESSAGE) if (!(RESULT)) { std::cerr << MESSAGE << std::endl; return false; }
#define RUN_TEST(RESULT) if (!RESULT) { return false; }
#define ASSERT_SUCCESS(ERROR) do { auto _error = to_error_code(ERROR); if (_error) { std::cerr << "Expected success, got error: " << _error << std::endl; return false; } } while(0);
#define TEST_FAIL(MESSAGE) { std::cerr << "FAIL: " << (MESSAGE) << std::endl; return false; }
#define TEST_SUCCEED() { return true; }


#endif // TEST_MACROS_H