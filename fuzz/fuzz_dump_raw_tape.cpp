#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "NullBuffer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    try {
        simdjson::dom::parser pj;
        auto elem=pj.parse(Data, Size);
        auto v=elem.value();
        NulOStream os;
        UNUSED auto dumpstatus=v.dump_raw_tape(os);
    } catch (...) {
    }
    return 0;
}
