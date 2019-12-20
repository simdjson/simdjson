#ifndef SIMDJSON_JSONPARSER_H
#define SIMDJSON_JSONPARSER_H

#include "simdjson/common_defs.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/padded_string.h"
#include "simdjson/parsedjson.h"
#include "simdjson/parsedjsoniterator.h"
#include "simdjson/simdjson.h"
#include "simdjson/stage1_find_marks.h"
#include "simdjson/stage2_build_tape.h"
#include <string>

#ifdef SIMDJSON_THREADS_ENABLED
#include <thread>
#include <atomic>

#endif

typedef int (*stage1_functype)(const unsigned char *buf, size_t len, simdjson::ParsedJson &pj);

const auto MAX_BATCH_SIZE = 4000000l;
namespace simdjson {
    // json_parse_implementation is the generic function, it is specialized for
    // various architectures, e.g., as
    // json_parse_implementation<Architecture::HASWELL> or
    // json_parse_implementation<Architecture::ARM64>
    template<Architecture T>
    int json_parse_implementation(const uint8_t *buf, size_t len, ParsedJson &pj,
                                  bool realloc_if_needed = true) {
#ifdef SIMDJSON_THREADS_ENABLED
        if (pj.byte_capacity < len) {
            return simdjson::CAPACITY;
        }

        auto s2 = simdjson::create_stage2<simdjson::Architecture::HASWELL>();
        auto remaining_len = (long) len;
        auto batch_size = std::min(remaining_len, MAX_BATCH_SIZE);
        auto thread_batch_size = 0;
        int res;
        std::thread stage1_thread{};
        simdjson::ParsedJson pj_thread{};
        std::atomic<int> stage1_is_ok;

        s2->initialize(pj);
        do {
            auto offset = 0;
            if (!stage1_thread.joinable()) {
                stage1_is_ok = simdjson::find_structural_bits<T>(buf, batch_size, pj);
                if (stage1_is_ok != simdjson::SUCCESS) {
                    pj.error_code = stage1_is_ok;
                    return pj.error_code;
                }
            } else {
                stage1_thread.join();
                if (stage1_is_ok != simdjson::SUCCESS) {
                    pj.error_code = stage1_is_ok;
                    return pj.error_code;
                }
                offset = s2->getCurrentIndex() - pj.n_structural_indexes;
                std::swap(pj_thread.structural_indexes, pj.structural_indexes);
                pj.n_structural_indexes = pj_thread.n_structural_indexes;
            }

            if (remaining_len - pj.structural_indexes[pj.n_structural_indexes - 1] > 0) {
                pj.n_structural_indexes -= 4;
                thread_batch_size = std::min(remaining_len - pj.structural_indexes[pj.n_structural_indexes],
                                             MAX_BATCH_SIZE);

                if(pj_thread.byte_capacity == 0 && !pj_thread.allocate_capacity(thread_batch_size))
                        std::cerr << "failure during memory allocation of pj_thread" << std::endl;

                auto next_idx = pj.structural_indexes[pj.n_structural_indexes];
                stage1_thread = std::thread(
                        [&stage1_is_ok, &pj_thread, buf, thread_batch_size, next_idx] {
                            stage1_is_ok = simdjson::find_structural_bits<T>(
                                    &buf[next_idx],
                                    thread_batch_size,
                                    pj_thread
                            );
                        });
            }


            res = s2->run(buf, batch_size, offset);
            buf = &buf[pj.structural_indexes[pj.n_structural_indexes]];
            remaining_len -= pj.structural_indexes[pj.n_structural_indexes];
            batch_size = thread_batch_size;

        } while (res == simdjson::SUCCESS_AND_HAS_MORE);

        return res;
#else

        if (pj.byte_capacity < len) {
            return simdjson::CAPACITY;
        }
        bool reallocated = false;
        if (realloc_if_needed) {
            const uint8_t *tmp_buf = buf;
            buf = (uint8_t *) allocate_padded_buffer(len);
            if (buf == NULL)
                return simdjson::MEMALLOC;
            memcpy((void *) buf, tmp_buf, len);
            reallocated = true;
        }   // if(realloc_if_needed) {
        int stage1_is_ok = simdjson::find_structural_bits<T>(buf, len, pj);
        if (stage1_is_ok != simdjson::SUCCESS) {
            if (reallocated) { // must free before we exit
                aligned_free((void *) buf);
            }
            pj.error_code = stage1_is_ok;
            return pj.error_code;
        }
        int res = unified_machine<T>(buf, len, pj);
        if (reallocated) {
            aligned_free((void *) buf);
        }
        return res;
#endif
    }

// Parse a document found in buf.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// You can also check validity by calling pj.is_valid(). The same ParsedJson can
// be reused for other documents.
//
// If realloc_if_needed is true (default) then a temporary buffer is created
// when needed during processing (a copy of the input string is made). The input
// buf should be readable up to buf + len + SIMDJSON_PADDING if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage). The ParsedJson object can be reused.

