#ifndef SIMDJSON_STAGE1_FIND_MARKS_H
#define SIMDJSON_STAGE1_FIND_MARKS_H

#include "simdjson/common_defs.h"

struct ParsedJson;

class SetBitsCounter {
    static constexpr uint8_t  set_bytes_array[16] = {
            0, 1, 1, 2, 1, 2, 2, 3,
            1, 2, 2, 3, 2, 3, 3, 4
    };
public:
    static uint64_t count_set_bits(uint64_t num);
};

WARN_UNUSED
bool find_structural_bits(const uint8_t *buf, size_t len, ParsedJson &pj);

WARN_UNUSED
bool find_structural_bits(const char *buf, size_t len, ParsedJson &pj);

#endif
