#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "NullBuffer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    simdjson::dom::parser parser;
    simdjson::dom::element elem;
    auto error = parser.parse(Data, Size).get(elem);
    if (error) { return 1; }

    NulOStream os;
    SIMDJSON_UNUSED auto dumpstatus = elem.dump_raw_tape(os);
    return 0;
}
