#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
#if SIMDJSON_EXCEPTIONS
    try {
        simdjson::dom::parser pj;
        auto result=pj.parse(Data, Size);
        UNUSED auto v=result.value();
    }catch(...) {
    }
#endif
    return 0;
}
