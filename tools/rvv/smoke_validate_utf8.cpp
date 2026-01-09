#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV UTF-8 Smoketest
// -------------------------------------------------------------------------
// This tool manually forces the "rvv" backend to be active and runs
// targeted correctness tests. It is useful for debugging QEMU crashes
// or logic errors without running the full CTest suite.
// -------------------------------------------------------------------------

int main() {
  std::cout << "==================================================" << std::endl;
  std::cout << "   RVV Backend Smoketest: UTF-8 Validation        " << std::endl;
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
    std::cerr << "[CRITICAL] 'rvv' backend not found in available implementations!" << std::endl;
    std::cerr << "Ensure you compiled with -DSIMDJSON_ENABLE_RVV=ON and linked correctly." << std::endl;
    return 1;
  }

  const auto* active_impl = simdjson::get_active_implementation();
  std::cout << "Active Backend: " << active_impl->name() << std::endl;
  std::cout << "Description:    " << active_impl->description() << std::endl;
  std::cout << "--------------------------------------------------" << std::endl;

  // 2. Test Cases
  // -----------------------------------------------------------------------
  struct TestCase {
    std::string name;
    std::string data;
    bool expected;
  };

  std::vector<TestCase> cases = {
    // Basic
    {"Empty", "", true},
    {"ASCII Word", "simdjson", true},
    {"ASCII Sentence", "The quick brown fox jumps over the lazy dog.", true},

    // Valid Multibyte
    {"2-byte (Latin)", "Gr\xC3\xBC\xC3\x9F Gott", true}, // Grüß Gott
    {"3-byte (Euro)", "\xE2\x82\xAC", true}, // €
    {"4-byte (Emoji)", "\xF0\x9F\x98\x80", true}, // 😀

    // Boundaries (Simulating 64-byte block crossings)
    {"Block Boundary (63 chars)", std::string(63, 'a'), true},
    {"Block Boundary (64 chars)", std::string(64, 'a'), true},
    {"Block Boundary (65 chars)", std::string(65, 'a'), true},

    // Invalid sequences
    {"Truncated 2-byte", "\xC3", false},
    {"Truncated 3-byte", "\xE2\x82", false},
    {"Truncated 4-byte", "\xF0\x9F\x98", false},
    {"Invalid Header (Continuation)", "\x80", false},
    {"Overlong / (2-byte)", "\xC0\xAF", false},
    {"Surrogate Pair (Invalid in UTF-8)", "\xED\xA0\x80", false},
    {"Impossible Byte (> 0xF4)", "\xFF", false},
  };

  // 3. Execution Loop
  // -----------------------------------------------------------------------
  int failures = 0;
  for (const auto& test : cases) {
    // We call the generic public API, which delegates to the active backend (rvv).
    bool actual = simdjson::validate_utf8(test.data.data(), test.data.size());

    if (actual != test.expected) {
      std::cout << "[FAIL] " << test.name << "\n"
                << "       Expected: " << (test.expected ? "true" : "false") << "\n"
                << "       Got:      " << (actual ? "true" : "false") << std::endl;
      failures++;
    } else {
      std::cout << "[PASS] " << test.name << std::endl;
    }
  }

  // 4. Large Input Test (Stress Tail Handling)
  // -----------------------------------------------------------------------
  // 1024 bytes + 1 (odd size) to ensure we hit the loop + tail logic
  std::cout << "--------------------------------------------------" << std::endl;
  std::string large_input(1025, 'x');
  // Inject an error at the very end
  large_input[1024] = static_cast<char>(0xFF);

  if (simdjson::validate_utf8(large_input.data(), large_input.size()) == false) {
      std::cout << "[PASS] Large Buffer Tail Error Detection" << std::endl;
  } else {
      std::cout << "[FAIL] Large Buffer Tail Error Detection (False Positive)" << std::endl;
      failures++;
  }

  std::cout << "--------------------------------------------------" << std::endl;
  if (failures == 0) {
    std::cout << "SUCCESS: All smoke tests passed." << std::endl;
    return 0;
  } else {
    std::cerr << "FAILURE: " << failures << " tests failed." << std::endl;
    return 1;
  }
}
