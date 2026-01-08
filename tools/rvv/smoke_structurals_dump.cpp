#include <iostream>
#include <iomanip>
#include <bitset>
#include <vector>
#include <string>
#include <algorithm>

// -------------------------------------------------------------------------
// RVV Backend Tool: Structural Mask Dumper
// -------------------------------------------------------------------------
// Purpose:
// Directly invokes the RVV Stage 1 classification kernel (json_character_block)
// on an input string and prints the resulting 64-bit masks.
//
// useful for verifying that:
// 1. Whitespace is correctly identified.
// 2. Structural characters (op) are correctly identified.
// 3. The 'store + pack' logic is working as expected.
// -------------------------------------------------------------------------

// We need internal access to the RVV backend headers
#include "simdjson/rvv/base.h"
#include "simdjson/rvv/rvv_config.h"
#include "simdjson/rvv/simd.h"
#include "simdjson/rvv/stringparsing_defs.h"

// Helper: Print a 64-bit mask in binary (MSB on left, usually corresponds to higher memory address)
// Note: In little-endian, bit 0 is the first byte. We print bit 0 on the RIGHT for standard math view,
// OR bit 0 on the LEFT for string alignment view.
// Let's use string alignment view (Bit 0 = Byte 0) for easier visual correlation with the string.
void print_mask_aligned(uint64_t mask) {
    for (int i = 0; i < 64; ++i) {
        std::cout << ((mask >> i) & 1);
    }
}

int main(int argc, char *argv[]) {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Structural Mask Dumper (Low-Level)         " << std::endl;
    std::cout << "==================================================" << std::endl;

    // 1. Input Handling
    // Default test string if none provided
    std::string input = R"({"key": "value", "array": [1, 2, 3], "null": null})";

    if (argc > 1) {
        input = argv[1];
    }

    // Pad input to ensure we can load full 64-byte blocks safely
    // (simulating the padded_string behavior)
    size_t len = input.size();
    size_t padded_len = (len + 63) & ~63; // Align to 64
    if (padded_len < len + 64) padded_len += 64; // Extra safety buffer

    std::vector<uint8_t> buffer(padded_len, ' '); // Fill with spaces (whitespace)
    std::copy(input.begin(), input.end(), buffer.begin());

    std::cout << "Input Length: " << len << " bytes" << std::endl;
    std::cout << "Analysis (Block Size = " << simdjson::rvv::RVV_BLOCK_BYTES << " bytes):" << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    // 2. Iterate Logic
    size_t offset = 0;
    while (offset < len) {
        std::cout << "\n[Block Offset " << std::setw(4) << offset << "]" << std::endl;

        // Load 64 bytes
        // We use the raw SIMD abstraction defined in simd.h
        const uint8_t* ptr = buffer.data() + offset;

        // Note: We assume the M1 'load' implementation in simd8x64 is active.
        // If not fully implemented yet, this relies on the implicit copy constructor
        // or a load helper if one exists. For this smoke test, we simulate the load
        // by constructing from pointer if the constructor exists, otherwise we'd need a specific loader.
        // Assuming simd8x64<uint8_t> has a constructor from pointer (Standard in simdjson backends).
        simdjson::rvv::simd8x64<uint8_t> chunk(ptr);

        // Classify
        auto result = simdjson::rvv::json_character_block::classify(chunk);

        // 3. Visualization
        // Print the string chunk for reference
        std::cout << "Text:  ";
        for(int i=0; i<64; ++i) {
            char c = (char)ptr[i];
            if (c < 0x20 || c > 0x7E) c = '.'; // Replace non-printable
            std::cout << c;
        }
        std::cout << std::endl;

        // Print Masks
        // We print '1' where the bit is set, matched to the character above
        std::cout << "Struct:";
        print_mask_aligned(result.op);
        std::cout << "  (Hex: 0x" << std::hex << result.op << std::dec << ")" << std::endl;

        std::cout << "White: ";
        print_mask_aligned(result.whitespace);
        std::cout << "  (Hex: 0x" << std::hex << result.whitespace << std::dec << ")" << std::endl;

        offset += 64;
    }

    std::cout << "\nDone." << std::endl;
    return 0;
}
