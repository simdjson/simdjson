#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <cmath>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Smoketest: Number Parsing
// -------------------------------------------------------------------------
// This tool isolates the number parsing subsystem.
//
// Purpose:
// 1. Verify correct parsing of Integers (Signed/Unsigned) via the RVV backend.
// 2. Verify correct parsing of Floating Point numbers (Double).
// 3. Verify handling of Scientific Notation.
// 4. Confirm that the fallback wiring (M0) or vector optimization (M4)
//    is functioning correctly.
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
// Helper: Run Parse Test
// -------------------------------------------------------------------------
template <typename T>
bool check_value(const std::string& json, T expected, const std::string& type_name) {
    simdjson::dom::parser parser;
    simdjson::dom::element doc;

    auto error = parser.parse(simdjson::padded_string(json)).get(doc);
    if (error) {
        std::cerr << "[FAIL] Parse error for " << type_name << ": " << error << std::endl;
        return false;
    }

    T actual;
    if (doc.get(actual) != simdjson::SUCCESS) {
        std::cerr << "[FAIL] Type mismatch or retrieval error for " << type_name << std::endl;
        return false;
    }

    // For doubles, use approximate equality
    if constexpr (std::is_floating_point_v<T>) {
        if (std::abs(actual - expected) > 1e-9) {
             std::cerr << "[FAIL] Value mismatch for " << type_name << "\n"
                       << "       Expected: " << expected << "\n"
                       << "       Got:      " << actual << std::endl;
             return false;
        }
    } else {
        if (actual != expected) {
             std::cerr << "[FAIL] Value mismatch for " << type_name << "\n"
                       << "       Expected: " << expected << "\n"
                       << "       Got:      " << actual << std::endl;
             return false;
        }
    }

    std::cout << "[PASS] " << type_name << ": " << json << std::endl;
    return true;
}

// -------------------------------------------------------------------------
// Test Cases
// -------------------------------------------------------------------------

bool test_integers() {
    std::cout << "--- Testing Integers ---" << std::endl;
    bool ok = true;

    ok &= check_value<int64_t>("0", 0, "Zero");
    ok &= check_value<int64_t>("1", 1, "One");
    ok &= check_value<int64_t>("-1", -1, "Negative One");
    ok &= check_value<int64_t>("9223372036854775807", std::numeric_limits<int64_t>::max(), "Int64 Max");
    ok &= check_value<int64_t>("-9223372036854775808", std::numeric_limits<int64_t>::min(), "Int64 Min");

    return ok;
}

bool test_doubles() {
    std::cout << "--- Testing Doubles ---" << std::endl;
    bool ok = true;

    ok &= check_value<double>("0.0", 0.0, "Double Zero");
    ok &= check_value<double>("3.14159", 3.14159, "Pi");
    ok &= check_value<double>("-123.456", -123.456, "Negative Double");
    ok &= check_value<double>("1.23e10", 1.23e10, "Scientific (e)");
    ok &= check_value<double>("1.23E+10", 1.23e10, "Scientific (E+)");
    ok &= check_value<double>("1.23e-5", 1.23e-5, "Scientific (e-)");

    return ok;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Number Parsing Smoketest                   " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool all_passed = true;
    all_passed &= test_integers();
    all_passed &= test_doubles();

    if (all_passed) {
        std::cout << "SUCCESS: All Number Parsing tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more tests failed." << std::endl;
        return 1;
    }
}
