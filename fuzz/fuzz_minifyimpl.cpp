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
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    using Buffer=std::vector<uint8_t>;
    auto minify=[Data,Size](const simdjson::implementation* impl) -> Buffer {
        Buffer ret(Size+simdjson::SIMDJSON_PADDING);
        std::size_t retsize=0;
        if(impl->minify(Data,Size,ret.data(),retsize)) {
            auto tmp=impl->name();
            tmp += " gave error";
            ret.assign(tmp.begin(),tmp.end());
        } else {
            ret.resize(retsize);
        }
        return ret;
    };


    auto first=simdjson::available_implementations.begin();
    auto last=simdjson::available_implementations.end();

    //make sure there is an implementation
    assert(first!=last);

    const auto reference=minify(*first);

    bool failed=false;
    for(auto it=first+1;it!=last; ++it) {
        const auto current=minify(*it);
        if(current!=reference) {
            // hmm, seems like it is possible that the fallback can succeed where
            // the simd implementations fail. might need to allow that? or is
            // the fallback supposed to behave identically?
            failed=true;
        }
    }

    if(failed) {
        std::cerr<<std::boolalpha<<"Mismatch between implementations of minify() found:\n";
        for(auto it=first;it!=last; ++it) {
            const auto current=minify(*it);
            std::string tmp(current.begin(),current.end());
            std::cerr<<(*it)->name()<<" returns "<<tmp<<std::endl;
        }
        std::abort();
    }


    //all is well
    return 0;
}
