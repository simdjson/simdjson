#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: Minify (Whitespace Removal) Regression Suite
// -------------------------------------------------------------------------
// This suite verifies the correctness of the RVV-optimized minify kernel.
//
// Key Verification Points:
// 1. Correctness: Only removing 0x20, 0x09, 0x0A, 0x0D.
// 2. Tail Safety: Validating inputs of length 0..130 to catch off-by-one
//    errors in the VL (Vector Length) calculation.
// 3. Data Integrity: Ensuring no non-whitespace characters are dropped.
// -------------------------------------------------------------------------

// Helper to select RVV (safe pattern)
bool force_rvv() {
  auto rvv = simdjson::get_available_implementations()["rvv"];
  if (!rvv) { return false; }
  if (!rvv->supported_by_runtime_system()) { return false; }
  simdjson::get_active_implementation() = rvv;
  return true;
}

// -------------------------------------------------------------------------
// Test Helper
// -------------------------------------------------------------------------
bool check_minify(const std::string& input, const std::string& expected) {
  // Contract: dst must be at least len + SIMDJSON_PADDING bytes.
  std::vector<uint8_t> dst(input.size() + SIMDJSON_PADDING);
  size_t dst_len = 0;

  auto err = simdjson::minify(
      reinterpret_cast<const uint8_t*>(input.data()),
      input.size(),
      dst.data(),
      dst_len
  );

  if (err) {
    std::cerr << "  [FAIL] API Error: " << err << std::endl;
    return false;
  }

  std::string actual(reinterpret_cast<const char*>(dst.data()), dst_len);
  if (actual != expected) {
    std::cerr << "  [FAIL] Mismatch!\n"
              << "    Input (len=" << input.size() << "): [" << input << "]\n"
              << "    Expected: [" << expected << "]\n"
              << "    Got:      [" << actual << "]" << std::endl;
    return false;
  }
  return true;
}

// -------------------------------------------------------------------------
// Test 1: Basic Correctness
// -------------------------------------------------------------------------
bool test_basics() {
  std::cout << "Running test_basics..." << std::endl;
  bool ok = true;

  ok &= check_minify("", "");
  ok &= check_minify("   ", "");
  ok &= check_minify("abc", "abc");
  ok &= check_minify(" { } ", "{}");
  ok &= check_minify(
      " { \"key\" : 1 , \"list\" : [ 1 , 2 ] } ",
      "{\"key\":1,\"list\":[1,2]}"
  );

  return ok;
}

// -------------------------------------------------------------------------
// Test 2: Whitespace Permutations
// -------------------------------------------------------------------------
bool test_whitespace_chars() {
  std::cout << "Running test_whitespace_chars..." << std::endl;
  bool ok = true;

  // JSON defines whitespace as: Space (0x20), Tab (0x09), LF (0x0A), CR (0x0D)
  std::string all_ws = "\x20\x09\x0A\x0D";
  ok &= check_minify(all_ws, "");

  // Interleaved
  ok &= check_minify("A\x20" "B\x09" "C\x0A" "D\x0D" "E", "ABCDE");

  // Edge case: Non-whitespace control characters (should stay)
  std::string ctrls = "A\x01\x02\x08Z";
  ok &= check_minify(ctrls, ctrls);

  return ok;
}

// -------------------------------------------------------------------------
// Test 3: Boundary Sweep (The "Off-By-One" Killer)
// -------------------------------------------------------------------------
bool test_boundaries() {
  std::cout << "Running test_boundaries (1 to 130 bytes)..." << std::endl;
  bool ok = true;

  for (size_t len = 1; len <= 130; ++len) {
    // Case A: All spaces
    std::string all_spaces(len, ' ');
    if (!check_minify(all_spaces, "")) {
      std::cerr << "    Failed at length " << len << " (All Spaces)" << std::endl;
      ok = false;
    }

    // Case B: Single char at end
    std::string single_char = all_spaces;
    single_char[len - 1] = 'X';
    if (!check_minify(single_char, "X")) {
      std::cerr << "    Failed at length " << len << " (Char at End)" << std::endl;
      ok = false;
    }

    // Case C: Alternating
    std::string alt;
    std::string expect;
    alt.reserve(len);
    expect.reserve(len);

    for (size_t i = 0; i < len; ++i) {
      if ((i % 2) == 0) {
        alt += 'x';
        expect += 'x';
      } else {
        alt += ' ';
      }
    }

    if (!check_minify(alt, expect)) {
      std::cerr << "    Failed at length " << len << " (Alternating)" << std::endl;
      ok = false;
    }
  }

  return ok;
}

// -------------------------------------------------------------------------
// Main Driver
// -------------------------------------------------------------------------
int main() {
  std::cout << "==================================================" << std::endl;
  std::cout << "   RVV Minify Regression Suite                    " << std::endl;
  std::cout << "==================================================" << std::endl;

  if (!force_rvv()) {
    std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
    return 1;
  }

  std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
  std::cout << "--------------------------------------------------" << std::endl;

  bool all_passed = true;
  all_passed &= test_basics();
  all_passed &= test_whitespace_chars();
  all_passed &= test_boundaries();

  if (all_passed) {
    std::cout << "SUCCESS: All Minify tests passed." << std::endl;
    return 0;
  } else {
    std::cerr << "FAILURE: One or more tests failed." << std::endl;
    return 1;
  }
}
