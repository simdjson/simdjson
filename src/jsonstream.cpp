//
// Created by jpiotte on 10/23/19.
//

#include <simdjson/isadetection.h>
#include <atomic>
#include <simdjson/padded_string.h>
#include "simdjson/jsonstream.h"

using namespace simdjson;

//Architecture find_best_supported_implementation();
//int json_parse_implementation(ParsedJson &pj, bool realloc_if_needed);
//
//
//// function pointer type for json_parse
//using json_parse_functype = int(const char *buf, size_t len, size_t batch_size, ParsedJson &pj, bool realloc_if_needed);
//
//// Pointer that holds the json_parse implementation corresponding to the
//// available SIMD instruction set
//extern std::atomic<json_parse_functype *> json_parse_ptr;
//






JsonStream::JsonStream(const char *buf, size_t len, size_t batch_size) {
//    this->best_implementation = find_best_supported_implementation();
    this->buf = buf;
    this->len = len;
    this->batch_size = batch_size;
}

JsonStream::~JsonStream() {
    batch_size = 0;
}


int JsonStream::json_parse(ParsedJson &pj, bool realloc_if_needed) {
    //return json_parse_ptr.load(std::memory_order_relaxed)(buf, len, batch_size, pj, realloc_if_needed);

    if (pj.byte_capacity == 0) {
        bool allocok = pj.allocate_capacity(batch_size, batch_size);
        if (!allocok) {
            std::cerr << "can't allocate memory" << std::endl;
            return false;
        }
    }
    else if (pj.byte_capacity < batch_size) {
        return simdjson::CAPACITY;
    }

    if(next_json > 0){
        std::fill_n(pj.tape, sizeof(pj.tape)/sizeof(pj.tape[0]), 0);
        pj.current_loc = 0;
    }
    if (load_next_batch){
        auto current_buffer_loc{0};
        if(next_json > 0){
            current_buffer_loc = pj.structural_indexes[next_json];
        }

        auto remaining_len = len - current_buffer_loc;
        size_t next_batch_size = std::min(batch_size, remaining_len);
//        pj.structural_indexes = nullptr;
//        pj.n_structural_indexes = 0;
        int stage1_is_ok = simdjson::find_structural_bits<Architecture::HASWELL>(buf, next_batch_size + current_buffer_loc, pj, current_buffer_loc);

//        if (stage1_is_ok != simdjson::SUCCESS) {
//            pj.error_code = stage1_is_ok;
//            return pj.error_code;
//        }
        load_next_batch = false;
        next_json = 0;
    }



    int res = unified_machine<Architecture::HASWELL>(buf, len, pj, next_json);

    //have a more precise error check
    if ( res > 1 && error_count < 1 ) {
        load_next_batch = true;
        error_count++;
        res = json_parse(pj);
    }
    if (res <= 1) error_count = 0;

    return res;
}




//// TODO: generalize this set of functions.  We don't want to have a copy in jsonparser.cpp
//Architecture find_best_supported_implementation() {
//    constexpr uint32_t haswell_flags =
//            instruction_set::AVX2 | instruction_set::PCLMULQDQ |
//            instruction_set::BMI1 | instruction_set::BMI2;
//    constexpr uint32_t westmere_flags =
//            instruction_set::SSE42 | instruction_set::PCLMULQDQ;
//
//    uint32_t supports = detect_supported_architectures();
//    // Order from best to worst (within architecture)
//    if ((haswell_flags & supports) == haswell_flags)
//        return Architecture::HASWELL;
//    if ((westmere_flags & supports) == westmere_flags)
//        return Architecture::WESTMERE;
//    if (instruction_set::NEON)
//        return Architecture::ARM64;
//
//    std::cerr << "The processor is not supported by simdjson." << std::endl;
//    return Architecture::NONE;
//}





//template <Architecture T>
//int json_parse_implementation(const char *buf, size_t len, size_t batch_size, ParsedJson &pj, bool realloc_if_needed = true) {
//    //pj.reset()   have a function that resets pj without creating a new one
//    if (pj.byte_capacity < batch_size) {
//        return simdjson::CAPACITY;
//    }
//
//    size_t next_batch_size = std::min(batch_size, len);
//
//    int stage1_is_ok = simdjson::find_structural_bits<T>(buf, next_batch_size, pj);
//
//    if (stage1_is_ok != simdjson::SUCCESS) {
//        pj.error_code = stage1_is_ok;
//        return pj.error_code;
//    }
//
//
//    int res = unified_machine<T>(buf, len, pj);
//
//
//
//    return res;
//}
//
//// Responsible to select the best json_parse implementation
//int json_parse_dispatch(const char *buf, size_t len, size_t batch_size, ParsedJson &pj, bool realloc) {
//    Architecture best_implementation = find_best_supported_implementation();
//    // Selecting the best implementation
//    switch (best_implementation) {
//#ifdef IS_X86_64
//        case Architecture::HASWELL:
//            json_parse_ptr.store(&json_parse_implementation<Architecture::HASWELL>, std::memory_order_relaxed);
//            break;
//        case Architecture::WESTMERE:
//            json_parse_ptr.store(&json_parse_implementation<Architecture::WESTMERE>, std::memory_order_relaxed);
//            break;
//#endif
//#ifdef IS_ARM64
//        case Architecture::ARM64:
//    json_parse_ptr.store(&json_parse_implementation<Architecture::ARM64>, std::memory_order_relaxed);
//    break;
//#endif
//        default:
//            std::cerr << "The processor is not supported by simdjson." << std::endl;
//            return simdjson::UNEXPECTED_ERROR;
//    }
//
//    return json_parse_ptr.load(std::memory_order_relaxed)(buf, len, batch_size, pj, realloc);
//}
//
//std::atomic<json_parse_functype *> json_parse_ptr = &json_parse_dispatch;