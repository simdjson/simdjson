#pragma once

#include "simdjson.h"
#include <vector>
#include <cstdlib>

/**
 * @brief get_runtime_supported_implementations
 * Returns a vector of implementations, which both
 * have been compiled *and* are dynamically checked to
 * be supported at runtime.
 *
 * Aborts if no implementations are available (should not happen, fallback
 * should always be there for us!)
 * @return
 */
std::vector<const simdjson::implementation*>
get_runtime_supported_implementations() {
    std::vector<const simdjson::implementation*> ret;
    for(auto& e: simdjson::get_available_implementations()) {
        if(e->supported_by_runtime_system()) {
            ret.emplace_back(e);
        }
    }
    if(ret.empty()) {
        // No implementations available, not even fallback, weird.
        std::abort();
    }
    return ret;
}
