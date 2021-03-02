#ifndef SIMDJSON_JSON_TYPE_INL_H
#define SIMDJSON_JSON_TYPE_INL_H

#include "simdjson/json_type.h"

namespace simdjson {

inline std::ostream& operator<<(std::ostream& out, json_type type) noexcept {
    switch (type) {
        case json_type::array: out << "array"; break;
        case json_type::object: out << "object"; break;
        case json_type::number: out << "number"; break;
        case json_type::string: out << "string"; break;
        case json_type::boolean: out << "boolean"; break;
        case json_type::null: out << "null"; break;
        default: SIMDJSON_UNREACHABLE();
    }
    return out;
}

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson_result<json_type> &type) noexcept(false) {
    return out << type.value();
}
#endif

} // namespace simdjson

#endif // SIMDJSON_JSON_TYPE_INL_H
