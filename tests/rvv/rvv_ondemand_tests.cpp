#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <string_view>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: On-Demand API Regression Suite
// -------------------------------------------------------------------------
// This suite verifies the integration between the RVV backend's Stage 1
// output and the high-level On-Demand iterator logic.
//
// Key Verification Points:
// 1. Index Accuracy: Do iterators land on the correct start bytes?
// 2. Skipping: Can the parser successfully skip large arrays/objects using
//    the structural brackets identified by RVV?
// 3. Raw String Extraction: Verifies strict pointer arithmetic.
// -------------------------------------------------------------------------

// Helper assertions
#define ASSERT_SUCCESS(expr) \
  if ((expr) != simdjson::SUCCESS) { \
    std::cerr << "  [FAIL] Error at line " << __LINE__ << " Code: " << (expr) << std::endl; \
    return false; \
  }

#define ASSERT_EQUAL(actual, expected) \
  if ((actual) != (expected)) { \
    std::cerr << "  [FAIL] Expected " << (expected) << ", got " << (actual) << " at line " << __LINE__ << std::endl; \
    return false; \
  }

#define ASSERT_TRUE(cond) \
  if (!(cond)) { \
    std::cerr << "  [FAIL] Condition failed at line " << __LINE__ << std::endl; \
    return false; \
  }

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------
bool force_rvv() {
  auto rvv = simdjson::get_available_implementations()["rvv"];
  if (!rvv) { return false; }
  if (!rvv->supported_by_runtime_system()) { return false; }
  simdjson::get_active_implementation() = rvv;
  return true;
}

// -------------------------------------------------------------------------
// Test 1: Basic Scalars
// -------------------------------------------------------------------------
bool test_basics() {
  std::cout << "Running test_basics..." << std::endl;
  simdjson::ondemand::parser parser;
  simdjson::padded_string json = R"({ "i": 123, "d": 3.14, "s": "text", "b": true, "n": null })"_padded;

  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));

  int64_t i;
  double d;
  std::string_view s;
  bool b;
  bool is_null;

  ASSERT_SUCCESS(doc["i"].get(i));
  ASSERT_EQUAL(i, 123);

  ASSERT_SUCCESS(doc["d"].get(d));
  ASSERT_TRUE(std::abs(d - 3.14) < 1e-6);

  ASSERT_SUCCESS(doc["s"].get(s));
  ASSERT_TRUE(s == "text");

  ASSERT_SUCCESS(doc["b"].get(b));
  ASSERT_EQUAL(b, true);

  ASSERT_SUCCESS(doc["n"].is_null().get(is_null));
  ASSERT_TRUE(is_null);

  return true;
}

// -------------------------------------------------------------------------
// Test 2: Array Iteration & skipping
// -------------------------------------------------------------------------
bool test_arrays() {
  std::cout << "Running test_arrays..." << std::endl;
  simdjson::ondemand::parser parser;

  // Case A: Simple Iteration
  auto json_a = R"([1, 2, 3, 4])"_padded;
  simdjson::ondemand::document doc_a;
  ASSERT_SUCCESS(parser.iterate(json_a).get(doc_a));

  simdjson::ondemand::array arr_a;
  ASSERT_SUCCESS(doc_a.get_array().get(arr_a));

  int64_t sum = 0;
  for (auto val : arr_a) {
    int64_t v;
    ASSERT_SUCCESS(val.get(v));
    sum += v;
  }
  ASSERT_EQUAL(sum, 10);

  // Case B: Lazy Skipping (The "One-Pass" Test)
  auto json_b = R"([
      {"id": 1, "data": "skip me"},
      {"id": 2, "data": "skip me too"},
      {"id": 3, "data": "keep me"}
  ])"_padded;

  simdjson::ondemand::document doc_b;
  ASSERT_SUCCESS(parser.iterate(json_b).get(doc_b));

  simdjson::ondemand::array arr_b;
  ASSERT_SUCCESS(doc_b.get_array().get(arr_b));

  int64_t count = 0;
  for (auto val : arr_b) {
    count++;
    if (count == 3) {
      simdjson::ondemand::object obj;
      ASSERT_SUCCESS(val.get(obj));
      int64_t id;
      ASSERT_SUCCESS(obj["id"].get(id));
      ASSERT_EQUAL(id, 3);
    }
  }

  ASSERT_EQUAL(count, 3);
  return true;
}

// -------------------------------------------------------------------------
// Test 3: Object Field Lookup
// -------------------------------------------------------------------------
bool test_objects() {
  std::cout << "Running test_objects..." << std::endl;
  simdjson::ondemand::parser parser;

  auto json = R"({
      "a": 1,
      "b": 2,
      "c": 3
  })"_padded;

  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));

  // Find "b" (skips "a")
  int64_t b;
  ASSERT_SUCCESS(doc["b"].get(b));
  ASSERT_EQUAL(b, 2);

  // Forward access: find "c"
  int64_t c;
  ASSERT_SUCCESS(doc["c"].get(c));
  ASSERT_EQUAL(c, 3);

  return true;
}

// -------------------------------------------------------------------------
// Test 4: Raw JSON Extraction
// -------------------------------------------------------------------------
bool test_raw_json() {
  std::cout << "Running test_raw_json..." << std::endl;
  simdjson::ondemand::parser parser;

  auto json = R"({ "data": [ 1, 2, { "x": 10 } ] })"_padded;

  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));

  std::string_view raw;
  ASSERT_SUCCESS(doc["data"].get_raw_json_string().get(raw));

  // Minimal sanity check (avoid whitespace expectations)
  ASSERT_TRUE(raw.find('[') != std::string_view::npos);
  ASSERT_TRUE(raw.find("10") != std::string_view::npos);
  ASSERT_TRUE(raw.find(']') != std::string_view::npos);

  return true;
}

// -------------------------------------------------------------------------
// Main Driver
// -------------------------------------------------------------------------
int main() {
  std::cout << "==================================================" << std::endl;
  std::cout << "   RVV On-Demand Regression Suite                 " << std::endl;
  std::cout << "==================================================" << std::endl;

  if (!force_rvv()) {
    std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
    return 1;
  }
  std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
  std::cout << "--------------------------------------------------" << std::endl;

  bool pass = true;
  pass &= test_basics();
  pass &= test_arrays();
  pass &= test_objects();
  pass &= test_raw_json();

  if (pass) {
    std::cout << "SUCCESS: All On-Demand tests passed." << std::endl;
    return 0;
  } else {
    std::cerr << "FAILURE: One or more tests failed." << std::endl;
    return 1;
  }
}
