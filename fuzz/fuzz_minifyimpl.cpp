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
#include <iostream>
#include <vector>
#include "supported_implementations.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

  // since this check is expensive, only do it once
  static const auto implementations=get_runtime_supported_implementations();

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

    auto const first = implementations.begin();
    auto const last = implementations.end();

    const auto reference=minify(*first);

    bool failed=false;
    for(auto it=first+1;it != last; ++it) {
        const auto current=minify(*it);
        if(current!=reference) {
            failed=true;
        }
    }

    if(failed) {
        std::cerr<<std::boolalpha<<"Mismatch between implementations of minify() found:\n";
        for(const auto& e:implementations) {
            const auto current=minify(e);
            std::string tmp(current.begin(),current.end());
            std::cerr<<e->name()<<" returns "<<tmp<<std::endl;
        }
        std::abort();
    }

    //all is well
    return 0;
}
