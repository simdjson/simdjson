/*
 * Minifies using the minify() function directly, without parsing.
 *
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
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    using Buffer=std::vector<uint8_t>;
    auto minify=[Data,Size](const simdjson::implementation* impl) -> Buffer {
        Buffer ret(Size);
        std::size_t retsize=0;
        auto err=impl->minify(Data,Size,ret.data(),retsize);
        if(err) {
            std::string tmp = error_message(err);
            ret.assign(tmp.begin(),tmp.end());
        } else {
            assert(retsize<=Size && "size should not grow by minimize()!");
            ret.resize(retsize);
        }
        return ret;
    };


    auto const first = simdjson::available_implementations.begin();
    auto const last = simdjson::available_implementations.end();


    auto it = first;
    while((it != last) && (!(*it)->supported_by_runtime_system())) { it++; }
    assert(it != last);

    const auto reference=minify(*first);

    bool failed=false;
    for(;it != last; ++it) {
        if(!(*it)->supported_by_runtime_system()) { continue; }
        const auto current=minify(*it);
        if(current!=reference) {
            failed=true;
        }
    }

    if(failed) {
        std::cerr<<std::boolalpha<<"Mismatch between implementations of minify() found:\n";
        for(it = first;it != last; ++it) {
            if(!(*it)->supported_by_runtime_system()) { continue; }
            const auto current=minify(*it);
            std::string tmp(current.begin(),current.end());
            std::cerr<<(*it)->name()<<" returns "<<tmp<<std::endl;
        }
        std::abort();
    }

    //all is well
    return 0;
}