    int json_parse(const uint8_t *buf, size_t len, ParsedJson &pj,
                   bool realloc_if_needed = true);

// Parse a document found in buf.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// You can also check validity
// by calling pj.is_valid(). The same ParsedJson can be reused for other
// documents.
//
// If realloc_if_needed is true (default) then a temporary buffer is created
// when needed during processing (a copy of the input string is made). The input
// buf should be readable up to buf + len + SIMDJSON_PADDING  if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage). The ParsedJson object can be reused.
    int json_parse(const char *buf, size_t len, ParsedJson &pj,
                   bool realloc_if_needed = true);

// We do not want to allow implicit conversion from C string to std::string.
    int json_parse(const char *buf, ParsedJson &pj) = delete;

// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// A temporary buffer is created when needed during processing
// (a copy of the input string is made).
    inline int json_parse(const std::string &s, ParsedJson &pj) {
        return json_parse(s.data(), s.length(), pj, true);
    }

// Parse a document found in in string s.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)).
//
// The function returns simdjson::SUCCESS (an integer = 0) in case of a success
// or an error code from simdjson/simdjson.h in case of failure such as
// simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
// the simdjson::error_message function converts these error codes into a
// string).
//
// You can also check validity
// by calling pj.is_valid(). The same ParsedJson can be reused for other
// documents.
    inline int json_parse(const padded_string &s, ParsedJson &pj) {
        return json_parse(s.data(), s.length(), pj, false);
    }

// Build a ParsedJson object. You can check validity
// by calling pj.is_valid(). This does the memory allocation needed for
// ParsedJson. If realloc_if_needed is true (default) then a temporary buffer is
// created when needed during processing (a copy of the input string is made).
//
// The input buf should be readable up to buf + len + SIMDJSON_PADDING  if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage).
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// This is a convenience function which calls json_parse.
    WARN_UNUSED
    ParsedJson build_parsed_json(const uint8_t *buf, size_t len,
                                 bool realloc_if_needed = true);

    WARN_UNUSED
// Build a ParsedJson object. You can check validity
// by calling pj.is_valid(). This does the memory allocation needed for
// ParsedJson. If realloc_if_needed is true (default) then a temporary buffer is
// created when needed during processing (a copy of the input string is made).
//
// The input buf should be readable up to buf + len + SIMDJSON_PADDING if
// realloc_if_needed is false, all bytes at and after buf + len  are ignored
// (can be garbage).
//
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// This is a convenience function which calls json_parse.
    inline ParsedJson build_parsed_json(const char *buf, size_t len,
                                        bool realloc_if_needed = true) {
        return build_parsed_json(reinterpret_cast<const uint8_t *>(buf), len,
                                 realloc_if_needed);
    }

// We do not want to allow implicit conversion from C string to std::string.
    ParsedJson build_parsed_json(const char *buf) = delete;

// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)). Return SUCCESS (an integer = 0) in case of a
// success. You can also check validity by calling pj.is_valid(). The same
// ParsedJson can be reused for other documents.
//
// A temporary buffer is created when needed during processing
// (a copy of the input string is made).
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// This is a convenience function which calls json_parse.
    WARN_UNUSED
    inline ParsedJson build_parsed_json(const std::string &s) {
        return build_parsed_json(s.data(), s.length(), true);
    }

// Parse a document found in in string s.
// You need to preallocate ParsedJson with a capacity of len (e.g.,
// pj.allocate_capacity(len)). Return SUCCESS (an integer = 0) in case of a
// success. You can also check validity by calling pj.is_valid(). The same
// ParsedJson can be reused for other documents.
//
// The content should be a valid JSON document encoded as UTF-8. If there is a
// UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
// discouraged.
//
// This is a convenience function which calls json_parse.
    WARN_UNUSED
    inline ParsedJson build_parsed_json(const padded_string &s) {
        return build_parsed_json(s.data(), s.length(), false);
    }

} // namespace simdjson
#endif
