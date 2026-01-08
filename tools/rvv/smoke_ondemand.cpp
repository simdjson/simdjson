#include <iostream>
#include <string>
#include <vector>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Smoketest: On-Demand API
// -------------------------------------------------------------------------
// This tool validates that the RVV backend correctly supports the On-Demand
// API (simdjson::ondemand).
//
// Integration Point:
// The On-Demand API relies on the backend's Stage 1 to produce a tape of
// structural indexes. If Stage 1 is broken (e.g., missing commas, wrong
// string masking), On-Demand iterators will fail or return wrong values.
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

bool test_basic_traversal() {
    std::cout << "--- Testing Basic Traversal ---" << std::endl;
    simdjson::ondemand::parser parser;
    simdjson::padded_string json = R"({ "x": 1, "y": 2 })"_padded;

    simdjson::ondemand::document doc;
    if (parser.iterate(json).get(doc) != simdjson::SUCCESS) {
        std::cerr << "[FAIL] Failed to start iteration" << std::endl;
        return false;
    }

    int64_t x, y;
    if (doc["x"].get(x) != simdjson::SUCCESS || x != 1) {
        std::cerr << "[FAIL] Failed to read key 'x'" << std::endl;
        return false;
    }
    if (doc["y"].get(y) != simdjson::SUCCESS || y != 2) {
        std::cerr << "[FAIL] Failed to read key 'y'" << std::endl;
        return false;
    }

    std::cout << "[PASS] Basic Traversal" << std::endl;
    return true;
}

bool test_array_iteration() {
    std::cout << "--- Testing Array Iteration ---" << std::endl;
    simdjson::ondemand::parser parser;
    simdjson::padded_string json = R"([1, 2, 3])"_padded;

    simdjson::ondemand::document doc;
    if (parser.iterate(json).get(doc) != simdjson::SUCCESS) return false;

    int64_t sum = 0;
    for (auto val : doc) {
        int64_t v;
        if (val.get(v) != simdjson::SUCCESS) {
            std::cerr << "[FAIL] Bad array element" << std::endl;
            return false;
        }
        sum += v;
    }

    if (sum != 6) {
        std::cerr << "[FAIL] Array sum mismatch. Expected 6, got " << sum << std::endl;
        return false;
    }

    std::cout << "[PASS] Array Iteration" << std::endl;
    return true;
}

bool test_large_skip() {
    std::cout << "--- Testing Large Value Skipping ---" << std::endl;
    // This tests the backend's ability to skip over large chunks of data
    // using the structural indexes.

    std::string big_array = "[";
    for(int i=0; i<1000; i++) big_array += "1,";
    big_array += "2]";
    simdjson::padded_string json(big_array);

    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    if (parser.iterate(json).get(doc) != simdjson::SUCCESS) return false;

    // We only care about the last element. On-Demand should skip the rest.
    // Note: On-Demand array iterator is forward-only. We iterate to end.
    int64_t last_val = 0;
    for(auto val : doc) {
        val.get(last_val); // ignore error for speed, just get last
    }

    if (last_val != 2) {
        std::cerr << "[FAIL] Failed to retrieve last element after skip" << std::endl;
        return false;
    }

    std::cout << "[PASS] Large Skip" << std::endl;
    return true;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV On-Demand API Smoketest                    " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool all_passed = true;
    all_passed &= test_basic_traversal();
    all_passed &= test_array_iteration();
    all_passed &= test_large_skip();

    if (all_passed) {
        std::cout << "SUCCESS: All On-Demand tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more tests failed." << std::endl;
        return 1;
    }
}
