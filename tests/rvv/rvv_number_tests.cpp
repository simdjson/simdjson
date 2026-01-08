#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: Number Parsing Regression Suite
// -------------------------------------------------------------------------
// This suite verifies the correctness of numerical parsing within the DOM.
//
// Key Verification Points:
// 1. Integer Precision: Full 64-bit signed/unsigned range coverage.
// 2. Float Precision: Correct handling of mantissa/exponent.
// 3. Scientific Notation: e/E handling, +/- exponents.
// 4. Negative Zero: -0.0 handling.
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
    for (auto implementation : simdjson::get_available_implementations()) {
        if (implementation->name() == "rvv") {
            simdjson::get_active_implementation() = implementation;
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------------------------
// Test 1: Signed Integers (int64_t)
// -------------------------------------------------------------------------
bool test_integers() {
    std::cout << "Running test_integers..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;

    // 0
    ASSERT_SUCCESS(parser.parse("0"_padded).get(doc));
    ASSERT_EQUAL(int64_t(doc), 0);

    // -1
    ASSERT_SUCCESS(parser.parse("-1"_padded).get(doc));
    ASSERT_EQUAL(int64_t(doc), -1);

    // Max Int64
    std::string max_s = std::to_string(std::numeric_limits<int64_t>::max());
    ASSERT_SUCCESS(parser.parse(simdjson::padded_string(max_s)).get(doc));
    ASSERT_EQUAL(int64_t(doc), std::numeric_limits<int64_t>::max());

    // Min Int64
    std::string min_s = std::to_string(std::numeric_limits<int64_t>::min());
    ASSERT_SUCCESS(parser.parse(simdjson::padded_string(min_s)).get(doc));
    ASSERT_EQUAL(int64_t(doc), std::numeric_limits<int64_t>::min());

    return true;
}

// -------------------------------------------------------------------------
// Test 2: Unsigned Integers (uint64_t)
// -------------------------------------------------------------------------
bool test_unsigned() {
    std::cout << "Running test_unsigned..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;

    // > Int64 Max
    uint64_t val = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 100;
    std::string val_s = std::to_string(val);

    ASSERT_SUCCESS(parser.parse(simdjson::padded_string(val_s)).get(doc));
    uint64_t parsed;
    ASSERT_SUCCESS(doc.get(parsed));
    ASSERT_EQUAL(parsed, val);

    // Max Uint64
    std::string max_u = std::to_string(std::numeric_limits<uint64_t>::max());
    ASSERT_SUCCESS(parser.parse(simdjson::padded_string(max_u)).get(doc));
    ASSERT_SUCCESS(doc.get(parsed));
    ASSERT_EQUAL(parsed, std::numeric_limits<uint64_t>::max());

    return true;
}

// -------------------------------------------------------------------------
// Test 3: Floating Point
// -------------------------------------------------------------------------
bool test_floats() {
    std::cout << "Running test_floats..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    double val;

    // Basic
    ASSERT_SUCCESS(parser.parse("1.2345"_padded).get(doc));
    ASSERT_SUCCESS(doc.get(val));
    ASSERT_TRUE(std::abs(val - 1.2345) < 1e-9);

    // Scientific (+)
    ASSERT_SUCCESS(parser.parse("1.5e3"_padded).get(doc)); // 1500.0
    ASSERT_SUCCESS(doc.get(val));
    ASSERT_EQUAL(val, 1500.0);

    // Scientific (-)
    ASSERT_SUCCESS(parser.parse("1.5e-2"_padded).get(doc)); // 0.015
    ASSERT_SUCCESS(doc.get(val));
    ASSERT_TRUE(std::abs(val - 0.015) < 1e-9);

    // Negative Zero
    ASSERT_SUCCESS(parser.parse("-0.0"_padded).get(doc));
    ASSERT_SUCCESS(doc.get(val));
    ASSERT_EQUAL(val, 0.0);
    ASSERT_TRUE(std::signbit(val)); // Ensure sign bit is preserved

    return true;
}

// -------------------------------------------------------------------------
// Test 4: Invalid Numbers
// -------------------------------------------------------------------------
bool test_invalid() {
    std::cout << "Running test_invalid..." << std::endl;
    simdjson::dom::parser parser;

    // Leading zeros (forbidden in JSON except for "0")
    if (parser.parse("01"_padded).error() == simdjson::SUCCESS) {
        std::cerr << "  [FAIL] Leading zero should be invalid" << std::endl;
        return false;
    }

    // Trailing decimal
    if (parser.parse("1."_padded).error() == simdjson::SUCCESS) {
        std::cerr << "  [FAIL] Trailing decimal should be invalid" << std::endl;
        return false;
    }

    // Missing integer part
    if (parser.parse(".123"_padded).error() == simdjson::SUCCESS) {
        std::cerr << "  [FAIL] Missing integer part should be invalid" << std::endl;
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------
// Main Driver
// -------------------------------------------------------------------------
int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Number Parsing Regression Suite            " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool pass = true;
    pass &= test_integers();
    pass &= test_unsigned();
    pass &= test_floats();
    pass &= test_invalid();

    if (pass) {
        std::cout << "SUCCESS: All Number Parsing tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more tests failed." << std::endl;
        return 1;
    }
}
