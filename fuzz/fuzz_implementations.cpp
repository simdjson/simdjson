/*
 * For fuzzing all of the implementations (haswell/fallback/westmere),
 * finding any difference between the output of each which would
 * indicate inconsistency. Also, it gets the non-default backend
 * some fuzzing love.
 *
 * Copyright Paul Dreik 20200909 for the simdjson project.
 */

#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <array>
#include "supported_implementations.h"


// store each implementation along with it's intermediate results,
// which would make things easier to debug in case this fuzzer ever
// catches anything
struct Impl {
    explicit Impl(const simdjson::implementation* im=nullptr) : impl(im),parser(),element(),error(),output(){}
    //silence -Weffc++
    Impl(const Impl&)=delete;
    Impl& operator=(const Impl&)=delete;

    const simdjson::implementation* impl;
    simdjson::dom::parser parser;
    simdjson::dom::element element;
    simdjson::error_code error;
    std::string output;
};

template<class Iterator>
void showErrorAndAbort(Iterator first, Iterator last) {
    auto it=first;
    while(it!=last) {
        std::cerr<<"Implementation: "<<it->impl->name()<<"\tError:"<<it->error<<'\n';
        it++;
    }
    std::cerr.flush();
    std::abort();
}

template<class Iterator>
void showOutputAndAbort(Iterator first, Iterator last) {

    for(auto it=first;it!=last;++it) {
        std::cerr<<"Implementation: "<<it->impl->name()<<"\tOutput: "<<it->output<<'\n';
    }

    // show the pairwise results
    for(auto it1=first; it1!=last; ++it1) {
        for(auto it2=it1; it2!=last; ++it2) {
            if(it1!=it2) {
                const bool matches=(it1->output==it2->output);
                std::cerr<<"Implementation "<<it1->impl->name()<<" and "<<it2->impl->name()<<(matches?" match.":" do NOT match.")<<'\n';
            }
        }
    }
    std::cerr.flush();
    std::abort();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

  // since this check is expensive, only do it once
  static const auto supported_implementations=get_runtime_supported_implementations();


    // make this dynamic, so it works regardless of how it was compiled
    // or what hardware it runs on
    constexpr std::size_t Nimplementations_max=3;
    const std::size_t Nimplementations = supported_implementations.size();

    if(Nimplementations>Nimplementations_max) {
        //there is another backend added, please bump Nimplementations_max!
        std::abort();
    }

    // get pointers to the backend implementation
    std::array<Impl,Nimplementations_max> implementations;
    {
        std::size_t i=0;
        for(auto& e: supported_implementations) {
              implementations[i++].impl=e;
        }
    }

    // let each implementation parse and store the result
    std::size_t nerrors=0;
    for(std::size_t i=0; i<Nimplementations; ++i) {
        auto& e=implementations[i];
        simdjson::get_active_implementation()=e.impl;
        e.error=e.parser.parse(Data,Size).get(e.element);
        if(e.error) {
            ++nerrors;
        } else {
            std::ostringstream oss;
            oss<<e.element;
            e.output=oss.str();
        }
    }

    //we should either have no errors, or all should error
    if(nerrors!=0) {
        if(nerrors!=Nimplementations) {
            showErrorAndAbort(implementations.begin(),
                              implementations.begin()+Nimplementations);
        }
        return 0;
    }

    //parsing went well for all. compare the output against the first.
    const std::string& reference=implementations[0].output;
    for(std::size_t i=1; i<Nimplementations; ++i) {
        if(implementations[i].output!=reference) {
            showOutputAndAbort(implementations.begin(),
                              implementations.begin()+Nimplementations);
        }
    }

    //all is well
    return 0;
}
