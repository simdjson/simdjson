#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

#include "NullBuffer.h"


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
#if SIMDJSON_EXCEPTIONS
    try {
        simdjson::dom::parser pj;
        auto elem=pj.parse(Data, Size);
        NulOStream os;
        os<<elem;
    } catch (...) {
    }
#endif
    return 0;
}
