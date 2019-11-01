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


void JsonStream::set_new_buffer(const char *buf, size_t len) {
    this->buf = buf;
    this->len = len;
    batch_size = 0;
    batch_size = 0;
    next_json = 0;
    current_buffer_loc = 0;
    n_parsed_docs = 0;
    error_on_last_attempt= false;
    load_next_batch = true;
}


int JsonStream::json_parse(ParsedJson &pj, bool realloc_if_needed) {
    //return json_parse_ptr.load(std::memory_order_relaxed)(buf, len, batch_size, pj, realloc_if_needed);

    if (pj.byte_capacity == 0) {
        const bool allocok = pj.allocate_capacity(batch_size, batch_size);
        if (!allocok) {
            std::cerr << "can't allocate memory" << std::endl;
            return false;
        }
    }
    else if (pj.byte_capacity < batch_size) {
        return simdjson::CAPACITY;
    }

    //Quick heuristic to see if it's worth parsing the remaining data in the batch
    if(!load_next_batch && n_bytes_parsed > 0) {
        const auto remaining_data = batch_size - current_buffer_loc;
        const auto avg_doc_len = (float) n_bytes_parsed / n_parsed_docs;

        if(remaining_data < avg_doc_len)
            load_next_batch = true;
    }

    if (load_next_batch){

        buf = &buf[current_buffer_loc];
        len -= current_buffer_loc;
        n_bytes_parsed += current_buffer_loc;

        batch_size = std::min(batch_size, len);

        int stage1_is_ok = simdjson::find_structural_bits<Architecture::HASWELL>(buf, batch_size, pj, true);

        if (stage1_is_ok != simdjson::SUCCESS) {
            pj.error_code = stage1_is_ok;
            return pj.error_code;
        }

        load_next_batch = false;

        //If we loaded a perfect amount of documents last time, we need to skip the first element,
        // because it represents the end of the last document
        next_json = next_json == 1;
    }


    int res = unified_machine<Architecture::HASWELL>(buf, len, pj, next_json);

    if (res == simdjson::SUCCESS_AND_HAS_MORE) {
        error_on_last_attempt = false;
        n_parsed_docs++;
        //Check if we loaded a perfect amount of json documents and we are done parsing them.
        //Since we don't know the position of the next json document yet, point the current_buffer_loc to the end
        //of the last loaded document and start parsing at structural_index[1] for the next batch.
        // It should point to the start of the first document in the new batch
        if(next_json > 0 && pj.structural_indexes[next_json] == 0) {
            current_buffer_loc = pj.structural_indexes[next_json - 1];
            next_json = 1;
            load_next_batch = true;
        }

        else current_buffer_loc = pj.structural_indexes[next_json];
    }
    //TODO: have a more precise error check
    //Give it two chances for now.  We assume the error is because the json was not loaded completely in this batch.
    //Load a new batch and if the error persists, it's a genuine error.
    else if ( res > 1 && !error_on_last_attempt) {
        load_next_batch = true;
        error_on_last_attempt = true;
        res = json_parse(pj);
    }

    return res;
}

size_t JsonStream::get_current_buffer_loc() const {
    return current_buffer_loc;
}

size_t JsonStream::get_n_parsed_docs() const {
    return n_parsed_docs;
}

size_t JsonStream::get_n_bytes_parsed() const {
    return n_bytes_parsed;
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