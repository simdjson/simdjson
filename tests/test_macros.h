#ifndef TEST_MACROS_H
#define TEST_MACROS_H

#ifndef SIMDJSON_BENCHMARK_DATA_DIR
#define SIMDJSON_BENCHMARK_DATA_DIR "jsonexamples/"
#endif
const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";
const char *TWITTER_TIMELINE_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter_timeline.json";
const char *REPEAT_JSON = SIMDJSON_BENCHMARK_DATA_DIR "repeat.json";
const char *AMAZON_CELLPHONES_NDJSON = SIMDJSON_BENCHMARK_DATA_DIR "amazon_cellphones.ndjson";

#define SIMDJSON_BENCHMARK_SMALLDATA_DIR SIMDJSON_BENCHMARK_DATA_DIR "small/"

const char *ADVERSARIAL_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "adversarial.json";
const char *FLATADVERSARIAL_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "flatadversarial.json";
const char *DEMO_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "demo.json";
const char *SMALLDEMO_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "smalldemo.json";
const char *TRUENULL_JSON = SIMDJSON_BENCHMARK_SMALLDATA_DIR  "truenull.json";

// For the ASSERT_EQUAL macro
template<typename T>
bool equals_expected(T actual, T expected) {
  return actual == expected;
}
template<>
bool equals_expected<const char *>(const char *actual, const char *expected) {
  return !strcmp(actual, expected);
}
#define ASSERT_EQUAL(ACTUAL, EXPECTED) if (!equals_expected(ACTUAL, EXPECTED)) { std::cerr << "Expected " << #ACTUAL << " to be " << (EXPECTED) << ", got " << (ACTUAL) << " instead!" << std::endl; return false; }
#define ASSERT(RESULT, MESSAGE) if (!(RESULT)) { std::cerr << MESSAGE << std::endl; return false; }
#define ASSERT_SUCCESS(ERROR) if (ERROR) { std::cerr << (ERROR) << std::endl; return false; }

#endif // TEST_MACROS_H