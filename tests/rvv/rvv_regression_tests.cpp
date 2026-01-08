#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: Regression Test Suite
// -------------------------------------------------------------------------
// This suite accumulates specific test cases for bugs discovered during 
// development. It focuses on "tricky" inputs that stress specific 
// interaction paths in the RVV kernels.
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
// Regression Case 1: Exact Block Alignment (The "Fencepost" Bug)
// -------------------------------------------------------------------------
// Issue: Logic that iterates in 64-byte chunks may read past the buffer
// or miss the final character if the input length is a perfect multiple.
bool test_block_alignment() {
    std::cout << "Running test_block_alignment..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;

    // Create a JSON string exactly 64 bytes long
    // {"key": ".................................................."}
    // 8 chars overhead + 56 dots = 64 chars
    std::string padding(56, '.');
    std::string json_64 = R"({"key": ")" + padding + R"("})";
    
    if (json_64.size() != 64) {
        std::cerr << "  [SETUP ERROR] String is " << json_64.size() << " bytes, expected 64." << std::endl;
        return false;
    }

    ASSERT_SUCCESS(parser.parse(simdjson::padded_string(json_64)).get(doc));
    
    std::string_view val;
    ASSERT_SUCCESS(doc["key"].get(val));
    ASSERT_EQUAL(val.size(), 56);

    return true;
}

// -------------------------------------------------------------------------
// Regression Case 2: Structural Character at End of Block
// -------------------------------------------------------------------------
// Issue: If a closing brace '}' or quote '"' sits at index 63 (byte 64),
// the carry-over logic for the next iteration must be correct.
bool test_boundary_structurals() {
    std::cout << "Running test_boundary_structurals..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;

    // We want '}' to be at index 63.
    // {"a":1} -> length 7. Pad with spaces.
    // 56 spaces + {"a":1} = 63 chars (ends at index 62) -> mismatch.
    // 57 spaces + {"a":1} = 64 chars. '}' is at 63.
    std::string padding(57, ' ');
    std::string json = padding + R"({"a":1})";

    ASSERT_SUCCESS(parser.parse(simdjson::padded_string(json)).get(doc));
    int64_t a;
    ASSERT_SUCCESS(doc["a"].get(a));
    ASSERT_EQUAL(a, 1);

    return true;
}

// -------------------------------------------------------------------------
// Regression Case 3: Empty/Null Inputs
// -------------------------------------------------------------------------
// Issue: Vector loops with AVL=0 or null pointers must not crash.
bool test_empty_inputs() {
    std::cout << "Running test_empty_inputs..." << std::endl;
    simdjson::dom::parser parser;
    
    // Empty string (Valid padded_string, but invalid JSON)
    auto err = parser.parse("").error();
    if (err != simdjson::EMPTY) {
        // Depending on version, might be EMPTY or some other error, 
        // but definitely not SUCCESS and definitely not a Segfault.
        if (err == simdjson::SUCCESS) {
             std::cerr << "  [FAIL] Empty string should not parse successfully." << std::endl;
             return false;
        }
    }

    return true;
}

// -------------------------------------------------------------------------
// Regression Case 4: Deeply Nested Arrays (Stack Overflow Check)
// -------------------------------------------------------------------------
// Issue: Recursive logic or limited stack space in Stage 1.
bool test_deep_nesting() {
    std::cout << "Running test_deep_nesting..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;

    // 50 levels of nesting
    std::string open(50, '[');
    std::string close(50, ']');
    std::string json = open + "1" + close;
    
    ASSERT_SUCCESS(parser.parse(simdjson::padded_string(json)).get(doc));
    
    // Verify we can drill down
    simdjson::dom::element curr = doc;
    for(int i=0; i<50; ++i) {
        simdjson::dom::array arr;
        ASSERT_SUCCESS(curr.get(arr));
        ASSERT_EQUAL(arr.size(), 1);
        curr = arr.at(0);
    }
    int64_t val;
    ASSERT_SUCCESS(curr.get(val));
    ASSERT_EQUAL(val, 1);

    return true;
}

// -------------------------------------------------------------------------
// Main Driver
// -------------------------------------------------------------------------
int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Regression Test Suite                      " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool pass = true;
    pass &= test_block_alignment();
    pass &= test_boundary_structurals();
    pass &= test_empty_inputs();
    pass &= test_deep_nesting();

    if (pass) {
        std::cout << "SUCCESS: All regression tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more tests failed." << std::endl;
        return 1;
    }
}