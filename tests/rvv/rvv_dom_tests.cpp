#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: DOM API Regression Suite
// -------------------------------------------------------------------------
// This suite verifies the full DOM API surface when powered by the RVV backend.
// It covers:
// 1. Parsing correctness (valid JSON).
// 2. Error handling (invalid JSON).
// 3. API usability (minified vs pretty, padding, reuse).
// -------------------------------------------------------------------------

// Helper assertions
#define ASSERT_SUCCESS(expr) \
  if ((expr) != simdjson::SUCCESS) { \
    std::cerr << "  [FAIL] Error at line " << __LINE__ << " Code: " << (expr) << std::endl; \
    return false; \
  }

#define ASSERT_NOT_SUCCESS(expr) \
  if ((expr) == simdjson::SUCCESS) { \
    std::cerr << "  [FAIL] Expected an error, got SUCCESS at line " << __LINE__ << std::endl; \
    return false; \
  }

#define ASSERT_EQUAL(actual, expected) \
  if ((actual) != (expected)) { \
    std::cerr << "  [FAIL] Expected " << (expected) << ", got " << (actual) << " at line " << __LINE__ << std::endl; \
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
// Test 1: Complex Document Navigation
// -------------------------------------------------------------------------
bool test_complex_navigation() {
  std::cout << "Running test_complex_navigation..." << std::endl;
  simdjson::dom::parser parser;

  auto json = R"({
      "meta": { "count": 2, "verified": true },
      "users": [
          { "id": 1, "name": "Alice", "tags": ["admin", "staff"] },
          { "id": 2, "name": "Bob", "tags": [] }
      ]
  })"_padded;

  simdjson::dom::element doc;
  ASSERT_SUCCESS(parser.parse(json).get(doc));

  // 1. Deep Access
  std::string_view name;
  ASSERT_SUCCESS(doc["users"].at(0)["name"].get(name));
  ASSERT_EQUAL(name, "Alice");

  // 2. Array iteration
  simdjson::dom::array tags;
  ASSERT_SUCCESS(doc["users"].at(0)["tags"].get(tags));
  ASSERT_EQUAL(tags.size(), 2);

  // 3. Boolean check
  bool verified;
  ASSERT_SUCCESS(doc["meta"]["verified"].get(verified));
  ASSERT_EQUAL(verified, true);

  return true;
}

// -------------------------------------------------------------------------
// Test 2: Error Handling (Invalid JSON)
// -------------------------------------------------------------------------
bool test_errors() {
  std::cout << "Running test_errors..." << std::endl;
  simdjson::dom::parser parser;

  // 1. Unclosed Brace (exact error code may vary across versions/impls)
  auto err1 = parser.parse(R"({"key": 1)"_padded).error();
  ASSERT_NOT_SUCCESS(err1);

  // 2. Trailing Comma (Standard JSON disallows this)
  auto err2 = parser.parse(R"({"a": 1,})"_padded).error();
  ASSERT_NOT_SUCCESS(err2);

  // 3. Unquoted Keys (Invalid JSON)
  auto err3 = parser.parse(R"({key: 1})"_padded).error();
  ASSERT_NOT_SUCCESS(err3);

  return true;
}

// -------------------------------------------------------------------------
// Test 3: Parser Reuse
// -------------------------------------------------------------------------
bool test_reuse() {
  std::cout << "Running test_reuse..." << std::endl;
  simdjson::dom::parser parser;
  simdjson::dom::element doc;

  // Pass 1
  ASSERT_SUCCESS(parser.parse(R"({"a": 1})"_padded).get(doc));
  int64_t a;
  ASSERT_SUCCESS(doc["a"].get(a));
  ASSERT_EQUAL(a, 1);

  // Pass 2 (Same parser, different content)
  ASSERT_SUCCESS(parser.parse(R"({"b": 2})"_padded).get(doc));
  int64_t b;
  ASSERT_SUCCESS(doc["b"].get(b));
  ASSERT_EQUAL(b, 2);

  return true;
}

// -------------------------------------------------------------------------
// Test 4: Minified vs Pretty
// -------------------------------------------------------------------------
bool test_formats() {
  std::cout << "Running test_formats..." << std::endl;
  simdjson::dom::parser parser;
  simdjson::dom::element doc;

  // Minified
  std::string min = R"({"k":1,"a":[1,2]})";
  ASSERT_SUCCESS(parser.parse(simdjson::padded_string(min)).get(doc));
  ASSERT_EQUAL(int64_t(doc["k"]), 1);

  // Pretty (Lots of whitespace)
  std::string pretty = R"({
      "k": 1,
      "a": [
          1,
          2
      ]
  })";
  ASSERT_SUCCESS(parser.parse(simdjson::padded_string(pretty)).get(doc));
  ASSERT_EQUAL(int64_t(doc["k"]), 1);

  return true;
}

// -------------------------------------------------------------------------
// Main Driver
// -------------------------------------------------------------------------
int main() {
  std::cout << "==================================================" << std::endl;
  std::cout << "   RVV DOM API Regression Suite                   " << std::endl;
  std::cout << "==================================================" << std::endl;

  if (!force_rvv()) {
    std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
    return 1;
  }
  std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
  std::cout << "--------------------------------------------------" << std::endl;

  bool pass = true;
  pass &= test_complex_navigation();
  pass &= test_errors();
  pass &= test_reuse();
  pass &= test_formats();

  if (pass) {
    std::cout << "SUCCESS: All DOM API tests passed." << std::endl;
    return 0;
  } else {
    std::cerr << "FAILURE: One or more tests failed." << std::endl;
    return 1;
  }
}
