#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: Equivalence (Golden Master) Tests
// -------------------------------------------------------------------------
// Purpose:
// Prove that the RVV backend produces identical DOM trees to the
// reference scalar implementation ("fallback") for identical inputs.
//
// This validates:
// 1. Structural parity (same hierarchy).
// 2. Data parity (same numbers, same strings).
// 3. Error parity (same error codes for invalid inputs).
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
// Helpers: DOM Comparison
// -------------------------------------------------------------------------

// Recursively compare two DOM elements for equality
bool are_equal(simdjson::dom::element a, simdjson::dom::element b, std::string& reason) {
    if (a.type() != b.type()) {
        reason = "Type mismatch (" + std::to_string((int)a.type()) + " vs " + std::to_string((int)b.type()) + ")";
        return false;
    }

    switch (a.type()) {
        case simdjson::dom::element_type::ARRAY: {
            auto arr_a = a.get_array();
            auto arr_b = b.get_array();
            if (arr_a.size() != arr_b.size()) {
                reason = "Array size mismatch";
                return false;
            }
            auto it_a = arr_a.begin();
            auto it_b = arr_b.begin();
            int idx = 0;
            while (it_a != arr_a.end()) {
                if (!are_equal(*it_a, *it_b, reason)) {
                    reason = "Array index " + std::to_string(idx) + ": " + reason;
                    return false;
                }
                ++it_a; ++it_b; ++idx;
            }
            return true;
        }
        case simdjson::dom::element_type::OBJECT: {
            auto obj_a = a.get_object();
            auto obj_b = b.get_object();
            if (obj_a.size() != obj_b.size()) {
                reason = "Object size mismatch";
                return false;
            }
            // Note: Key order is preserved in simdjson, so we can iterate in lockstep
            auto it_a = obj_a.begin();
            auto it_b = obj_b.begin();
            while (it_a != obj_a.end()) {
                std::string_view key_a = (*it_a).key;
                std::string_view key_b = (*it_b).key;
                if (key_a != key_b) {
                    reason = "Object key mismatch (" + std::string(key_a) + " vs " + std::string(key_b) + ")";
                    return false;
                }
                if (!are_equal((*it_a).value, (*it_b).value, reason)) {
                    reason = "Key [" + std::string(key_a) + "]: " + reason;
                    return false;
                }
                ++it_a; ++it_b;
            }
            return true;
        }
        case simdjson::dom::element_type::INT64:
            if (int64_t(a) != int64_t(b)) {
                reason = "Int64 mismatch";
                return false;
            }
            return true;
        case simdjson::dom::element_type::UINT64:
            if (uint64_t(a) != uint64_t(b)) {
                reason = "UInt64 mismatch";
                return false;
            }
            return true;
        case simdjson::dom::element_type::DOUBLE: {
            double val_a = double(a);
            double val_b = double(b);
            // We expect bit-exactness if we share the same number parser.
            // If implementation diverges, we might need epsilon.
            // For now, strict check.
            if (val_a != val_b && !(std::isnan(val_a) && std::isnan(val_b))) {
                reason = "Double mismatch";
                return false;
            }
            return true;
        }
        case simdjson::dom::element_type::STRING: {
            std::string_view val_a = std::string_view(a);
            std::string_view val_b = std::string_view(b);
            if (val_a != val_b) {
                reason = "String mismatch";
                return false;
            }
            return true;
        }
        case simdjson::dom::element_type::BOOL:
            if (bool(a) != bool(b)) {
                reason = "Bool mismatch";
                return false;
            }
            return true;
        case simdjson::dom::element_type::NULL_VALUE:
            return true;
        default:
            reason = "Unknown type";
            return false;
    }
}

// -------------------------------------------------------------------------
// Helper: Switch & Parse
// -------------------------------------------------------------------------
struct Result {
    simdjson::error_code error;
    simdjson::dom::element doc;
    // We must keep the parser alive as the doc references it
};

// We cannot easily return a 'doc' that outlives the parser/implementation switch
// in a clean thread-safe way without careful orchestration.
// Simpler approach: Run the logic sequentially.

