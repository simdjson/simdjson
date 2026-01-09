#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include "simdjson.h"

// -------------------------------------------------------------------------
// RVV Backend: Comprehensive UTF-8 Validation Tests
// -------------------------------------------------------------------------
// Unlike the smoketest (which checks if it runs), this suite checks if it
// runs *correctly* under stress. It focuses on:
// 1. Block boundaries (64-byte chunks).
// 2. Split multi-byte characters (spanning vector registers).
// 3. Invalid bit patterns (overlongs, surrogates).
// -------------------------------------------------------------------------

// Helper to assert equality
#define ASSERT_EQUAL(name, actual, expected) \
    if (actual != expected) { \
        std::cerr << "[FAIL] " << name << ": Expected " << expected << ", got " << actual << std::endl; \
        return false; \
    } else { \
        std::cout << "[PASS] " << name << std::endl; \
    }

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
// Test Group 1: Standard Compliance (RFC 3629)
// -------------------------------------------------------------------------
bool test_standard_compliance() {
    bool ok = true;
    std::cout << "--- Testing Standard Compliance ---" << std::endl;

    struct Case { std::string name; std::string data; bool valid; };
    std::vector<Case> cases = {
        // ASCII
        {"Empty", "", true},
        {"ASCII", "Hello World", true},
        {"Null Byte", std::string("Hello\0World", 11), true}, // Valid in UTF-8, though often bad for C strings

        // 2-Byte
        {"2-Byte (Latin)", "\xC3\xB1", true}, // ñ
        {"2-Byte Truncated", "\xC3", false},
        {"2-Byte Overlong (space)", "\xC0\xA0", false}, // < 0x80 encoded as 2 bytes

        // 3-Byte
        {"3-Byte (Euro)", "\xE2\x82\xAC", true}, // €
        {"3-Byte Truncated 1", "\xE2", false},
        {"3-Byte Truncated 2", "\xE2\x82", false},
        {"3-Byte Overlong", "\xE0\x80\x80", false}, // Encodes 0
        {"Surrogate (Min)", "\xED\xA0\x80", false}, // D800
        {"Surrogate (Max)", "\xED\xBF\xBF", false}, // DFFF

        // 4-Byte
        {"4-Byte (Emoji)", "\xF0\x9F\x98\x80", true}, // 😀
        {"4-Byte Truncated 1", "\xF0", false},
        {"4-Byte Truncated 2", "\xF0\x9F", false},
        {"4-Byte Truncated 3", "\xF0\x9F\x98", false},
        {"4-Byte Too Large", "\xF4\x90\x80\x80", false}, // > 0x10FFFF
    };

    for (const auto& c : cases) {
        bool res = simdjson::validate_utf8(c.data.data(), c.data.size());
        if (res != c.valid) {
            std::cerr << "[FAIL] " << c.name << " (Expected " << (c.valid?"Valid":"Invalid") << ")" << std::endl;
            ok = false;
        }
    }
    return ok;
}

// -------------------------------------------------------------------------
// Test Group 2: Boundary Stress Testing (The "VLEN Killer")
// -------------------------------------------------------------------------
// This ensures that multi-byte characters crossing the 64-byte block boundary
// (or the hardware VLEN boundary) are handled correctly.
bool test_boundaries() {
    bool ok = true;
    std::cout << "--- Testing Block Boundaries ---" << std::endl;

    // We test around the 64-byte mark (simdjson standard block)
    // and 128/256 bytes (common hardware VLENs).
    std::vector<size_t> boundaries = {64, 128, 256, 1024};
    std::string filler(2000, 'a'); // Pure ASCII filler

    // Multi-byte sequence to inject: Euro symbol (3 bytes: E2 82 AC)
    std::string euro = "\xE2\x82\xAC";

    for (size_t boundary : boundaries) {
        // Case A: Character ENDS exactly at boundary
        // [ ... ascii ... ][E2][82][AC] | [ascii...]
        std::string s1 = filler.substr(0, boundary - 3) + euro + filler.substr(0, 10);
        if (!simdjson::validate_utf8(s1.data(), s1.size())) {
            std::cerr << "[FAIL] Valid Euro ending at boundary " << boundary << std::endl;
            ok = false;
        }

        // Case B: Character SPLIT 1/2 across boundary
        // [ ... ascii ... ][E2] | [82][AC][ascii...]
        std::string s2 = filler.substr(0, boundary - 1) + euro + filler.substr(0, 10);
        if (!simdjson::validate_utf8(s2.data(), s2.size())) {
            std::cerr << "[FAIL] Valid Euro split (1/2) at boundary " << boundary << std::endl;
            ok = false;
        }

        // Case C: Character SPLIT 2/1 across boundary
        // [ ... ascii ... ][E2][82] | [AC][ascii...]
        std::string s3 = filler.substr(0, boundary - 2) + euro + filler.substr(0, 10);
        if (!simdjson::validate_utf8(s3.data(), s3.size())) {
            std::cerr << "[FAIL] Valid Euro split (2/1) at boundary " << boundary << std::endl;
            ok = false;
        }

        // Case D: Invalid Split (Truncated at boundary)
        // [ ... ascii ... ][E2][82] | (EOF)
        std::string s4 = filler.substr(0, boundary - 2) + euro.substr(0, 2);
        if (simdjson::validate_utf8(s4.data(), s4.size())) {
            std::cerr << "[FAIL] Invalid Truncated Euro at boundary " << boundary << " passed validation!" << std::endl;
            ok = false;
        }
    }

    if (ok) std::cout << "[PASS] Boundary Tests" << std::endl;
    return ok;
}

// -------------------------------------------------------------------------
// Test Group 3: Random Fuzzing
// -------------------------------------------------------------------------
bool test_fuzzing() {
    std::cout << "--- Testing Random Permutations ---" << std::endl;
    // We generate random mostly-valid ASCII with occasional valid/invalid multibyte chars.
    std::mt19937 gen(12345);
    std::uniform_int_distribution<> dis(0, 255);

    // Simply check that we don't crash. Correctness is hard to verify randomly
    // without a reference implementation, but we assume the generic implementation
    // (if we fell back) or the previous tests covered logic.
    // This is primarily for SEGFAULT detection.

    for (int i = 0; i < 100; ++i) {
        size_t len = 100 + (dis(gen) % 1000);
        std::vector<char> buf(len);
        for (size_t j = 0; j < len; ++j) buf[j] = (char)dis(gen);

        // Just ensure it returns true/false and doesn't crash (e.g. invalid memory access)
        simdjson::validate_utf8(buf.data(), len);
    }

    std::cout << "[PASS] Fuzzing (No crashes observed)" << std::endl;
    return true;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV UTF-8 Correctness Test Suite               " << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!force_rvv()) {
        std::cerr << "[CRITICAL] Could not activate RVV backend." << std::endl;
        return 1;
    }
    std::cout << "Active Backend: " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool all_passed = true;
    all_passed &= test_standard_compliance();
    all_passed &= test_boundaries();
    all_passed &= test_fuzzing();

    if (all_passed) {
        std::cout << "SUCCESS: All UTF-8 tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: One or more tests failed." << std::endl;
        return 1;
    }
}
