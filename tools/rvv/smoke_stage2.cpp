#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Smoketest: Stage 2 (DOM Construction)
// -------------------------------------------------------------------------
// This tool validates the full parsing pipeline (Stage 1 + Stage 2) using
// the RVV backend.
//
// Focus Areas:
// 1. Factory Wiring: confirming 'create_dom_parser_implementation' works.
// 2. Integration: ensuring Stage 1 structural indexes feed correctly into
//    the generic Stage 2 tape builder.
// 3. Basic Types: Strings, Ints, Doubles, Bools, Nulls, Arrays, Objects.
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
// Test Cases
// -------------------------------------------------------------------------

bool test_basic_types() {
    std::cout << "--- Testing Basic Types ---" << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    bool ok = true;

    // 1. Integer
    if (parser.parse("12345"_padded).get(doc) != simdjson::SUCCESS || 
        int64_t(doc) != 12345) {
        std::cerr << "[FAIL] Integer parsing" << std::endl;
        ok = false;
    }

    // 2. Double
    if (parser.parse("123.45"_padded).get(doc) != simdjson::SUCCESS || 
        double(doc) != 123.45) {
        std::cerr << "[FAIL] Double parsing" << std::endl;
        ok = false;
    }

    // 3. Boolean True
    if (parser.parse("true"_padded).get(doc) != simdjson::SUCCESS || 
        bool(doc) != true) {
        std::cerr << "[FAIL] Boolean true" << std::endl;
        ok = false;
    }

    // 4. Boolean False
    if (parser.parse("false"_padded).get(doc) != simdjson::SUCCESS || 
        bool(doc) != false) {
        std::cerr << "[FAIL] Boolean false" << std::endl;
        ok = false;
    }

    // 5. Null
    if (parser.parse("null"_padded).get(doc) != simdjson::SUCCESS || 
        !doc.is_null()) {
        std::cerr << "[FAIL] Null parsing" << std::endl;
        ok = false;
    }

    // 6. String (Simple)
    std::string_view sv;
    if (parser.parse(R"("hello")"_padded).get(doc) != simdjson::SUCCESS || 
        doc.get(sv) != simdjson::SUCCESS || sv != "hello") {
        std::cerr << "[FAIL] String parsing" << std::endl;
        ok = false;
    }

    if (ok) std::cout << "[PASS] Basic Types" << std::endl;
    return ok;
}

bool test_structural_integration() {
    std::cout << "--- Testing Structural Integration (Arrays/Objects) ---" << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    bool ok = true;

    // 1. Simple Array
    // [1, 2, 3]
    if (parser.parse(R"([1, 2, 3])"_padded).get(doc) != simdjson::SUCCESS) {
        std::cerr << "[FAIL] Simple Array parse error" << std::endl;
        ok = false;
    } else {
        simdjson::dom::array arr;
        if (doc.get(arr) != simdjson::SUCCESS || arr.size() != 3) {
            std::cerr << "[FAIL] Simple Array structure/size" << std::endl;
            ok = false;
        }
    }

    // 2. Simple Object
    // {"a": 1, "b": 2}
    if (parser.parse(R"({"a": 1, "b": 2})"_padded).get(doc) != simdjson::SUCCESS) {
        std::cerr << "[FAIL] Simple Object parse error" << std::endl;
        ok = false;
    } else {
        simdjson::dom::object obj;
        if (doc.get(obj) != simdjson::SUCCESS || obj.size() != 2) {
             std::cerr << "[FAIL] Simple Object structure/size" << std::endl;
             ok = false;
        }
        int64_t val;
        if (obj["a"].get(val) != simdjson::SUCCESS || val != 1) {
             std::cerr << "[FAIL] Object key access 'a'" << std::endl;
             ok = false;
        }
    }

    if (ok) std::cout << "[PASS] Structural Integration" << std::endl;
    return ok;
}

bool test_large_integers() {
    std::cout << "--- Testing Large Integers (64-bit limits) ---" << std::endl;
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    bool ok = true;

    // Max int64
    std::string max_str = std::to_string(std::numeric_limits<int64_t>::max());
    if (parser.parse(simdjson::padded_string(max_str)).get(doc) != simdjson::SUCCESS || 
        int64_t(doc) != std::numeric_limits<int64_t>::max()) {
        std::cerr << "[FAIL] Int64 Max" << std::endl;
        ok = false;
    }

    // Min int64
    std::string min_str = std::to_string(std::numeric_limits<int64_t>::min());
    if (parser.parse(simdjson::padded_string(min_str)).get(doc) != simdjson::SUCCESS || 
        int64_t(doc) != std::numeric_limits<int64_t>::min()) {
        std::cerr << "[FAIL] Int64 Min" << std::endl;
        ok = false;
    }

    if (ok) std::cout << "[PASS] Large Integers" << std::endl;
    return ok;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Stage 2 (DOM) Smoketest                    " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool all_passed = true;
    all_passed &= test_basic_types();
    all_passed &= test_structural_integration();
    all_passed &= test_large_integers();

    if (all_passed) {
        std::cout << "SUCCESS: All Stage 2 tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more tests failed." << std::endl;
        return 1;
    }
}