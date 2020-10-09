/*
 * For fuzzing all of the implementations (haswell/fallback/westmere),
 * finding any difference between the output of each which would
 * indicate inconsistency. Also, it gets the non-default backend
 * some fuzzing love.
 *
 * Copyright Paul Dreik 20200912 for the simdjson project.
 */

#include "simdjson.h"
#include <cstddef>
#include <cstdlib>
#include "supported_implementations.h"


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    // since this check is expensive, only do it once
    static const auto supported_implementations=get_runtime_supported_implementations();


    auto utf8verify=[Data,Size](const simdjson::implementation* impl) -> bool {
        return impl->validate_utf8((const char*)Data,Size);
    };


    auto first = supported_implementations.begin();
    auto last = supported_implementations.end();


    const bool reference=utf8verify(*first);

    bool failed=false;
    for(auto it=first+1; it != last; ++it) {
        const bool current=utf8verify(*it);
        if(current!=reference) {
            failed=true;
        }
    }

    if(failed) {
        std::cerr<<std::boolalpha<<"Mismatch between implementations of validate_utf8() found:\n";
        for(const auto& e: supported_implementations) {
            if(!e->supported_by_runtime_system()) { continue; }
            const bool current=utf8verify(e);
            std::cerr<<e->name()<<" returns "<<current<<std::endl;
        }
        std::abort();
    }

    //all is well
    return 0;
}
