#include <iostream>
#include <vector>
#include <memory>

// Include the public API
#include "simdjson.h"

// Include the RVV backend specifically to check internals
// (This is allowed in backend-specific tests)
#include "simdjson/rvv.h"

// Helper macro for assertion with message
#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        std::cerr << "[FAIL] " << msg << " (Line " << __LINE__ << ")" << std::endl; \
        return 1; \
    } else { \
        std::cout << "[PASS] " << msg << std::endl; \
    }

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RVV Backend Compilation & Registry Tests       " << std::endl;
    std::cout << "==================================================" << std::endl;

    // ---------------------------------------------------------------------
    // 1. Macro & Environment Checks
    // ---------------------------------------------------------------------
    // Ensure we are actually compiling what we think we are.
#if defined(SIMDJSON_IS_RVV)
    std::cout << "[INFO] SIMDJSON_IS_RVV is defined." << std::endl;
#else
    // This isn't necessarily fatal for the *library* (it might fall back),
    // but for this *test*, it is fatal.
    std::cerr << "[FAIL] SIMDJSON_IS_RVV is NOT defined. Check compiler flags." << std::endl;
    return 1;
#endif

    // ---------------------------------------------------------------------
    // 2. Structural & Alignment Checks (Crucial for SIMD)
    // ---------------------------------------------------------------------
    // We defined simd8x64 to be a wrapper around a 64-byte aligned buffer.
    // If this size is wrong, memory safety is compromised.
    
    size_t size_u8 = sizeof(simdjson::rvv::simd8x64<uint8_t>);
    size_t align_u8 = alignof(simdjson::rvv::simd8x64<uint8_t>);

    std::cout << "[INFO] simd8x64<uint8_t> Size:  " << size_u8 << " (Expected 64)" << std::endl;
    std::cout << "[INFO] simd8x64<uint8_t> Align: " << align_u8 << " (Expected 64)" << std::endl;

    TEST_ASSERT(size_u8 == 64, "simd8x64 size must be exactly 64 bytes");
    TEST_ASSERT(align_u8 == 64, "simd8x64 must be aligned to 64 bytes");

    // ---------------------------------------------------------------------
    // 3. Implementation Registry Check
    // ---------------------------------------------------------------------
    // Does the standard simdjson registry know about "rvv"?
    
    bool found_in_registry = false;
    for (auto impl : simdjson::get_available_implementations()) {
        if (impl->name() == "rvv") {
            found_in_registry = true;
            std::cout << "[INFO] Found 'rvv' in available implementations." << std::endl;
            
            // Check properties
            TEST_ASSERT(impl->instruction_set() == simdjson::internal::instruction_set::RVV, 
                        "Instruction set enum mismatch");
        }
    }
    
    TEST_ASSERT(found_in_registry, "The RVV backend was not registered in available_implementations()");

    // ---------------------------------------------------------------------
    // 4. Direct Instantiation Check
    // ---------------------------------------------------------------------
    // Can we manually instantiate the implementation class? 
    // This catches linker errors (missing symbols in src/rvv.cpp).
    
    {
        simdjson::rvv::implementation rvv_impl;
        std::cout << "[INFO] Successfully instantiated simdjson::rvv::implementation." << std::endl;
        
        // Basic method check (should not crash)
        auto name = rvv_impl.name();
        TEST_ASSERT(name == "rvv", "Implementation name mismatch");
    }

    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "SUCCESS: All compilation tests passed." << std::endl;
    return 0;
}