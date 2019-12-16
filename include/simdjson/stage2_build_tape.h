#ifndef SIMDJSON_STAGE2_BUILD_TAPE_H
#define SIMDJSON_STAGE2_BUILD_TAPE_H

#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"
#include "simdjson/simdjson.h"

namespace simdjson {



class Stage2{
public:
    typedef enum {
        root = 0,
        root_continue = 1,
        array_begin = 2,
        array_continue = 3,
        array_element = 4,
        object_begin = 5,
        object_continue = 6,
        object_key = 7,
        fail,
        succeed
    } State;

protected:
    State state{};
    uint32_t i = 0; /* index of the structural character (0,1,2,3...) */
    uint32_t idx{}; /* location of the structural character in the input (buf)   */
    uint8_t c{};    /* used to track the (structural) character we are looking at,
                       updated */
    /* by UPDATE_CHAR macro */
    uint32_t depth = 1; /* could have an arbitrary starting depth */
    ParsedJson *_pj = nullptr;
    const uint8_t *_buf{};
    size_t _len{};

    State scope_stack[1024]{};

    virtual inline State handle_root() =0;
    virtual inline State handle_root_continue() =0;
    virtual inline State handle_array_begin() =0;
    virtual inline State handle_array_continue() =0;
    virtual inline State handle_array_element() =0;
    virtual inline State handle_object_begin() =0;
    virtual inline State handle_object_continue()=0;
    virtual inline State handle_object_key()=0;


    virtual inline State scope_end()=0;

    void update_char() {
        idx = _pj->structural_indexes[i++];
        c = _buf[idx];
    };
    virtual inline void conclude()=0;


public:
    virtual void initialize(ParsedJson &pj)=0;

    virtual int run(const uint8_t *buf, const size_t &len, uint32_t offset)=0;
    ParsedJson *getPj() const { return _pj; };
    uint32_t getCurrentIndex() { return i; };
};

template <Architecture T = Architecture::NATIVE>
Stage2* create_stage2();

template <Architecture T = Architecture::NATIVE>
WARN_UNUSED int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj);

template <Architecture T = Architecture::NATIVE>
int unified_machine(const char *buf, size_t len, ParsedJson &pj) {
  return unified_machine<T>(reinterpret_cast<const uint8_t *>(buf), len, pj);
}



// Streaming
template <Architecture T = Architecture::NATIVE>
WARN_UNUSED int
unified_machine(const uint8_t *buf, size_t len, ParsedJson &pj, size_t &next_json);

template <Architecture T = Architecture::NATIVE>
int unified_machine(const char *buf, size_t len, ParsedJson &pj, size_t &next_json) {
    return unified_machine<T>(reinterpret_cast<const uint8_t *>(buf), len, pj, next_json);
}


} // namespace simdjson

#endif
