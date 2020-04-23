#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "NullBuffer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    simdjson::dom::parser parser;
    simdjson::error_code error;
    simdjson::dom::element elem;
    parser.parse(Data, Size).tie(elem, error);
    if (error) { return 1; }

    NulOStream os;
    UNUSED auto dumpstatus = elem.dump_raw_tape(os);
    return 0;
}