bool verify_equivalence(const std::string& name, const std::string& json_str) {
    simdjson::padded_string json(json_str);
    std::string reason;

    // 1. Run Reference Implementation (Fallback)
    // We explicitly look for "fallback" or "westmere" or "generic".
    // "fallback" is always present.
    auto fallback_impl = simdjson::get_available_implementations()["fallback"];
    if (!fallback_impl) {
        std::cerr << "[SETUP] Fallback implementation not found!" << std::endl;
        return false;
    }

    simdjson::get_active_implementation() = fallback_impl;
    simdjson::dom::parser parser_ref;
    simdjson::dom::element doc_ref;
    auto err_ref = parser_ref.parse(json).get(doc_ref);

    // 2. Run Target Implementation (RVV)
    auto rvv_impl = simdjson::get_available_implementations()["rvv"];
    if (!rvv_impl) {
        std::cerr << "[SETUP] RVV implementation not found!" << std::endl;
        return false;
    }

    simdjson::get_active_implementation() = rvv_impl;
    simdjson::dom::parser parser_rvv;
    simdjson::dom::element doc_rvv;
    auto err_rvv = parser_rvv.parse(json).get(doc_rvv);

    // 3. Compare Results

    // A) Error Code Parity
    if (err_ref != err_rvv) {
        std::cerr << "[FAIL] " << name << ": Error code mismatch.\n"
                  << "       Ref: " << err_ref << "\n"
                  << "       RVV: " << err_rvv << std::endl;
        return false;
    }

    // If both failed, they match (provided error codes match).
    if (err_ref != simdjson::SUCCESS) {
        std::cout << "[PASS] " << name << " (Both returned error: " << err_ref << ")" << std::endl;
        return true;
    }

    // B) Content Parity
    if (!are_equal(doc_ref, doc_rvv, reason)) {
        std::cerr << "[FAIL] " << name << ": Content mismatch.\n"
                  << "       Reason: " << reason << std::endl;
        return false;
    }

    std::cout << "[PASS] " << name << std::endl;
    return true;
}

// -------------------------------------------------------------------------
// Test Cases
// -------------------------------------------------------------------------

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV vs Fallback Equivalence Tests              " << std::endl;
    std::cout << "==================================================" << std::endl;

    bool all_passed = true;

    // 1. Primitives
    all_passed &= verify_equivalence("Integers", "[1, -1, 0, 9223372036854775807]");
    all_passed &= verify_equivalence("Floats", "[1.2, -3.4, 0.0, 1.5e10]");
    all_passed &= verify_equivalence("Booleans/Null", "[true, false, null]");

    // 2. Structures
    all_passed &= verify_equivalence("Nested Array", "[[1,2],[3,4]]");
    all_passed &= verify_equivalence("Object", R"({"a": 1, "b": "text"})");
    all_passed &= verify_equivalence("Mixed", R"({"data": [1, true, null, {"nested": 0.5}]})");

    // 3. Strings & Escapes
    all_passed &= verify_equivalence("Simple String", R"(["hello world"])");
    all_passed &= verify_equivalence("Escapes", R"(["line\nfeed", "tab\t", "quote\""])");
    all_passed &= verify_equivalence("Unicode", R"(["\u00A9 Copyright", "Björk", "😊"])");

    // 4. Edge Cases (should produce errors)
    all_passed &= verify_equivalence("Unclosed Array", "[1, 2, 3");
    all_passed &= verify_equivalence("Trailing Comma", "[1, 2, 3,]");
    all_passed &= verify_equivalence("Bad Number", "[0123]");

    // 5. Large Input (Forces multiple blocks)
    std::string large_json = "[";
    for(int i=0; i<1000; i++) large_json += R"({"id": )" + std::to_string(i) + "},";
    large_json.pop_back(); // remove last comma
    large_json += "]";
    all_passed &= verify_equivalence("Large Array (1000 items)", large_json);

    std::cout << "--------------------------------------------------" << std::endl;

    if (all_passed) {
        std::cout << "SUCCESS: All equivalence tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: Discrepancy detected between RVV and Reference." << std::endl;
        return 1;
    }
}
