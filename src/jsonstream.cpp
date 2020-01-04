#include <map>
#include "simdjson/jsonstream.h"
#include "simdjson/isadetection.h"
#include "jsoncharutils.h"


using namespace simdjson;
void find_the_best_supported_implementation();

typedef int (*stage1_functype)(const char *buf, size_t len, ParsedJson &pj, bool streaming);
typedef int (*stage2_functype)(const char *buf, size_t len, ParsedJson &pj, size_t &next_json);

stage1_functype best_stage1;
stage2_functype best_stage2;

JsonStream::JsonStream(const char *buf, size_t len, size_t batchSize)
        : _buf(buf), _len(len), _batch_size(batchSize) {
    find_the_best_supported_implementation();
}

JsonStream::~JsonStream() {
#ifdef SIMDJSON_THREADS_ENABLED
    if(stage_1_thread.joinable()) {
      stage_1_thread.join();
    }
#endif
}


void JsonStream::set_new_buffer(const char *buf, size_t len) {
#ifdef SIMDJSON_THREADS_ENABLED
    if(stage_1_thread.joinable()) {
      stage_1_thread.join();
    }
#endif
    this->_buf = buf;
    this->_len = len;
    _batch_size = 0;
    _batch_size = 0;
    next_json = 0;
    current_buffer_loc = 0;
    n_parsed_docs = 0;
    error_on_last_attempt= false;
    load_next_batch = true;
}

