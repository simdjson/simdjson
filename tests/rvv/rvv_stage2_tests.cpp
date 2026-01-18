#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: Stage 2 (DOM) Regression Suite
// -------------------------------------------------------------------------
// This suite verifies the "Tape Building" phase of the parser.
// Even though Stage 2 is currently generic (scalar), specific backend
// configurations (like padding or index formats) can subtly break it.
// -------------------------------------------------------------------------

// Helper macro for assertions
#define ASSERT_TRUE(cond) \
    if (!(cond)) { \
        std::cerr << "  [FAIL] " << #cond << " at line " << __LINE__ << std::endl; \
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
// Test 1: Primitives (The Atomic Units)
// -------------------------------------------------------------------------
bool test_primitives() {
    std::cout << "Running test_primitives..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;

    // 1. Integers
    ASSERT_TRUE(parser.parse("0"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_EQUAL(int64_t(doc), 0);

    ASSERT_TRUE(parser.parse("-12345"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_EQUAL(int64_t(doc), -12345);

    // 2. Floats
    ASSERT_TRUE(parser.parse("3.14159"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(std::abs(double(doc) - 3.14159) < 1e-6);

    // 3. Booleans & Null
    ASSERT_TRUE(parser.parse("true"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(bool(doc));

    ASSERT_TRUE(parser.parse("false"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(!bool(doc));

    ASSERT_TRUE(parser.parse("null"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(doc.is_null());

    return true;
}

// -------------------------------------------------------------------------
// Test 2: Strings & Escapes (Tape Text)
// -------------------------------------------------------------------------
bool test_strings() {
    std::cout << "Running test_strings..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    std::string_view sv;

    // Simple
    ASSERT_TRUE(parser.parse(R"("hello")"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(doc.get(sv) == simdjson::SUCCESS);
    ASSERT_TRUE(sv == "hello");

    // Escapes (Requires Stage 2 to unescape correctly)
    // Input: "line\nfeed"
    ASSERT_TRUE(parser.parse(R"("line\nfeed")"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(doc.get(sv) == simdjson::SUCCESS);
    ASSERT_TRUE(sv == "line\nfeed");

    // Unicode (Requires UTF-8 validation in Stage 1 + pass-through)
    // Input: "Björk" (UTF-8 bytes: 42 6A C3 B6 72 6B)
    ASSERT_TRUE(parser.parse(R"("Björk")"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(doc.get(sv) == simdjson::SUCCESS);
    ASSERT_TRUE(sv == "Björk");

    return true;
}

// -------------------------------------------------------------------------
// Test 3: Structure & Nesting (Tape Navigation)
// -------------------------------------------------------------------------
bool test_structure() {
    std::cout << "Running test_structure..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;

    // Array of objects
    auto json = R"([
        { "id": 1, "val": "a" },
        { "id": 2, "val": "b" }
    ])"_padded;

    ASSERT_TRUE(parser.parse(json).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(doc.is_array());

    simdjson::dom::array arr;
    ASSERT_TRUE(doc.get(arr) == simdjson::SUCCESS);
    ASSERT_EQUAL(arr.size(), 2);

    // Access elements
    int64_t id;
    std::string_view val;

    // Element 0
    simdjson::dom::element e0 = arr.at(0).value();
    ASSERT_TRUE(e0["id"].get(id) == simdjson::SUCCESS);
    ASSERT_EQUAL(id, 1);

    // Element 1
    simdjson::dom::element e1 = arr.at(1).value();
    ASSERT_TRUE(e1["val"].get(val) == simdjson::SUCCESS);
    ASSERT_TRUE(val == "b");

    return true;
}

// -------------------------------------------------------------------------
// Test 4: Corner Cases
// -------------------------------------------------------------------------
bool test_corners() {
    std::cout << "Running test_corners..." << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;

    // Empty Object
    ASSERT_TRUE(parser.parse(R"({})"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(doc.is_object());
    ASSERT_EQUAL(doc.get_object().value().size(), 0);

    // Empty Array
    ASSERT_TRUE(parser.parse(R"([])"_padded).get(doc) == simdjson::SUCCESS);
    ASSERT_TRUE(doc.is_array());
    ASSERT_EQUAL(doc.get_array().value().size(), 0);

    // Deep Nesting (Check stack depth limits)
    std::string deep = R"({"a":[[[[[[[[[1]]]]]]]]]})";
    ASSERT_TRUE(parser.parse(simdjson::padded_string(deep)).get(doc) == simdjson::SUCCESS);

    return true;
}

// -------------------------------------------------------------------------
// Main Driver
// -------------------------------------------------------------------------
int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Stage 2 Regression Tests                   " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool pass = true;
    pass &= test_primitives();
    pass &= test_strings();
    pass &= test_structure();
    pass &= test_corners();

    if (pass) {
        std::cout << "SUCCESS: All Stage 2 tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more tests failed." << std::endl;
        return 1;
    }
}
