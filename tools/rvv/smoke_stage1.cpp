#include <iostream>
#include <string>
#include <vector>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Smoketest: Stage 1 (Structural Indexing)
// -------------------------------------------------------------------------
// This tool validates the "Stage 1" phase of the parser, which is responsible
// for identifying structural characters ({}[]:,) and handling quotes/escapes.
// It forces the RVV backend and runs targeted structural tests.
// -------------------------------------------------------------------------

// Helper to print visible representation of invisible chars
std::string visualize(const std::string& s) {
  std::string out;
  for (char c : s) {
    if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

int main() {
  std::cout << "==================================================" << std::endl;
  std::cout << "   RVV Backend Smoketest: Stage 1 (Structure)     " << std::endl;
  std::cout << "==================================================" << std::endl;

  // 1. Force Selection of RVV Backend
  // -----------------------------------------------------------------------
  bool rvv_found = false;
  for (auto implementation : simdjson::get_available_implementations()) {
    if (implementation->name() == "rvv") {
      simdjson::get_active_implementation() = implementation;
      rvv_found = true;
      break;
    }
  }

  if (!rvv_found) {
    std::cerr << "[CRITICAL] 'rvv' backend not found!" << std::endl;
    return 1;
  }

  std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
  std::cout << "--------------------------------------------------" << std::endl;

  // 2. Test Cases
  // -----------------------------------------------------------------------
  struct TestCase {
    std::string name;
    std::string json;
    bool should_parse; // true = expect SUCCESS, false = expect ERROR
  };

  std::vector<TestCase> cases = {
    // Basic Structure
    {"Simple Object", R"({"key": "value"})", true},
    {"Simple Array", R"([1, 2, 3])", true},
    {"Nested", R"({"a": [1, {"b": 2}]})", true},

    // Quotes and Escapes (The hard part of Stage 1)
    {"Escaped Quote", R"({"key": "val\"ue"})", true},
    {"Backslash at End", R"({"key": "val\\"})", true}, // ends with double backslash (valid)
    {"Escaped Backslash", R"({"key": "val\\ue"})", true},

    // Structural characters inside quotes (Should be ignored)
    {"Braces in String", R"({"key": "{}"})", true},
    {"Colons in String", R"({"key": "a:b"})", true},
    {"Commas in String", R"({"key": "a,b"})", true},

    // Edge Cases: Block Boundaries (64 bytes)
    // We pad with spaces to push significant chars across boundaries.
    {"Boundary: Quote Split", std::string(60, ' ') + R"({"a": "b"})", true},
    {"Boundary: Structural Split", std::string(63, ' ') + R"([1,2])", true},

    // Invalid JSON (Should fail)
    {"Unclosed Quote", R"({"key": "value)", false},
    {"Unescaped Quote", R"({"key": "val"ue"})", false},
    {"Missing Colon", R"({"key" "value"})", false},
    {"Trailing Comma", R"([1, 2,])", false}, // Standard JSON disallows this
    {"Garbage after valid", R"({"a":1} garbage)", false},
  };

  // 3. Execution Loop
  // -----------------------------------------------------------------------
  simdjson::dom::parser parser;
  int failures = 0;

  for (const auto& test : cases) {
    auto result = parser.parse(test.json);
    bool success = !result.error();

    if (success != test.should_parse) {
      std::cout << "[FAIL] " << test.name << "\n"
                << "       Input:    " << visualize(test.json.substr(0, 50)) << (test.json.size() > 50 ? "..." : "") << "\n"
                << "       Expected: " << (test.should_parse ? "SUCCESS" : "ERROR") << "\n"
                << "       Got:      " << (success ? "SUCCESS" : "ERROR") << "\n"
                << "       Error Code: " << result.error() << std::endl;
      failures++;
    } else {
      std::cout << "[PASS] " << test.name << std::endl;
    }
  }

  // 4. Large Depth Stress Test
  // -----------------------------------------------------------------------
  // Tests if the structural indexer correctly handles deeply nested structures
  // without overflowing internal buffers or stacks.
  std::cout << "--------------------------------------------------" << std::endl;
  std::string deep_json;
  int depth = 500;
  for(int i=0; i<depth; i++) deep_json += "{\"a\":";
  deep_json += "1";
  for(int i=0; i<depth; i++) deep_json += "}";

  if (!parser.parse(deep_json).error()) {
     std::cout << "[PASS] Deeply Nested JSON (" << depth << " levels)" << std::endl;
  } else {
     std::cout << "[FAIL] Deeply Nested JSON Failed" << std::endl;
     failures++;
  }

  std::cout << "--------------------------------------------------" << std::endl;
  if (failures == 0) {
    std::cout << "SUCCESS: All Stage 1 smoke tests passed." << std::endl;
    return 0;
  } else {
    std::cerr << "FAILURE: " << failures << " tests failed." << std::endl;
    return 1;
  }
}