// todo: this code is too complicated, it should be greatly simplified
int JsonStream::json_parse(ParsedJson &pj) {
    if (pj.byte_capacity == 0) {
        const bool allocok = pj.allocate_capacity(_batch_size);
        const bool allocok_thread = pj_thread.allocate_capacity(_batch_size);
        if (!allocok || !allocok_thread) {
            std::cerr << "can't allocate memory" << std::endl;
            return false;
        }
    }
    else if (pj.byte_capacity < _batch_size) {
        return simdjson::CAPACITY;
    }
#ifdef SIMDJSON_THREADS_ENABLED
    if(current_buffer_loc == last_json_buffer_loc) {
        load_next_batch = true;
    }
#endif

    if (load_next_batch) {
#ifdef SIMDJSON_THREADS_ENABLED
        //First time loading
        if(!stage_1_thread.joinable()) {
            _buf = _buf + current_buffer_loc;
            _len -= current_buffer_loc;
            n_bytes_parsed += current_buffer_loc;
            _batch_size = std::min(_batch_size, _len);
            _batch_size = trimmed_length_safe_utf8((const char*)_buf, _batch_size);
            if(_batch_size == 0) {
                pj.error_code = simdjson::UTF8_ERROR;
                return pj.error_code;
            }
            int stage1_is_ok = best_stage1(_buf, _batch_size, pj, true);
            if (stage1_is_ok != simdjson::SUCCESS) {
                pj.error_code = stage1_is_ok;
                return pj.error_code;
            }
            size_t last_index = find_last_json_buf_idx(_buf, _batch_size, pj);
            if(last_index == 0) {
              pj.error_code = simdjson::EMPTY;
              return pj.error_code;
            }
            pj.n_structural_indexes = last_index + 1;
        }
        // the second thread is running or done.
        else {
            stage_1_thread.join();
            if (stage1_is_ok_thread != simdjson::SUCCESS) {
                pj.error_code = stage1_is_ok_thread;
                return pj.error_code;
            }
            std::swap(pj.structural_indexes, pj_thread.structural_indexes);
            pj.n_structural_indexes = pj_thread.n_structural_indexes;
            _buf = _buf + last_json_buffer_loc;
            _len -= last_json_buffer_loc;
            n_bytes_parsed += last_json_buffer_loc;
            last_json_buffer_loc = 0; //because we want to use it in the if above.
        }
        if(_len - _batch_size > 0) {
            last_json_buffer_loc =  pj.structural_indexes[find_last_json_buf_idx(_buf,_batch_size,pj)];
            _batch_size = std::min(_batch_size, _len - last_json_buffer_loc);
            if(_batch_size > 0) {
                _batch_size = trimmed_length_safe_utf8((const char*)(_buf + last_json_buffer_loc), _batch_size);
                if(_batch_size == 0) {
                  pj.error_code = simdjson::UTF8_ERROR;
                  return pj.error_code;
                }
                // let us capture read-only variables
                const char * const b = _buf + last_json_buffer_loc;
                const size_t bs = _batch_size;
                // we call the thread on a lambda that will update  this->stage1_is_ok_thread
                // there is only one thread that may write to this value
                stage_1_thread = std::thread(
                   [this, b, bs] {
                     this->stage1_is_ok_thread = best_stage1(b, bs, this->pj_thread, true);
                });
            }
        }

        //If we loaded a perfect amount of documents last time, we need to skip the first element,
        // because it represents the end of the last document
        next_json = next_json == 1;
#else
        _buf = _buf + current_buffer_loc;
        _len -= current_buffer_loc;
        n_bytes_parsed += current_buffer_loc;

        _batch_size = std::min(_batch_size, _len);
        _batch_size = trimmed_length_safe_utf8((const char*)_buf, _batch_size);
        int stage1_is_ok = best_stage1(_buf, _batch_size, pj, true);
        if (stage1_is_ok != simdjson::SUCCESS) {
            pj.error_code = stage1_is_ok;
            return pj.error_code;
        }
        size_t last_index = find_last_json_buf_idx(_buf, _batch_size, pj);
        if(last_index == 0) {
            pj.error_code = simdjson::EMPTY;
            return pj.error_code;
        }
        pj.n_structural_indexes = last_index + 1;
#endif
        load_next_batch = false;

    }
//#define SIMDJSON_IREALLYNEEDHELP
#ifdef SIMDJSON_IREALLYNEEDHELP // for debugging
    size_t oldnext_json = next_json;
#endif
    int res = best_stage2(_buf, _len, pj, next_json);
#ifdef SIMDJSON_IREALLYNEEDHELP // for debugging
    int sizeofdoc = pj.structural_indexes[next_json]-pj.structural_indexes[oldnext_json];
    printf("size = %d\n", sizeofdoc);
    if(sizeofdoc > 0) {
      printf("%.*s\n",sizeofdoc, _buf + pj.structural_indexes[oldnext_json]);
    } else {
      printf("<empty>\n");
    }
#endif

    if (res == simdjson::SUCCESS_AND_HAS_MORE) {
        error_on_last_attempt = false; 
        n_parsed_docs++;
        current_buffer_loc = pj.structural_indexes[next_json];
    } else if (res == simdjson::SUCCESS) {
        error_on_last_attempt = false; 
        n_parsed_docs++;
        if(_len > _batch_size) {
            current_buffer_loc = pj.structural_indexes[next_json - 1];
#ifndef SIMDJSON_THREADS_ENABLED
            next_json = 1;
#endif
            load_next_batch = true;
            res = simdjson::SUCCESS_AND_HAS_MORE;
        }
    }
    // We assume the error is because the json was not loaded completely in this batch.
    // Load a new batch and if the error persists, it's a genuine error.
    else if(!error_on_last_attempt) {
                printf("failure?\n");
 
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
void find_the_best_supported_implementation() {
    uint32_t supports = detect_supported_architectures();
    // Order from best to worst (within architecture)
#ifdef IS_X86_64
    constexpr uint32_t haswell_flags =
            instruction_set::AVX2 | instruction_set::PCLMULQDQ |
            instruction_set::BMI1 | instruction_set::BMI2;
    constexpr uint32_t westmere_flags =
            instruction_set::SSE42 | instruction_set::PCLMULQDQ;
    if ((haswell_flags & supports) == haswell_flags) {
        best_stage1 = simdjson::find_structural_bits<Architecture ::HASWELL>;
        best_stage2 = simdjson::unified_machine<Architecture ::HASWELL>;
        return;
    }
    if ((westmere_flags & supports) == westmere_flags) {
        best_stage1 = simdjson::find_structural_bits<Architecture ::WESTMERE>;
        best_stage2 = simdjson::unified_machine<Architecture ::WESTMERE>;
        return;
    }
#endif
#ifdef IS_ARM64
    if (supports & instruction_set::NEON) {
        best_stage1 = simdjson::find_structural_bits<Architecture ::ARM64>;
        best_stage2 = simdjson::unified_machine<Architecture ::ARM64>;
        return;
    }
#endif
    std::cerr << "The processor is not supported by simdjson." << std::endl;
    // we throw an exception since this should not be recoverable
    throw new std::runtime_error("unsupported architecture");
}
