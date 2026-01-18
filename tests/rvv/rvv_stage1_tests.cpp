#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: Stage 1 (Structural Indexing) Stress Tests
// -------------------------------------------------------------------------
// This suite targets the "simd8x64" logic and the scalar fallback loops
// defined in 'stringparsing_defs.h'.
//
// Key Failure Modes to cover:
// 1. Missing a structural char at a 64-byte block boundary.
// 2. Incorrectly classifying a structural char inside a quote as valid.
// 3. Miscounting backslashes (odd vs even) at block boundaries.
// -------------------------------------------------------------------------

// Helper to select RVV
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
// Test Group 1: Complex Escapes
// -------------------------------------------------------------------------
bool test_complex_escapes() {
    std::cout << "--- Testing Escape Sequences ---" << std::endl;
    simdjson::dom::parser parser;
    bool ok = true;

    // Pairs of {JSON, ExpectedValidity}
    // These test the odd/even backslash counting logic (scalar bit manipulation).
    struct Case { std::string name; std::string json; bool valid; };
    std::vector<Case> cases = {
        {"Simple Quote", R"({"key": "val\"ue"})", true},
        {"Double Backslash", R"({"key": "val\\"})", true}, // Ends in double slash (escaped slash) -> valid
        {"Triple Backslash", R"({"key": "val\\\""})", true}, // \\ + \" -> valid
        {"Quad Backslash", R"({"key": "val\\\\"})", true}, // \\ + \\ -> valid

        // Invalid cases
        {"Unescaped Quote", R"({"key": "val"ue"})", false}, // "val" ue" -> error
        {"Trailing Single Slash", R"({"key": "val\"})", false}, // Escapes the closing quote -> error
        {"Trailing Triple Slash", R"({"key": "val\\\"})", false}, // \\ + \" (escapes closer) -> error
    };

    for (const auto& c : cases) {
        auto err = parser.parse(c.json).error();
        bool is_valid = (err == simdjson::SUCCESS);

        if (is_valid != c.valid) {
            std::cerr << "[FAIL] " << c.name << "\n"
                      << "       Input: " << c.json << "\n"
                      << "       Expected: " << (c.valid?"Valid":"Invalid")
                      << ", Got: " << (is_valid?"Valid":"Invalid") << std::endl;
            ok = false;
        }
    }
    if(ok) std::cout << "[PASS] Escape Logic" << std::endl;
    return ok;
}

// -------------------------------------------------------------------------
// Test Group 2: Boundary Alignment (The "Block Splitter")
// -------------------------------------------------------------------------
bool test_boundaries() {
    std::cout << "--- Testing Block Boundaries (64B / VLEN) ---" << std::endl;
    simdjson::dom::parser parser;
    bool ok = true;

    // We use spaces to pad critical characters to specific offsets.
    // Spec defines fixed block size = 64 bytes.
    // Critical offsets: 63 (end of block 0), 64 (start of block 1).

    std::vector<size_t> offsets = {
        62, 63, 64, 65,         // Block 0/1 boundary
        126, 127, 128, 129,     // Block 1/2 boundary (VLEN=128 boundary)
        254, 255, 256, 257      // Block 3/4 boundary (VLEN=256 boundary)
    };

    for (size_t pad_len : offsets) {
        std::string padding(pad_len, ' ');

        // Scenario A: Structural character at boundary
        // {"a": 1} ...
        // We place the colon ':' or comma ',' exactly at the boundary.
        std::string json_a = R"({"key")" + padding + R"(: "value"})";
        if (parser.parse(json_a).error()) {
            std::cerr << "[FAIL] Structural char (:) at offset " << (6 + pad_len) << " failed." << std::endl;
            ok = false;
        }

        // Scenario B: Quote starting at boundary
        std::string json_b = R"({"key": )" + padding + R"("value"})";
        if (parser.parse(json_b).error()) {
            std::cerr << "[FAIL] Quote start at offset " << (8 + pad_len) << " failed." << std::endl;
            ok = false;
        }

        // Scenario C: Escaped quote crossing boundary
        // We want the sequence \" to straddle the boundary if possible,
        // or sit right on it.
        // Padded so \" lands near the cut.
        std::string json_c = R"({"k": ")" + padding + R"(\"val"})";
        if (parser.parse(json_c).error()) {
            std::cerr << "[FAIL] Escaped quote with padding " << pad_len << " failed." << std::endl;
            ok = false;
        }
    }

    if(ok) std::cout << "[PASS] Boundary Alignment" << std::endl;
    return ok;
}

// -------------------------------------------------------------------------
// Test Group 3: Structural Confusion (Quotes vs Structure)
// -------------------------------------------------------------------------
bool test_confusion() {
    std::cout << "--- Testing Structural Confusion ---" << std::endl;
    simdjson::dom::parser parser;
    bool ok = true;

    // If the bitmasks are wrong, these will be detected as structural chars,
    // causing parse errors.
    std::string json = R"({
        "fake_structure": "{ [ : , ] }",
        "nested": {
            "more_fake": "{\"a\":1}"
        }
    })";

    if (parser.parse(json).error()) {
        std::cerr << "[FAIL] Parser confused by structural chars inside strings." << std::endl;
        ok = false;
    }

    // Heavy nesting to stress the depth counter and stack logic
    std::string deep = R"({"a": [[[[[[[[[[[[[[[[[[[[[[[[[[[[1]]]]]]]]]]]]]]]]]]]]]]]]]]]]})";
    if (parser.parse(deep).error()) {
        std::cerr << "[FAIL] Deeply nested array failed." << std::endl;
        ok = false;
    }

    if(ok) std::cout << "[PASS] Structural Confusion" << std::endl;
    return ok;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Stage 1 Correctness Test Suite             " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool all_passed = true;
    all_passed &= test_complex_escapes();
    all_passed &= test_boundaries();
    all_passed &= test_confusion();

    if (all_passed) {
        std::cout << "SUCCESS: All Stage 1 tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more tests failed." << std::endl;
        return 1;
    }
}
