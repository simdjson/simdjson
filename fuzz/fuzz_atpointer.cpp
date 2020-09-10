#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

#include <string_view>

// split the input in a pointer and a document

struct FuzzData {
    std::string_view json_pointer;
    std::string_view json_doc;
};

FuzzData split(const uint8_t *Data, size_t Size) {
    const std::string_view sep("\n~~~\n");
    std::string_view all((const char*)Data,Size);
    auto pos=all.find(sep);
    if(pos==std::string_view::npos) {
        //not found.
        return FuzzData{std::string_view{},all};
    } else {
        return FuzzData{std::string_view{all.substr(0,pos)},all.substr(pos+sep.size())};
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
   // if(Size<1)
    //    return 0;

    const auto fd=split(Data,Size);

    simdjson::dom::parser parser;

    try {
        simdjson::dom::element doc =  parser.parse(fd.json_doc.data(),fd.json_doc.size());
        auto x=doc.at_pointer(fd.json_pointer);
        if(fd.json_pointer==std::string_view{"/hello"} && x.is_string()) {
            if(std::string_view{x.get_c_str()}=="world") {
             std::abort();
            }
            //std::puts(x.get_c_str());
        }
    } catch (...) {
       // std::puts("darnit");
    }

    return 0;
}
