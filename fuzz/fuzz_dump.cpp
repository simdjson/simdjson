#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "NullBuffer.h"

// example from doc/basics.md#tree-walking-and-json-element-types
static void print_json(std::ostream& os, simdjson::dom::element element) {
    const char endl='\n';
    switch (element.type()) {
    case simdjson::dom::element_type::ARRAY:
        os << "[";
        for (simdjson::dom::element child : simdjson::dom::array(element)) {
            print_json(os, child);
            os << ",";
        }
        os << "]";
        break;
    case simdjson::dom::element_type::OBJECT:
        os << "{";
        for (simdjson::dom::key_value_pair field : simdjson::dom::object(element)) {
            os << "\"" << field.key << "\": ";
            print_json(os, field.value);
        }
        os << "}";
        break;
    case simdjson::dom::element_type::INT64:
        os << int64_t(element) << endl;
        break;
    case simdjson::dom::element_type::UINT64:
        os << uint64_t(element) << endl;
        break;
    case simdjson::dom::element_type::DOUBLE:
        os << double(element) << endl;
        break;
    case simdjson::dom::element_type::STRING:
        os << std::string_view(element) << endl;
        break;
    case simdjson::dom::element_type::BOOL:
        os << bool(element) << endl;
        break;
    case simdjson::dom::element_type::NULL_VALUE:
        os << "null" << endl;
        break;
    }
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
#if SIMDJSON_EXCEPTIONS
    try {
        simdjson::dom::parser pj;
        auto elem=pj.parse(Data, Size);
        auto v=elem.value();
        NulOStream os;
        //std::ostream& os(std::cout);
        print_json(os,v);
    } catch (...) {
    }
#endif
    return 0;
}
