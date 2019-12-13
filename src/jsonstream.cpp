#include "simdjson/jsonstream.h"
#include "simdjson/isadetection.h"
#include <map>

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

void JsonStream::set_new_buffer(const char *buf, size_t len) {
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
    if(current_buffer_loc == last_json_buffer_loc)
        load_next_batch = true;
#endif

    if (load_next_batch){
#ifdef SIMDJSON_THREADS_ENABLED
        //First time loading
        if(!stage_1_thread.joinable()){
            _buf = &_buf[current_buffer_loc];
            _len -= current_buffer_loc;
            n_bytes_parsed += current_buffer_loc;

            _batch_size = std::min(_batch_size, _len);
            int stage1_is_ok = (*best_stage1)(_buf, _batch_size, pj, true);

            if (stage1_is_ok != simdjson::SUCCESS) {
                pj.error_code = stage1_is_ok;
                return pj.error_code;
            }
        }

            //the second thread is running or done.
        else{
            stage_1_thread.join();
            std::swap(pj.structural_indexes, pj_thread.structural_indexes);
            pj.n_structural_indexes = pj_thread.n_structural_indexes;

            _buf = &_buf[last_json_buffer_loc];
            _len -= last_json_buffer_loc;
            n_bytes_parsed += last_json_buffer_loc;
            last_json_buffer_loc = 0; //because we want to use it in the if above.
        }

        if(_len-_batch_size > 0) {
            last_json_buffer_loc = find_last_json_buf_loc(pj);
            _batch_size = std::min(_batch_size, _len - last_json_buffer_loc);
            if(_batch_size>0)
                stage_1_thread = std::thread(
                        static_cast<stage1_functype>(*best_stage1),
                        &_buf[last_json_buffer_loc], _batch_size,
                        std::ref(pj_thread),
                        true);

        }
#else
        _buf = &_buf[current_buffer_loc];
        _len -= current_buffer_loc;
        n_bytes_parsed += current_buffer_loc;

        _batch_size = std::min(_batch_size, _len);
        int stage1_is_ok = (*best_stage1)(_buf, _batch_size, pj, true);

        if (stage1_is_ok != simdjson::SUCCESS) {
            pj.error_code = stage1_is_ok;
            return pj.error_code;
        }
#endif
        load_next_batch = false;

        //If we loaded a perfect amount of documents last time, we need to skip the first element,
        // because it represents the end of the last document
        next_json = next_json == 1;
    }

    int res = (*best_stage2)(_buf, _len, pj, next_json);

    if (res == simdjson::SUCCESS_AND_HAS_MORE) {
        error_on_last_attempt = false;
        n_parsed_docs++;
        //Check if we loaded a perfect amount of json documents and we are done parsing them.
        //Since we don't know the position of the next json document yet, point the current_buffer_loc to the end
        //of the last loaded document and start parsing at structural_index[1] for the next batch.
        // It should point to the start of the first document in the new batch
        if(next_json == pj.n_structural_indexes) {
            current_buffer_loc = pj.structural_indexes[next_json - 1];
            next_json = 1;
            load_next_batch = true;
        }

        else {
            current_buffer_loc = pj.structural_indexes[next_json];
        }
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

#ifdef SIMDJSON_THREADS_ENABLED
size_t JsonStream::find_last_json_buf_loc(const ParsedJson &pj) {
    auto last_i = pj.n_structural_indexes - 1;
    if (pj.structural_indexes[last_i] == _batch_size)
        last_i = pj.n_structural_indexes - 2;
    auto arr_cnt = 0;
    auto obj_cnt = 0;
    for (auto i = last_i; i > 0; i--) {
        auto idxb = pj.structural_indexes[i];
        switch (_buf[idxb]) {
            case ':':
            case ',':
                continue;
            case '}':
                obj_cnt--;
                continue;
            case ']':
                arr_cnt--;
                continue;
            case '{':
                obj_cnt++;
                break;
            case '[':
                arr_cnt++;
                break;
        }
        auto idxa = pj.structural_indexes[i - 1];
        switch (_buf[idxa]) {
            case '{':
            case '[':
            case ':':
            case ',':
                continue;
        }
        if (!arr_cnt && !obj_cnt)
            return pj.structural_indexes[last_i+1];
        return idxb;
    }
    return 0;
}
#endif

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
}
