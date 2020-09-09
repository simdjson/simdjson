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
#include <string>
#include <array>


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

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    // make this dynamic, so it works regardless of how it was compiled
    // or what hardware it runs on
    constexpr std::size_t Nimplementations_max=3;
    const std::size_t Nimplementations=simdjson::available_implementations.size();
    if(Nimplementations>Nimplementations_max) {
        //there is another backend added, please bump Nimplementations_max!
        std::abort();
    }

    // get pointers to the backend implementation
    std::array<Impl,Nimplementations_max> implementations;
    {
        std::size_t i=0;
        for(auto& e: simdjson::available_implementations) {
            implementations[i++].impl=e;
        }
    }

    // let each implementation parse and store the result
    std::size_t nerrors=0;
    for(auto& e: implementations) {
        simdjson::active_implementation=e.impl;
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
            std::abort();
        }
        return 0;
    }

    //parsing went well for all. compare the output against the first.
    const std::string& reference=implementations[0].output;
    for(std::size_t i=1; i<Nimplementations; ++i) {
        if(implementations[i].output!=reference) {
            //mismatch on the output
            std::abort();
        }
    }

    //all is well
    return 0;
}
