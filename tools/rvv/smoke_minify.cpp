#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend Smoketest: Minify (Whitespace Removal)
// -------------------------------------------------------------------------
// This tool validates the RVV-optimized minify kernel (vcompress logic).
// It specifically tests:
// 1. Removal of all 4 JSON whitespace characters (Space, \n, \r, \t).
// 2. Preservation of non-whitespace characters.
// 3. Handling of 64-byte block boundaries and tail processing.
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
// Test Logic
// -------------------------------------------------------------------------
bool run_test(const std::string& name, const std::string& input, const std::string& expected) {
    // Allocate destination buffer (input length is safe upper bound)
    std::vector<uint8_t> dst(input.size());
    size_t dst_len = 0;

    // Call the minify API
    // Note: We use the active implementation's minify method directly via the public API helper.
    // Here we trust the public API delegates to the active backend (which we forced to RVV).
    auto error = simdjson::minify(input.data(), input.size(), reinterpret_cast<char*>(dst.data()), dst_len);

    if (error) {
         std::cerr << "[FAIL] " << name << ": API returned error " << error << std::endl;
         return false;
    }

    // Convert result to string for comparison
    std::string actual(reinterpret_cast<const char*>(dst.data()), dst_len);

    if (actual != expected) {
        std::cerr << "[FAIL] " << name << "\n"
                  << "       Input:    '" << input << "'\n"
                  << "       Expected: '" << expected << "'\n"
                  << "       Got:      '" << actual << "'" << std::endl;
        return false;
    }

    std::cout << "[PASS] " << name << std::endl;
    return true;
}

// -------------------------------------------------------------------------
// Test Cases
// -------------------------------------------------------------------------
bool test_whitespace_types() {
    std::cout << "--- Testing Whitespace Types ---" << std::endl;
    bool ok = true;
    
    // Space, Tab, LF, CR
    ok &= run_test("All Whitespace Types", 
        " \t\n\r{\"a\":1} \t\n\r", 
        "{\"a\":1}");
        
    ok &= run_test("Interleaved", 
        "{\n\"key\":\t123\r}", 
        "{\"key\":123}");

    return ok;
}

bool test_block_boundaries() {
    std::cout << "--- Testing Block Boundaries ---" << std::endl;
    bool ok = true;

    // Spec defines 64-byte blocks. 
    // We create a string that has whitespace/non-whitespace crossing the 63/64/65 boundary.
    
    // 60 chars of noise + critical section
    std::string padding(60, ' '); 
    std::string input = padding + "{\"a\":1}"; // starts at 60
    std::string expected = "{\"a\":1}";
    
    ok &= run_test("Crossing 64B Boundary", input, expected);

    // Exact fit
    std::string exact(64, ' '); // 64 spaces -> empty
    ok &= run_test("Full 64B Whitespace", exact, "");

    // 64 spaces + 1 char
    std::string overflow(64, ' ');
    overflow += "x";
    ok &= run_test("64B WS + 1 char", overflow, "x");

    return ok;
}

bool test_correctness_preservation() {
    std::cout << "--- Testing Content Preservation ---" << std::endl;
    bool ok = true;
    
    // Ensure we don't accidentally strip non-whitespace or mangle data
    // Note: The current kernel (M6) performs context-free whitespace removal.
    // It does not respect quotes. We test simple structural preservation here.
    
    ok &= run_test("Preserve Non-Whitespace", "a,b,c", "a,b,c");
    ok &= run_test("Numbers", "[1, 2, 3]", "[1,2,3]");
    
    return ok;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Minify (Whitespace Removal) Smoketest      " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool all_passed = true;
    all_passed &= test_whitespace_types();
    all_passed &= test_block_boundaries();
    all_passed &= test_correctness_preservation();

    if (all_passed) {
        std::cout << "SUCCESS: All Minify tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more tests failed." << std::endl;
        return 1;
    }
}